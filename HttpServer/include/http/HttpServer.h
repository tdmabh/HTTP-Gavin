// 框架核心，接入muduo的位置
// 实现新连接的回调，接收数据的消息回调，业务回调等接口

#pragma once

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <unordered_map>

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/base/Logging.h>

#include "HttpContext.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "../router/Router.h"
#include "../session/SessionManager.h"
#include "../middleware/MiddlewareChain.h"
#include "../middleware/cors/CorsMiddleware.h"
#include "../ssl/SslConnection.h"
#include "../ssl/SslContext.h"

class HttpRequest;
class HttpResponse;

namespace http
{

    class HttpServer : muduo::noncopyable
    {

    public:
        using HttpCallback = std::function<void(const http::HttpRequest &, http::HttpResponse *)>;

        HttpServer(int port, const std::string &name, bool useSSL = false,
                   muduo::net::TcpServer::Option option = muduo::net::TcpServer::kNoReusePort);

        void setThreadNum(int numThreads)
        {
            server_.setThreadNum(numThreads);
        }

        void start();

        muduo::net::EventLoop *getLoop() const
        {
            return server_.getLoop();
        }

        void setHttpCallback(const HttpCallback &cb)
        {
            httpCallback_ = cb;
        }

        // 注册静态路由处理器
        void Get(const std::string &path, const HttpCallback &cb)
        {
            router_.registerCallback(HttpRequest::kGet, path, cb);
        }

        void Get(const std::string &path, router::Router::HandlerPtr handler)
        {
            router_.registerHandler(HttpRequest::kGet, path, handler);
        }

        void Post(const std::string &path, const HttpCallback &cb)
        {
            router_.registerCallback(HttpRequest::kPost, path, cb);
        }

        void Post(const std::string &path, router::Router::HandlerPtr handler)
        {
            router_.registerHandler(HttpRequest::kPost, path, handler);
        }

        // 注册动态路由处理函数
        void addRoute(HttpRequest::Method method, const std::string &path, router::Router::HandlerPtr handler)
        {
            router_.addRegexHandler(method, path, handler);
        }

        // 注册动态路由处理函数
        void addRoute(HttpRequest::Method method, const std::string &path, const router::Router::HandlerCallback &callback)
        {
            router_.addRegexCallback(method, path, callback);
        }

        // 设置会话管理器
        void setSessionManager(std::unique_ptr<session::SessionManager> manager)
        {
            sessionManager_ = std::move(manager);
        }

        // 获取会话管理器
        session::SessionManager *getSessionManager() const
        {
            return sessionManager_.get();
        }

        // 添加中间件的方法
        void addMiddleware(std::shared_ptr<middleware::Middleware> middleware)
        {
            middlewareChain_.addMiddleware(middleware);
        }

        void enableSSL(bool enable)
        {
            useSSL_ = enable;
        }

        void setSslConfig(const ssl::SslConfig &config);

    private:
        void initialize();

        void onConnection(const muduo::net::TcpConnectionPtr &conn);
        void onMessage(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buf, muduo::Timestamp receiveTime);
        void onRequest(const muduo::net::TcpConnectionPtr &, const HttpRequest &);
        void handleRequest(const HttpRequest &req, HttpResponse *resp);

    private:
        muduo::net::InetAddress listenAddr_; // 监听地址
        muduo::net::TcpServer server_;
        muduo::net::EventLoop mainLoop_; // 主循环
        HttpCallback httpCallback_;
        router::Router router_;
        std::unique_ptr<session::SessionManager> sessionManager_;
        middleware::MiddlewareChain middlewareChain_;
        std::unique_ptr<ssl::SslContext> sslCtx_;
        bool useSSL_;

        std::map<muduo::net::TcpConnectionPtr, std::unique_ptr<ssl::SslConnection>> sslConns_;
    };

} // namespace http