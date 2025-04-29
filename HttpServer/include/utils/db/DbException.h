//专用于数据库出错时的异常类
#pragma once 

#include <stdexcept>
#include <string>

namespace http{
    namespace db{
        class DbException : public std::runtime_error{
        public:
            //在初始化列表里面，调用调用父类的构造函数，向其中传入message
            explicit DbException(const std::string& message) : std::runtime_error(message){}
            explicit DbException(const char* message) : std::runtime_error(message){}
        };

    }// namesapce dp
}// namespace http