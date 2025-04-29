//
#pragma once 
#include "db/DbConnectionPool.h"
#include <string>

namespace http{
    
    class MysqlUtil{
        public:
        static void init(const std::string& host,const std::string& user,
                        const std::string& password, const std::string& database,
                        size_t poolSize = 10){

            http::db::DbConnectionPool::getInstance().init(host,user,password,database,poolSize);
        }

        //模板函数，从连接池中获取一个对象conn，并调用对象的响应方法
        template<typename... Args>
        sql::ResultSet* executeQuery(const std::string& sql,Args&&... args){
            auto conn = http::db::DbConnectionPool::getInstance().getConnection();
            return conn->executeQuery(sql,std::forward<Args>(args)...);
        }
        //std::forward<Args>(args)... 是标准写法，保持每个参数原来的引用类型（左值还是右值）展开成一个个参数，传给底层数据库执行函数
    
        template<typename... Args>
        int executeUpdate(const std::string& sql,Args&&... args){
            auto conn = http::db::DbConnectionPool::getInstance().getConnection();
            return conn->executeUpdate(sql,std::forward<Args>(args)...);
        }
    };

}// namespace http
