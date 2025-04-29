// 配置跨域响应的响应头，cors组件完成的功能就是允许或者拒绝浏览器的跨域请求
#pragma once

#include <string>
#include <vector>

namespace http
{
    namespace middleware
    {
        struct CorsConfig
        {
            std::vector<std::string> allowedOrigins;
            std::vector<std::string> allowedMethods;
            std::vector<std::string> allowedHeaders;
            bool allowCredentials = false;
            int maxAge = 3600;

            static CorsConfig defaultConfig()
            {
                CorsConfig config;
                config.allowedOrigins = {"*"};
                config.allowedMethods = {"GET", "PUT", "POST", "DELETE", "OPTIONS"};
                config.allowedHeaders = {"Content-Type", "Authorization"};
                return config;
            }
        };

    } // namespace middleware
} // namespace http