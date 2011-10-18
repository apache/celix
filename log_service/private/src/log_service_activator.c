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
 * logService_activator.c
 *
 *  Created on: Jun 25, 2011
 *      Author: alexander
 */
#include <apr_general.h>
#include <stdlib.h>

#include "bundle_activator.h"
#include "log_service_impl.h"
#include "service_factory.h"
#include "log_factory.h"
#include "log.h"
#include "log_reader_service_impl.h"
#include "service_registration.h"

struct logActivator {
    BUNDLE_CONTEXT bundleContext;
    SERVICE_REGISTRATION logServiceFactoryReg;
    SERVICE_REGISTRATION logReaderServiceReg;

};

celix_status_t bundleActivator_create(BUNDLE_CONTEXT context, void **userData) {
    celix_status_t status = CELIX_SUCCESS;
    apr_pool_t *mp = NULL;

    bundleContext_getMemoryPool(context, &mp);
    struct logActivator * activator = apr_palloc(mp, sizeof(struct logActivator));

    activator->bundleContext = context;

    if (activator == NULL) {
        status = CELIX_ENOMEM;
    } else {
        *userData = activator;
    }

    return status;
}

celix_status_t bundleActivator_start(void * userData, BUNDLE_CONTEXT context) {
    struct logActivator * activator = (struct logActivator *) userData;
    celix_status_t status = CELIX_SUCCESS;
    apr_pool_t *mp = NULL;
    service_factory_t factory = NULL;
    log_t logger = NULL;

    log_reader_data_t reader = NULL;
    log_reader_service_t reader_service = NULL;

    bundleContext_getMemoryPool(context, &mp);

    log_create(mp, &logger);
    logFactory_create(mp, logger, &factory);

    bundleContext_registerServiceFactory(context, (char *) LOG_SERVICE_NAME, factory, NULL, &activator->logServiceFactoryReg);

    logReaderService_create(logger, mp, &reader);

    reader_service = apr_palloc(mp, sizeof(*reader_service));
    reader_service->reader = reader;
    reader_service->getLog = logReaderService_getLog;
    reader_service->addLogListener = logReaderService_addLogListener;
    reader_service->removeLogListener = logReaderService_removeLogListener;
    reader_service->removeAllLogListener = logReaderService_removeAllLogListener;

    bundleContext_registerService(context, (char *) LOG_READER_SERVICE_NAME, reader_service, NULL, &activator->logReaderServiceReg);

    return status;
}

celix_status_t bundleActivator_stop(void * userData, BUNDLE_CONTEXT context) {
	struct logActivator * activator = (struct logActivator *) userData;

	serviceRegistration_unregister(activator->logReaderServiceReg);
	activator->logReaderServiceReg = NULL;
	serviceRegistration_unregister(activator->logServiceFactoryReg);
	activator->logServiceFactoryReg = NULL;

    return CELIX_SUCCESS;
}

celix_status_t bundleActivator_destroy(void * userData, BUNDLE_CONTEXT context) {
    return CELIX_SUCCESS;
}
