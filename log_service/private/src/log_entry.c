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
 * log_entry.c
 *
 *  \date       Jun 26, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stddef.h>

#include "log_entry.h"

celix_status_t logEntry_create(BUNDLE bundle, SERVICE_REFERENCE reference,
        log_level_t level, char *message, int errorCode,
        apr_pool_t *pool, log_entry_t *entry) {
    celix_status_t status = CELIX_SUCCESS;

    *entry = apr_palloc(pool, sizeof(**entry));
    if (entry == NULL) {
        status = CELIX_ENOMEM;
    } else {
        (*entry)->bundle = bundle;
        (*entry)->reference = reference;
        (*entry)->level = level;
        (*entry)->message = message;
        (*entry)->errorCode = errorCode;
        (*entry)->time = time(NULL);
    }

    return status;
}

celix_status_t logEntry_getBundle(log_entry_t entry, BUNDLE *bundle) {
    *bundle = entry->bundle;
    return CELIX_SUCCESS;
}

celix_status_t logEntry_getErrorCode(log_entry_t entry, int *errorCode) {
    *errorCode = entry->errorCode;
    return CELIX_SUCCESS;
}

celix_status_t logEntry_getLevel(log_entry_t entry, log_level_t *level) {
    *level = entry->level;
    return CELIX_SUCCESS;
}

celix_status_t logEntry_getMessage(log_entry_t entry, char **message) {
    *message = entry->message;
    return CELIX_SUCCESS;
}

celix_status_t logEntry_getServiceReference(log_entry_t entry, SERVICE_REFERENCE *reference) {
    *reference = entry->reference;
    return CELIX_SUCCESS;
}

celix_status_t logEntry_getTime(log_entry_t entry, time_t *time) {
    *time = entry->time;
    return CELIX_SUCCESS;
}
