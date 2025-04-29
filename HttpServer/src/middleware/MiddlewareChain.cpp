#include "../../include/middleware/MiddlewareChain.h"
#include <muduo/base/Logging.h>

namespace http{
    namespace middleware{

        void MiddlewareChain::addMiddleware(std::shared_ptr<Middleware> middleware){
            middlewares_.push_back(middleware);
        }

        void MiddlewareChain::processBefore(HttpRequest &request){
            for(auto &middleware:middlewares_){
                middleware->before(request);
            }
        }

        void MiddlewareChain::processAfter(HttpResponse &response){
            try
            {
                //栈式执行模式（洋葱模型），在执行after时，对应before的顺序先进后出，所以反着找
                for(auto it = middlewares_.rbegin();it != middlewares_.rend();it++){
                    //解引用迭代器，直接取出迭代器指向元素
                    if(*it){
                        (*it)->after(response);
                    }
                }
            }
            catch(const std::exception& e)
            {
                LOG_ERROR << "Error in middleware after processing: " << e.what();
            }
            
        }

    }//namespace middleware
}//namespace http