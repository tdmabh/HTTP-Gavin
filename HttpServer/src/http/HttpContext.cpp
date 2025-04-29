// 解析报文关键信息，封装到HttpRequest中
#include "../../include/http/HttpContext.h"

using namespace muduo;
using namespace muduo::net;

namespace http
{

    bool HttpContext::parseRequest(Buffer *buf, Timestamp receiveTime)
    {
        bool ok = true;
        bool hasMore = true;
        while (hasMore)
        {
            if (state_ == kExpectRequestLine)
            {
                const char *crlf = buf->findCRLF();
                if (crlf)
                {
                    ok = processRequestLine(buf->peek(), crlf);
                    if (ok)
                    {
                        request_.setReceiveTime(receiveTime);
                        buf->retrieveUntil(crlf + 2);
                        state_ = kExpectHeaders;
                    }
                    else
                    {
                        hasMore = false;
                    }
                }
                else
                {
                    hasMore = false;
                }
            }
            else if (state_ == kExpectHeaders)
            {
                const char *crlf = buf->findCRLF();
                if (crlf)
                {
                    const char *colon = std::find(buf->peek(), crlf, ':');
                    if (colon < crlf)
                    {
                        request_.addHeader(buf->peek(), colon, crlf);
                    }
                    else if (buf->peek() == crlf)
                    { // 判断是否为空行
                        // 如果是空行，说明header已经结束了，下面该请求体了
                        // GET/HEAD/DELETE等是没有请求体的，POST/PUT有
                        if (request_.method() == HttpRequest::kPost || request_.method() == HttpRequest::kPut)
                        {
                            std::string contentLength = request_.getHeader("Content-Length");
                            if (!contentLength.empty())
                            {
                                request_.setContentLength(std::stoi(contentLength));
                                if (request_.contentLength() > 0)
                                {
                                    state_ = kExpectBody;
                                }
                                else
                                {
                                    state_ = kGotAll;
                                    hasMore = false;
                                }
                            }
                            else
                            {
                                // POST/PUT方法如果没有Content-Length，是HTTP语法错误
                                ok = false;
                                hasMore = false;
                            }
                        }
                        else
                        { // GET/HEAD/DELETE等是没有请求体的
                            state_ = kGotAll;
                            hasMore = false;
                        }
                    }
                    else
                    {
                        ok = false; // 没有空行，Header 格式错误
                        hasMore = false;
                    }

                    buf->retrieveUntil(crlf + 2); // 清除这一行，继续读下一行，不清除会死循环
                }
                else
                {
                    hasMore = false;
                }
            }
            else if (state_ == kExpectBody)
            {
                // 检查缓冲区中是否有足够的数据
                if (buf->readableBytes() < request_.contentLength())
                {
                    hasMore = false;
                    return true;
                }
                // 只读取Content-Length长度的数据
                std::string body(buf->peek(), buf->peek() + request_.contentLength());
                request_.setBody(body);

                buf->retrieve(request_.contentLength());

                state_ = kGotAll;
                hasMore = false;
            }
        }
        return ok;
    }

    // 解析请求行(第一行)，if第一个分支的处理逻辑
    // eg: GET /search?q=chatgpt&lang=zh HTTP/1.1
    bool HttpContext::processRequestLine(const char *begin, const char *end)
    {
        bool succeed = false;
        const char *start = begin;
        const char *space = std::find(start, end, ' ');
        if (space != end && request_.setMethod(start, space))
        {
            start = space + 1;
            space = std::find(start, end, ' ');
            if (space != end)
            {
                const char *argumentStart = std::find(start, space, '?');
                if (argumentStart != space) // 有？请求带参数
                {
                    request_.setPath(start, argumentStart); // 注意这些返回值边界
                    request_.setQueryParameters(argumentStart + 1, space);
                }
                else // 没有问号，请求不带参数
                {
                    request_.setPath(start, space);
                }

                // 解析Http版本
                start = space + 1;
                succeed = ((end - start == 8) && std::equal(start, end - 1, "HTTP/1."));
                if (succeed)
                {
                    if (*(end - 1) == '1')
                    {
                        request_.setVersion("HTTP/1.1");
                    }
                    else if (*(end - 1) == '0')
                    {
                        request_.setVersion("HTTP/1.0");
                    }
                    else
                    {
                        succeed = false;
                    }
                }
            }
        }
        return succeed;
    }

} // namespace http