//封装用户会话，保存会话数据和维护会话状态
#pragma once 

#include <memory>
#include <string>
#include <unordered_map>
#include <chrono>

namespace http{
    namespace session{

        class SessionManager;
        
        class Session: public std::enable_shared_from_this<Session>{
        public:
            Session(const std::string& sessionId,SessionManager* sessionManager,int maxAge = 3600);//默认3600秒过期

            const std::string& getId() const{
                return sessionId_;
            }

            bool isExpired() const;
            void refresh();
            
            void setManager(SessionManager* sessionManager){
                sessionManager_ = sessionManager;
            }

            SessionManager* getManager() const {
                return sessionManager_;
            }

            //数据存取
            void setValue(const std::string &key,const std::string &value);
            std::string getValue(const std::string &key) const;
            void remove(const std::string &key);
            void clear();

        private:
        std::string                                     sessionId_;
        std::unordered_map<std::string,std::string>     data_;
        std::chrono::system_clock::time_point           expiryTime_;
        int                                             maxAge_;
        SessionManager*                                 sessionManager_;
        };


    }//namespace http
}// namespace http