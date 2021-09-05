/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */

#pragma once

#include <memory>
#include "celix_log_helper.h"
#include "celix/BundleContext.h"

namespace celix {

    class LogHelper {
    public:
        LogHelper(const std::shared_ptr<celix::BundleContext>& ctx, const std::string& logName) : logHelper(createHelper(ctx, logName)) {}

        /**
         * @brief Logs to celix_logHelper_log using the CELIX_LOG_LEVEL_TRACE level, printf style
         */
        void trace(const char *format, ...) const {
            va_list args;
            va_start(args, format);
            vlog(CELIX_LOG_LEVEL_TRACE, format, args);
            va_end(args);
        }
        
        /**
         * @brief Logs to celix_logHelper_log using the CELIX_LOG_LEVEL_DEBUG level, printf style
         */
        void debug(const char *format, ...) const {
            va_list args;
            va_start(args, format);
            vlog(CELIX_LOG_LEVEL_DEBUG, format, args);
            va_end(args);
        }
        
        /**
         * @brief Logs to celix_logHelper_log using the CELIX_LOG_LEVEL_INFO level, printf style
         */
        void info(const char *format, ...) const {
            va_list args;
            va_start(args, format);
            vlog(CELIX_LOG_LEVEL_INFO, format, args);
            va_end(args);
        }
        
        /**
         * @brief Logs to celix_logHelper_log using the CELIX_LOG_LEVEL_WARNING level, printf style
         */
        void warning(const char *format, ...) const {
            va_list args;
            va_start(args, format);
            vlog(CELIX_LOG_LEVEL_WARNING, format, args);
            va_end(args);
        }
        
        /**
         * @brief Logs to celix_logHelper_log using the CELIX_LOG_LEVEL_ERROR level, printf style
         */
        void error(const char *format, ...) const {
            va_list args;
            va_start(args, format);
            vlog(CELIX_LOG_LEVEL_ERROR, format, args);
            va_end(args);
        }
        
        /**
         * @brief Logs to celix_logHelper_log using the CELIX_LOG_LEVEL_FATAL level, printf style
         */
        void fatal(const char *format, ...) const {
            va_list args;
            va_start(args, format);
            vlog(CELIX_LOG_LEVEL_FATAL, format, args);
            va_end(args);
        }

        /**
         * @brief Logs a message to the log_helper using a format string and a va_list argument (vprintf style).
         *
         * Silently ignores log level CELIX_LOG_LEVEL_DISABLED.
         */
        void vlog(celix_log_level_e level, const char *format, va_list formatArgs) const {
            celix_logHelper_vlog(logHelper.get(), level, format, formatArgs);
        }

        /**
         * @brief nr of times a helper log function has been called.
         */
        std::size_t count() const {
            return celix_logHelper_logCount(logHelper.get());
        }

    private:
        std::shared_ptr<celix_log_helper> createHelper(const std::shared_ptr<celix::BundleContext>& ctx, const std::string& logName) {
            return std::shared_ptr<celix_log_helper>{
                celix_logHelper_create(ctx->getCBundleContext(), logName.c_str()),
                [](celix_log_helper* lh) {celix_logHelper_destroy(lh);}
            };
        }

        const std::shared_ptr<celix_log_helper> logHelper;
    };


}
