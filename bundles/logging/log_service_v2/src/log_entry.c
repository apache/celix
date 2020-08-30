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
/**
 * log_entry.c
 *
 *  \date       Jun 26, 2011
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "celix_errno.h"
#include "log_service.h"
#include "log_entry.h"

celix_status_t logEntry_create(long bundleId, const char* bundleSymbolicName , service_reference_pt reference,
                               log_level_t level, char *message, int errorCode,
                               log_entry_t **entry) {
    celix_status_t status = CELIX_SUCCESS;

    *entry = malloc(sizeof(**entry));
    if (*entry == NULL) {
        status = CELIX_ENOMEM;
    } else {
        (*entry)->level = level;
        (*entry)->message = strdup(message);
        (*entry)->errorCode = errorCode;
        (*entry)->time = time(NULL);

        (*entry)->bundleSymbolicName = strdup(bundleSymbolicName);
        (*entry)->bundleId = bundleId;
    }

    return status;
}

celix_status_t logEntry_destroy(log_entry_t **entry) {
    if (*entry) {
        free((*entry)->bundleSymbolicName);
        free((*entry)->message);
        free(*entry);
        *entry = NULL;
    }
    return CELIX_SUCCESS;
}

celix_status_t logEntry_getBundleSymbolicName(log_entry_t *entry, const char** bundleSymbolicName) {
    *bundleSymbolicName = entry->bundleSymbolicName;
    return CELIX_SUCCESS;
}

celix_status_t logEntry_getBundleId(log_entry_t *entry, long *bundleId) {
    *bundleId = entry->bundleId;
    return CELIX_SUCCESS;
}

celix_status_t logEntry_getErrorCode(log_entry_t *entry, int *errorCode) {
    *errorCode = entry->errorCode;
    return CELIX_SUCCESS;
}

celix_status_t logEntry_getLevel(log_entry_t *entry, log_level_t *level) {
    *level = entry->level;
    return CELIX_SUCCESS;
}

celix_status_t logEntry_getMessage(log_entry_t *entry, const char **message) {
    *message = entry->message;
    return CELIX_SUCCESS;
}

celix_status_t logEntry_getTime(log_entry_t *entry, time_t *time) {
    *time = entry->time;
    return CELIX_SUCCESS;
}
