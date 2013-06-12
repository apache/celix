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
 *  \date       Jun 22, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>

#include "log_service_impl.h"
#include "module.h"
#include "bundle.h"

struct log_service_data {
    log_pt log;
    bundle_pt bundle;
    apr_pool_t *pool;
};

celix_status_t logService_create(log_pt log, bundle_pt bundle, apr_pool_t *pool, log_service_data_pt *logger) {
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

celix_status_t logService_log(log_service_data_pt logger, log_level_t level, char * message) {
    return logService_logSr(logger, NULL, level, message);
}

celix_status_t logService_logSr(log_service_data_pt logger, service_reference_pt reference, log_level_t level, char * message) {
    log_entry_pt entry = NULL;
    bundle_pt bundle = logger->bundle;
    if (reference != NULL) {
    	serviceReference_getBundle(reference, &bundle);
    }
    logEntry_create(bundle, reference, level, message, 0, logger->pool, &entry);
    log_addEntry(logger->log, entry);

    return CELIX_SUCCESS;
}
