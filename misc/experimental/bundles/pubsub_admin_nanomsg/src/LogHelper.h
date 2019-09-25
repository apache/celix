/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#pragma once
#include <sstream>
#include "log_helper.h"
#include <mutex>
namespace celix {
    namespace pubsub {
        namespace nanomsg {
            /*
             * Not that the loghelper is created in the firs log-call. This is because when a log-helper is started
             * during registration of a service with a service-factory a dead-lock can occur
             * This prevents it.
             */
            class LogHelper {
            public:
                LogHelper(bundle_context_t *_ctx, const std::string& _componentName ) :  ctx{_ctx}, helperCreated{true}, componentName{_componentName}{
                }

                LogHelper(const LogHelper& ) = delete;
                LogHelper& operator=(const LogHelper&) = delete;


                ~LogHelper() {
                    if (helperCreated && _logHelper) {
                        logHelper_stop(_logHelper);
                        logHelper_destroy(&_logHelper);
                    }
                    std::cerr << "Destroyed loghelper for " << componentName << std::endl;
                }
                template<typename... Args>
                void ERROR(Args... args) {
                    auto ss = LOG_STREAM(args...);
                    log_string(OSGI_LOGSERVICE_ERROR, ss.str());
                }

                template<typename... Args>
                void WARN(Args... args) {
                    auto ss = LOG_STREAM(args...);
                    log_string(OSGI_LOGSERVICE_WARNING, ss.str());
                }

                template<typename... Args>
                void INFO(Args... args)  {
                    auto ss = LOG_STREAM(args...);
                    log_string(OSGI_LOGSERVICE_INFO, ss.str());
                }

                template<typename... Args>
                void DBG(Args... args)  {
                    auto ss = LOG_STREAM(args...);
                    log_string(OSGI_LOGSERVICE_DEBUG, ss.str());
                }

            private:
                bundle_context_t *ctx;
                bool helperCreated{false};
                log_helper_t *_logHelper{};
                std::string componentName{};
                template<typename T>
                std::stringstream LOG_STREAM(T first) const {
                    std::stringstream ss;
                    ss << first;
                    return ss;
                }

                template<typename T, typename... Args>
                std::stringstream LOG_STREAM(T first, Args... args) const {
                    std::stringstream ss;
                    ss << "[" << componentName << "] " << first << LOG_STREAM(args...).str();
                    return ss;
                }

                void log_string(log_level_t level, const std::string& msg) {
                    if (_logHelper == nullptr) {
                        helperCreated = true;
                        logHelper_create(ctx, &_logHelper);
                        logHelper_start(_logHelper);
                    }
                    logHelper_log(_logHelper, level, msg.c_str());
                }
            };

        }
    }
}