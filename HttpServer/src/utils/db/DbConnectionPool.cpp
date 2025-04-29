#include "../../../include/utils/db/DbConnectionPool.h"
#include "../../../include/utils/db/DbException.h"
#include <muduo/base/Logging.h>

namespace http
{
    namespace db
    {
        void DbConnectionPool::init(const std::string &host,
                                    const std::string &user,
                                    const std::string &password,
                                    const std::string &database,
                                    size_t poolSize)
        {
            // 连接池会被多个线程访问，因此加锁
            std::lock_guard<std::mutex> lock(mutex_);
            // 确保只初始化一次
            if (initialized_)
            {
                return;
            }

            host_ = host;
            user_ = user;
            password_ = password;
            database_ = database;

            // 创建连接
            for (size_t i = 0; i < poolSize; i++)
            {
                connections_.push(createConnection());
            }

            initialized_ = true;
            LOG_INFO << "Database connection pool initialized with " << poolSize << "connections";
        }

        DbConnectionPool::DbConnectionPool()
        {
            checkThread_ = std::thread(&DbConnectionPool::checkConnections, this);
            checkThread_.detach();
        }

        DbConnectionPool::~DbConnectionPool()
        {
            std::lock_guard<std::mutex> lock(mutex_);
            while (!connections_.empty())
            {
                connections_.pop();
            }
            LOG_INFO << "Database connection pool destroyed";
        }

        // 修改获取连接的函数
        std::shared_ptr<DbConnection> DbConnectionPool::getConnection()
        {
            std::shared_ptr<DbConnection> conn;
            { // 下面上锁
                std::unique_lock<std::mutex> lock(mutex_);

                // 当连接池空的时候要等
                while (connections_.empty())
                {
                    if (!initialized_)
                    {
                        throw DbException("Connection pool not initialized");
                    }
                    LOG_INFO << "Waiting for available connection...";
                    cv_.wait(lock); // 释放锁阻塞线程
                }
                conn = connections_.front();
                connections_.pop();
            } // 释放锁
            try
            {
                // 在锁外检查连接
                if (!conn->ping())
                {
                    LOG_WARN << "Connection lost, attempting to reconnect...";
                    conn->reconnect();
                }

                return std::shared_ptr<DbConnection>(conn.get(),
                                                     [this, conn](DbConnection *)
                                                     {
                                                         std::lock_guard<std::mutex> lock(mutex_);
                                                         connections_.push(conn);
                                                         cv_.notify_one();
                                                     });
                // 这个智能指针接收两个参数，一个是conn.get()返回的裸指针
                // 一个是lambda表达式，作为deleter，在shared_ptr中，会维护一个计数器，引用计数归零时，用deleter清除
                // 捕获this是便于使用DbConnectionPool的对象，使用类内成员
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "Failed to get Connection: " << e.what();
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    connections_.push(conn);
                    cv_.notify_one();
                }
                throw; // 在catch里面重新抛出捕获的异常，这里是回收连接之后再次抛出给上层
            }
        }

        // 创建一个 DbConnection 对象，并用 shared_ptr 管理它。
        // 在一块内存中同时分配对象本体和引用计数控制块（性能比 new + shared_ptr 更好）
        std::shared_ptr<DbConnection> DbConnectionPool::createConnection()
        {
            return std::make_shared<DbConnection>(host_, user_, password_, database_);
        }

        // 修改检查连接的函数
        void DbConnectionPool::checkConnections()
        {
            while (true)
            {
                try
                {

                    std::vector<std::shared_ptr<DbConnection>> connsToCheck;
                    { // 上锁，进行拷贝，拷贝下来再检查，防止长时间持锁检查
                        std::unique_lock<std::mutex> lock(mutex_);
                        if (connections_.empty())
                        {
                            // 如果连接池是空的，缓1秒再进行下一轮，避免一直查询，进入忙等
                            std::this_thread::sleep_for(std::chrono::seconds(1));
                            continue;
                        }

                        auto temp = connections_;
                        while (!temp.empty())
                        { // 把连接池里面所有连接复制给ConnsToCheck
                            connsToCheck.push_back(temp.front());
                            temp.pop();
                        }
                    } // 释放锁

                    // 在锁外检查连接
                    for (auto &conn : connsToCheck)
                    {
                        if (!conn->ping())
                        {
                            try
                            {
                                conn->reconnect();
                            }
                            catch (const std::exception &e)
                            {
                                LOG_ERROR << "Failed to reconnect: " << e.what();
                            }
                        }
                    }
                    std::this_thread::sleep_for(std::chrono::seconds(60));
                }
                catch (const std::exception &e)
                {
                    LOG_ERROR << "Error in check thread: " << e.what();
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                }
            }
        }

    } // namespace db
} // namespace http