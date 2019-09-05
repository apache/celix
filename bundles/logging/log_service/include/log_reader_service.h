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

#ifndef LOG_READER_SERVICE_H_
#define LOG_READER_SERVICE_H_


#include "celix_errno.h"
#include "linked_list.h"
#include "log_listener.h"

static const char * const OSGI_LOGSERVICE_READER_SERVICE_NAME = "log_reader_service";

typedef struct log_reader_data log_reader_data_t;

struct log_reader_service {
    log_reader_data_t *reader;
    celix_status_t (*getLog)(log_reader_data_t *reader, linked_list_pt *list);
    celix_status_t (*addLogListener)(log_reader_data_t *reader, log_listener_t *listener);
    celix_status_t (*removeLogListener)(log_reader_data_t *reader, log_listener_t *listener);
    celix_status_t (*removeAllLogListener)(log_reader_data_t *reader);
};

typedef struct log_reader_service log_reader_service_t;
typedef struct log_reader_service *log_reader_service_pt CELIX_DEPRECATED_ATTR;

#endif /* LOG_READER_SERVICE_H_ */
