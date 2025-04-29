// 负责管理和Mysql的单个连接，执行查询，更新，连接检查和重连等
#pragma once
#include <memory>
#include <string>
#include <mutex>
#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <mysql_driver.h>
#include <mysql/mysql.h>
#include <muduo/base/Logging.h>
#include "DbException.h"

namespace http
{
    namespace db
    {
        class DbConnection
        {
        public:
            DbConnection(const std::string &host,
                         const std::string &user,
                         const std::string &password,
                         const std::string &database);
            ~DbConnection();

            // 禁止拷贝构造
            DbConnection(const DbConnection &) = delete;
            DbConnection &operator=(const DbConnection &) = delete;

            bool isValid();
            void reconnect();
            void cleanup();

            template <typename... Args>
            sql::ResultSet *executeQuery(const std::string &sql, Args &&...args)
            {
                std::lock_guard<std::mutex> lock(mutex_);
                try
                {
                    // 直接创建新的预处理语句，不使用缓存
                    std::unique_ptr<sql::PreparedStatement> stmt(conn_->prepareStatement(sql));
                    bindParams(stmt.get(), 1, std::forward<Args>(args)...);
                    // std::string sql = "SELECT * FROM users WHERE id = ?"; 绑的是这种

                    return stmt->executeQuery();
                }
                catch (const sql::SQLException &e)
                {
                    LOG_ERROR << "Query failed: " << e.what() << ",SQL: " << sql;
                    throw DbException(e.what());
                }
            }

            template <typename... Args>
            int executeUpdate(const std::string &sql, Args &&...args)
            {
                std::lock_guard<std::mutex> lock(mutex_);
                try
                {
                    // 直接创建新的预处理语句，不适用缓存
                    std::unique_ptr<sql::PreparedStatement> stmt(conn_->prepareStatement(sql));

                    bindParams(stmt.get(), 1, std::forward<Args>(args)...);
                    return stmt->executeUpdate();
                }
                catch (const sql::SQLException &e)
                {
                    LOG_ERROR << "Update failed : " << e.what() << ", SQL: " << sql;
                    throw DbException(e.what());
                }
            }

            bool ping();

        private:
            // 下面三个函数构成一个递归模板函数

            // 辅助函数，递归终止条件,当没有参数要绑定时，递归结束
            void bindParams(sql::PreparedStatement *, int) {}

            // 辅助函数，绑定参数，先绑定第一个参数，然后递归绑定后面的参数
            template <typename T, typename... Args>
            void bindParams(sql::PreparedStatement *stmt, int index, T &&value, Args &&...args)
            {
                stmt->setString(index, std::to_string(std::forward<T>(value)));
                bindParams(stmt, index + 1, std::forward<Args>(args)...);
            }
            /*语法解释*/
            /*stmt	sql::PreparedStatement* 类型，表示预处理 SQL 对象
            setString(index, ...)	绑定第 index 个 ? 占位符，值是字符串类型
            std::to_string(...)	把数字类型（如 int、double）转成 std::string
            std::forward<T>(value)	保留 value 的左值/右值属性，实现完美转发*/

            // 特化string类型的参数，不需要对string进行转换，直接传入
            template <typename... Args>
            void bindParams(sql::PreparedStatement *stmt, int index, const std::string &value, Args &&...args)
            {
                stmt->setString(index, value);
                bindParams(stmt, index + 1, std::forward<Args>(args)...);
            }

        private:
            std::shared_ptr<sql::Connection>    conn_;
            std::string                         host_;
            std::string                         user_;
            std::string                         password_;
            std::string                         database_;
            std::mutex                          mutex_;
        };

    } // namespace db
} // namespace http