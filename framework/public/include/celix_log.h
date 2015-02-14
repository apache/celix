/*
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
/*
 * celix_log.h
 *
 *  \date       Jan 12, 2012
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef CELIX_LOG_H_
#define CELIX_LOG_H_

#include <stdio.h>

#include "celix_errno.h"
#include "framework_exports.h"

enum framework_log_level
{
    OSGI_FRAMEWORK_LOG_ERROR = 0x00000001,
    OSGI_FRAMEWORK_LOG_WARNING = 0x00000002,
    OSGI_FRAMEWORK_LOG_INFO = 0x00000003,
    OSGI_FRAMEWORK_LOG_DEBUG = 0x00000004,
};

typedef enum framework_log_level framework_log_level_t;

typedef struct framework_logger *framework_logger_pt;

extern framework_logger_pt logger;

typedef celix_status_t (*framework_log_function_pt)(framework_log_level_t level, const char *func, const char *file, int line, char *msg);

struct framework_logger {
    framework_log_function_pt logFunction;
};

#define fw_log(logger, level, fmsg, args...) framework_log(logger, level, __func__, __FILE__, __LINE__, fmsg, ## args)
#define fw_logCode(logger, level, code, fmsg, args...) framework_logCode(logger, level, __func__, __FILE__, __LINE__, code, fmsg, ## args)
#define framework_logIfError(logger, status, error, fmsg, args...) \
    if (status != CELIX_SUCCESS) { \
        if (error != NULL) { \
            fw_logCode(logger, OSGI_FRAMEWORK_LOG_ERROR, status, #fmsg";\n Cause: %s", ## args, error); \
        } else { \
            fw_logCode(logger, OSGI_FRAMEWORK_LOG_ERROR, status, #fmsg, ## args); \
        } \
    }

FRAMEWORK_EXPORT celix_status_t frameworkLogger_log(framework_log_level_t level, const char *func, const char *file, int line, char *fmsg);
FRAMEWORK_EXPORT void framework_log(framework_logger_pt logger, framework_log_level_t level, const char *func, const char *file, int line, char *fmsg, ...);
FRAMEWORK_EXPORT void framework_logCode(framework_logger_pt logger, framework_log_level_t level, const char *func, const char *file, int line, celix_status_t code, char *fmsg, ...);

#endif /* CELIX_LOG_H_ */
