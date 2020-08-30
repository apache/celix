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
 * log_service_impl.c
 *
 *  \date       Jun 22, 2011
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <stdlib.h>

#include "log_service_impl.h"
#include "module.h"
#include "bundle.h"

struct log_service_data {
    log_t *log;
    celix_bundle_t *bundle;
};

celix_status_t logService_create(log_t *log, celix_bundle_t *bundle, log_service_data_t **logger) {
    celix_status_t status = CELIX_SUCCESS;
    *logger = calloc(1, sizeof(struct log_service_data));
    if (*logger == NULL) {
        status = CELIX_ENOMEM;
    } else {
        (*logger)->bundle = bundle;
        (*logger)->log = log;
    }

    return status;
}

celix_status_t logService_destroy(log_service_data_t **logger) {
    celix_status_t status = CELIX_SUCCESS;

    free(*logger);
    logger = NULL;

    return status;
}

celix_status_t logService_log(log_service_data_t *logger, log_level_t level, char * message) {
    return logService_logSr(logger, NULL, level, message);
}

celix_status_t logService_logSr(log_service_data_t *logger, service_reference_pt reference, log_level_t level, char * message) {
    celix_status_t status;
    log_entry_t *entry = NULL;
    celix_bundle_t *bundle = logger->bundle;
    bundle_archive_pt archive = NULL;
    module_pt module = NULL;
    const char *symbolicName = NULL;
    long bundleId = -1;

    if (reference != NULL) {
        serviceReference_getBundle(reference, &bundle);
    }

    status = bundle_getArchive(bundle, &archive);

    if (status == CELIX_SUCCESS) {
        status = bundleArchive_getId(archive, &bundleId);
    }

    if (status == CELIX_SUCCESS) {
        status = bundle_getCurrentModule(bundle, &module);

        if (status == CELIX_SUCCESS) {
            status = module_getSymbolicName(module, &symbolicName);
        }
    }

    if(status == CELIX_SUCCESS && symbolicName != NULL && message != NULL){
        status = logEntry_create(bundleId, symbolicName, reference, level, message, 0, &entry);
        log_addEntry(logger->log, entry);
    }

    return status;
}
