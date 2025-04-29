#include "../../../include/middleware/cors/CorsMiddleware.h"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <muduo/base/Logging.h>

namespace http
{
    namespace middleware
    {

        CorsMiddleware::CorsMiddleware(const CorsConfig &config) : config_(config) {}

        void CorsMiddleware::before(HttpRequest &request)
        {
            LOG_DEBUG << "CorsMiddleware::before - Processing request";

            if (request.method() == HttpRequest::Method::kOptions)
            {
                LOG_INFO << "Processing CORS preflight request";
                HttpResponse response;
                handlePreflightRequest(request, response);
                throw response;
            }
        }

        void CorsMiddleware::after(HttpResponse &response)
        {
            LOG_DEBUG << "CorsMiddleware::after - Processing response";

            // 直接添加CORS头，简化处理逻辑
            if (!config_.allowedOrigins.empty())
            {
                // 如果允许所有源
                if (std::find(config_.allowedOrigins.begin(), config_.allowedOrigins.end(), "*") != config_.allowedOrigins.end())
                {
                    addCorsHeader(response, "*");
                }
                else
                {
                    // 添加第一个允许的源
                    // HTTP 规范规定：Access-Control-Allow-Origin 响应头最多只能设置一个具体源 或者 所有源
                    addCorsHeader(response, config_.allowedOrigins[0]);
                }
            }
        }

        bool CorsMiddleware::isOriginAllowed(const std::string &origin) const
        {
            return config_.allowedOrigins.empty() || // 这是个默认放行全部的逻辑，空时允许所有
                   std::find(config_.allowedOrigins.begin(), config_.allowedOrigins.end(), "*") != config_.allowedOrigins.end() ||
                   std::find(config_.allowedOrigins.begin(), config_.allowedOrigins.end(), origin) != config_.allowedOrigins.end();
        }

        void CorsMiddleware::handlePreflightRequest(const HttpRequest &request, HttpResponse &response)
        {
            const std::string &origin = request.getHeader("Origin");

            if (!isOriginAllowed(origin))
            {
                LOG_WARN << "Origin not allowed: " << origin;
                response.setStatusCode(HttpResponse::k403Forbidden);
                return;
            }

            addCorsHeader(response, origin);
            response.setStatusCode(HttpResponse::k204NoContent);
            LOG_INFO << "Preflight request processed successfully";
        }

        void CorsMiddleware::addCorsHeader(HttpResponse &response, const std::string &origin)
        {
            try
            {
                response.addHeader("Access-Control-Allow-Origin", origin);

                if (config_.allowCredentials)
                {
                    response.addHeader("Access-Control-Allow-Credentials", "true");
                }
                if (!config_.allowedMethods.empty())
                {
                    response.addHeader("Access-Control-Allow-Methods", join(config_.allowedMethods, ", "));
                }

                if (!config_.allowedHeaders.empty())
                {
                    response.addHeader("Access-Control-Allow-Headers", join(config_.allowedMethods, ", "));
                }

                response.addHeader("Access-Control-Allow-Max-Age", std::to_string(config_.maxAge));

                LOG_DEBUG << "CORS headers added sucessfully";
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "Error adding CORS headers: " << e.what();
            }
        }

        // 工具函数，将字符串数组连接成单个字符串
        std::string CorsMiddleware::join(const std::vector<std::string> &strings, const std::string &delimiter)
        {
            std::ostringstream result;
            for (size_t i = 0; i < strings.size(); i++)
            {
                if (i > 0)
                    result << delimiter;
                result << strings[i];
            }
            return result.str();
        }

    } // namespace http
} // namespace middleware
