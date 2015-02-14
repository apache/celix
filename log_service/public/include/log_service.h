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
/*
 * log_service.h
 *
 *  \date       Jun 22, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef LOG_SERVICE_H_
#define LOG_SERVICE_H_

#include "celix_errno.h"
#include "service_reference.h"

static const char * const OSGI_LOGSERVICE_NAME = "log_service";

typedef struct log_service_data *log_service_data_pt;

enum log_level
{
    OSGI_LOGSERVICE_ERROR = 0x00000001,
    OSGI_LOGSERVICE_WARNING = 0x00000002,
    OSGI_LOGSERVICE_INFO = 0x00000003,
    OSGI_LOGSERVICE_DEBUG = 0x00000004,
};

typedef enum log_level log_level_t;

struct log_service {
    log_service_data_pt logger;
    celix_status_t (*log)(log_service_data_pt logger, log_level_t level, char * message);
    celix_status_t (*logSr)(log_service_data_pt logger, service_reference_pt reference, log_level_t level, char * message);
};

typedef struct log_service *log_service_pt;



#endif /* LOG_SERVICE_H_ */
