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
 * log.c
 *
 *  Created on: Jun 22, 2011
 *      Author: alexander
 */

#include "log_service_impl.h"
#include "module.h"
#include "bundle.h"

struct log_service_data {
    log_t log;
    BUNDLE bundle;
    apr_pool_t *pool;
};

celix_status_t logService_create(log_t log, BUNDLE bundle, apr_pool_t *pool, log_service_data_t *logger) {
    celix_status_t status = CELIX_SUCCESS;
    *logger = apr_palloc(pool, sizeof(struct log_service_data));
    if (*logger == NULL) {
        status = CELIX_ENOMEM;
    } else {
        (*logger)->bundle = bundle;
        (*logger)->log = log;
        (*logger)->pool = pool;
    }

    return status;
}

celix_status_t logService_log(log_service_data_t logger, log_level_t level, char * message) {
    printf("Logging: %s from %s\n", message, module_getSymbolicName(bundle_getCurrentModule(logger->bundle)));
    return logService_logSr(logger, NULL, level, message);
}

celix_status_t logService_logSr(log_service_data_t logger, SERVICE_REFERENCE reference, log_level_t level, char * message) {
    log_entry_t entry = NULL;
    BUNDLE bundle = logger->bundle;
    if (reference != NULL) {
        bundle = reference->bundle;
    }
    logEntry_create(bundle, reference, level, message, 0, logger->pool, &entry);
    log_addEntry(logger->log, entry);

    return CELIX_SUCCESS;
}
