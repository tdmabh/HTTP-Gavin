//抽象类，通过重写路由函数作为路由回调执行函数
#pragma once 
#include <string>
#include <memory>
#include "../http/HttpRequest.h"
#include "../http/HttpResponse.h"

namespace http{
namespace router{

class RouterHandler{
public:
    virtual ~RouterHandler() = default;
    virtual void handle(const HttpRequest& req, HttpResponse* resp) = 0;
};


} // namespace router
} // namespace http
