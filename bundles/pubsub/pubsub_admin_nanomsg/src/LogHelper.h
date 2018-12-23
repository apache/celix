/*
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#pragma once
#include <sstream>
#include "log_helper.h"

namespace celix {
    namespace pubsub {
        namespace nanomsg {

//            template<typename... Args>
//            void L_DEBUG(log_helper_t *logHelper, Args... args) {
//                std::stringstream ss = LOG_STREAM(args...);
//                logHelper_log(logHelper, OSGI_LOGSERVICE_DEBUG, ss.str().c_str());
//            }
//
//            template<typename... Args>
//            void L_INFO(log_helper_t *logHelper, Args... args) {
//                auto ss = LOG_STREAM(args...);
//                logHelper_log(logHelper, OSGI_LOGSERVICE_INFO, ss.str().c_str());
//            }
//
//            template<typename... Args>
//            void L_WARN(log_helper_t *logHelper, Args... args) {
//                auto ss = LOG_STREAM(args...);
//                logHelper_log(logHelper, OSGI_LOGSERVICE_WARNING, ss.str().c_str());
//            }
//
//            template<typename... Args>
//            void L_ERROR(log_helper_t *logHelper, Args... args) {
//                auto ss = LOG_STREAM(args...);
//                logHelper_log((log_helper_pt) logHelper, OSGI_LOGSERVICE_ERROR, ss.str().c_str());
//            }

            class LogHelper {
            public:
                LogHelper(log_helper_t *lh) : _logHelper{lh} {
                }

                LogHelper(const LogHelper& ) = default;
                LogHelper& operator=(const LogHelper&) = default;

                LogHelper(bundle_context_pt ctx) :  helperCreated{true} {
                    logHelper_create(ctx, &_logHelper);
                    logHelper_start(_logHelper);
                }

                ~LogHelper() {
                    if (helperCreated) {
                        logHelper_stop(_logHelper);
                        logHelper_destroy(&_logHelper);
                    }
                }
                template<typename... Args>
                void ERROR(Args... args) {
                    auto ss = LOG_STREAM(args...);
                    logHelper_log(_logHelper, OSGI_LOGSERVICE_ERROR, ss.str().c_str());
                }

                template<typename... Args>
                void WARN(Args... args) {
                    auto ss = LOG_STREAM(args...);
                    logHelper_log(_logHelper, OSGI_LOGSERVICE_WARNING, ss.str().c_str());
                }

                template<typename... Args>
                void INFO(Args... args) {
                    auto ss = LOG_STREAM(args...);
                    logHelper_log(_logHelper, OSGI_LOGSERVICE_INFO, ss.str().c_str());
                }

                template<typename... Args>
                void DBG(Args... args) {
                    auto ss = LOG_STREAM(args...);
                    logHelper_log(_logHelper, OSGI_LOGSERVICE_DEBUG, ss.str().c_str());
                }

            private:
                bool helperCreated{false};
                log_helper_t *_logHelper{};

                template<typename T>
                std::stringstream LOG_STREAM(T first) {
                    std::stringstream ss;
                    ss << first;
                    return ss;
                }

                template<typename T, typename... Args>
                std::stringstream LOG_STREAM(T first, Args... args) {
                    std::stringstream ss;
                    ss << first << LOG_STREAM(args...).str();
                    return ss;
                }

            };

        }
    }
}