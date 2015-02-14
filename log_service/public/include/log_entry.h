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
 *  \date       Jun 26, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef LOG_ENTRY_H_
#define LOG_ENTRY_H_

#include "log_service.h"

struct log_entry {
    int errorCode;
    log_level_t level;
    char *message;
    time_t time;

    char *bundleSymbolicName;
    long bundleId;
};

typedef struct log_entry * log_entry_pt;

celix_status_t logEntry_create(bundle_pt bundle, service_reference_pt reference,
        log_level_t level, char *message, int errorCode,
        log_entry_pt *entry);
celix_status_t logEntry_destroy(log_entry_pt *entry);
celix_status_t logEntry_getBundleSymbolicName(log_entry_pt entry, char **bundleSymbolicName);
celix_status_t logEntry_getBundleId(log_entry_pt entry, long *bundleId);
celix_status_t logEntry_getErrorCode(log_entry_pt entry, int *errorCode);
celix_status_t logEntry_getLevel(log_entry_pt entry, log_level_t *level);
celix_status_t logEntry_getMessage(log_entry_pt entry, char **message);
celix_status_t logEntry_getTime(log_entry_pt entry, time_t *time);

#endif /* LOG_ENTRY_H_ */
