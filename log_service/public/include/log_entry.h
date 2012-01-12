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
 * log_entry.h
 *
 *  Created on: Jun 26, 2011
 *      Author: alexander
 */

#ifndef LOG_ENTRY_H_
#define LOG_ENTRY_H_

#include "log_service.h"

struct log_entry {
    BUNDLE bundle;
    int errorCode;
    log_level_t level;
    char *message;
    SERVICE_REFERENCE reference;
    time_t time;
};

typedef struct log_entry * log_entry_t;

celix_status_t logEntry_create(BUNDLE bundle, SERVICE_REFERENCE reference,
        log_level_t level, char *message, int errorCode,
        apr_pool_t *pool, log_entry_t *entry);
celix_status_t logEntry_getBundle(log_entry_t entry, BUNDLE *bundle);
celix_status_t logEntry_getErrorCode(log_entry_t entry, int *errorCode);
celix_status_t logEntry_getLevel(log_entry_t entry, log_level_t *level);
celix_status_t logEntry_getMessage(log_entry_t entry, char **message);
celix_status_t logEntry_getServiceReference(log_entry_t entry, SERVICE_REFERENCE *reference);
celix_status_t logEntry_getTime(log_entry_t entry, time_t *time);

#endif /* LOG_ENTRY_H_ */
