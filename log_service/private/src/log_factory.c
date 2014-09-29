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
 * log_factory.c
 *
 *  \date       Jun 26, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stddef.h>

#include "service_factory.h"
#include "log_factory.h"
#include "log_service_impl.h"

struct log_service_factory {
    apr_pool_t *pool;
    log_pt log;
};

celix_status_t logFactory_create(apr_pool_t *pool, log_pt log, service_factory_pt *factory) {
    celix_status_t status = CELIX_SUCCESS;

    *factory = apr_palloc(pool, sizeof(**factory));
    if (*factory == NULL) {
        status = CELIX_ENOMEM;
    } else {
        log_service_factory_pt factoryData = apr_palloc(pool, sizeof(*factoryData));
        if (factoryData == NULL) {
            status = CELIX_ENOMEM;
        } else {
            factoryData->pool = pool;
            factoryData->log = log;

            (*factory)->factory = factoryData;
            (*factory)->getService = logFactory_getService;
            (*factory)->ungetService = logFactory_ungetService;
        }
    }

    return CELIX_SUCCESS;
}

celix_status_t logFactory_getService(void *factory, bundle_pt bundle, service_registration_pt registration, void **service) {
    log_service_factory_pt log_factory = ((service_factory_pt) factory)->factory;
    log_service_pt log_service = NULL;
    log_service_data_pt log_service_data = NULL;

    logService_create(log_factory->log, bundle, log_factory->pool, &log_service_data);

    log_service = apr_palloc(log_factory->pool, sizeof(*log_service));
    log_service->logger = log_service_data;
    log_service->log = logService_log;
    log_service->logSr = logService_logSr;

    (*service) = log_service;

    return CELIX_SUCCESS;
}

celix_status_t logFactory_ungetService(void *factory, bundle_pt bundle, service_registration_pt registration) {
    return CELIX_SUCCESS;
}
