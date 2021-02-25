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
 * log_service_activator.c
 *
 *  \date       Jun 25, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdlib.h>
#include <string.h>
#include "celix_constants.h"

#include "bundle_activator.h"
#include "log_service_impl.h"
#include "service_factory.h"
#include "log_factory.h"
#include "log.h"
#include "log_reader_service_impl.h"
#include "service_registration.h"

#define DEFAULT_MAX_SIZE 100
#define DEFAULT_STORE_DEBUG false

#define MAX_SIZE_PROPERTY "CELIX_LOG_MAX_SIZE"
#define STORE_DEBUG_PROPERTY "CELIX_LOG_STORE_DEBUG"

struct logActivator {
    celix_bundle_context_t *bundleContext;
    service_registration_t *logServiceFactoryReg;
    service_registration_t *logReaderServiceReg;

    bundle_listener_t *bundleListener;
    framework_listener_t *frameworkListener;

    log_t *logger;
    service_factory_pt factory;
    log_reader_data_t *reader;
    log_reader_service_t *reader_service;
};

static celix_status_t bundleActivator_getMaxSize(struct logActivator *activator, int *max_size);
static celix_status_t bundleActivator_getStoreDebug(struct logActivator *activator, bool *store_debug);

celix_status_t bundleActivator_create(celix_bundle_context_t *context, void **userData) {
    celix_status_t status = CELIX_SUCCESS;
	struct logActivator * activator = NULL;

    activator = (struct logActivator *) calloc(1, sizeof(struct logActivator));

    if (activator == NULL) {
        status = CELIX_ENOMEM;
    } else {
		activator->bundleContext = context;
		activator->logServiceFactoryReg = NULL;
		activator->logReaderServiceReg = NULL;

		activator->logger = NULL;
		activator->factory = NULL;
		activator->reader = NULL;
		activator->reader_service = NULL;

        *userData = activator;
    }

    return status;
}

celix_status_t bundleActivator_start(void * userData, celix_bundle_context_t *context) {
    struct logActivator * activator = (struct logActivator *) userData;
    celix_status_t status = CELIX_SUCCESS;

    int max_size = 0;
    bool store_debug = false;

    bundleActivator_getMaxSize(activator, &max_size);
    bundleActivator_getStoreDebug(activator, &store_debug);

    log_create(max_size, store_debug, &activator->logger);

    // Add logger as Bundle- and FrameworkEvent listener
    activator->bundleListener = calloc(1, sizeof(*activator->bundleListener));
    activator->bundleListener->handle = activator->logger;
    activator->bundleListener->bundleChanged = log_bundleChanged;
    bundleContext_addBundleListener(context, activator->bundleListener);

    activator->frameworkListener = calloc(1, sizeof(*activator->frameworkListener));
    activator->frameworkListener->handle = activator->logger;
    activator->frameworkListener->frameworkEvent = log_frameworkEvent;
    bundleContext_addFrameworkListener(context, activator->frameworkListener);

    logFactory_create(activator->logger, &activator->factory);

	celix_properties_t *props = celix_properties_create();


	bundleContext_registerServiceFactory(context, (char *) OSGI_LOGSERVICE_NAME, activator->factory, props, &activator->logServiceFactoryReg);

    logReaderService_create(activator->logger, &activator->reader);

    activator->reader_service = calloc(1, sizeof(*activator->reader_service));
    activator->reader_service->reader = activator->reader;
    activator->reader_service->getLog = logReaderService_getLog;
    activator->reader_service->addLogListener = logReaderService_addLogListener;
    activator->reader_service->removeLogListener = logReaderService_removeLogListener;
    activator->reader_service->removeAllLogListener = logReaderService_removeAllLogListener;

	props = celix_properties_create();

    bundleContext_registerService(context, (char *) OSGI_LOGSERVICE_READER_SERVICE_NAME, activator->reader_service, props, &activator->logReaderServiceReg);
    return status;
}

celix_status_t bundleActivator_stop(void * userData, celix_bundle_context_t *context) {
	struct logActivator * activator = (struct logActivator *) userData;

	serviceRegistration_unregister(activator->logReaderServiceReg);
	activator->logReaderServiceReg = NULL;
	serviceRegistration_unregister(activator->logServiceFactoryReg);
	activator->logServiceFactoryReg = NULL;

    logReaderService_destroy(&activator->reader);
	free(activator->reader_service);

	logFactory_destroy(&activator->factory);

	bundleContext_removeBundleListener(context, activator->bundleListener);
	bundleContext_removeFrameworkListener(context, activator->frameworkListener);

	free(activator->bundleListener);
	free(activator->frameworkListener);

	log_destroy(activator->logger);

    return CELIX_SUCCESS;
}

celix_status_t bundleActivator_destroy(void * userData, celix_bundle_context_t *context) {
	struct logActivator * activator = (struct logActivator *) userData;

	free(activator);

    return CELIX_SUCCESS;
}

static celix_status_t bundleActivator_getMaxSize(struct logActivator *activator, int *max_size) {
	celix_status_t status = CELIX_SUCCESS;

	const char *max_size_str = NULL;

	*max_size = DEFAULT_MAX_SIZE;

	bundleContext_getProperty(activator->bundleContext, MAX_SIZE_PROPERTY, &max_size_str);
	if (max_size_str) {
		*max_size = atoi(max_size_str);
	}

	return status;
}

static celix_status_t bundleActivator_getStoreDebug(struct logActivator *activator, bool *store_debug) {
	celix_status_t status = CELIX_SUCCESS;

	const char *store_debug_str = NULL;

	*store_debug = DEFAULT_STORE_DEBUG;

	bundleContext_getProperty(activator->bundleContext, STORE_DEBUG_PROPERTY, &store_debug_str);
	if (store_debug_str) {
		if (strcasecmp(store_debug_str, "true") == 0) {
			*store_debug = true;
		} else if (strcasecmp(store_debug_str, "false") == 0) {
			*store_debug = false;
		}
	}

	return status;
}
