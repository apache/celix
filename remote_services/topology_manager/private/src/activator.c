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
 * activator.c
 *
 *  \date       Sep 29, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "constants.h"
#include "bundle_activator.h"
#include "service_tracker.h"
#include "service_registration.h"

#include "topology_manager.h"
#include "endpoint_listener.h"
#include "remote_constants.h"
#include "listener_hook_service.h"
#include "log_service.h"
#include "log_helper.h"
#include "scope.h"
#include "tm_scope.h"
#include "topology_manager.h"

struct activator {
	bundle_context_pt context;

	topology_manager_pt manager;

	service_tracker_pt endpointListenerTracker;
	service_tracker_pt remoteServiceAdminTracker;
	service_listener_pt serviceListener;

	endpoint_listener_pt endpointListener;
	service_registration_pt endpointListenerService;

	listener_hook_service_pt hookService;
	service_registration_pt hook;

	tm_scope_service_pt	scopeService;
	service_registration_pt scopeReg;

	log_helper_pt loghelper;
};


static celix_status_t bundleActivator_createEPLTracker(struct activator *activator, service_tracker_pt *tracker);
static celix_status_t bundleActivator_createRSATracker(struct activator *activator, service_tracker_pt *tracker);
static celix_status_t bundleActivator_createServiceListener(struct activator *activator, service_listener_pt *listener);

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = NULL;
	void *scope;

	activator = calloc(1, sizeof(struct activator));

	if (!activator) {
		return CELIX_ENOMEM;
	}

	activator->context = context;
	activator->endpointListenerService = NULL;
	activator->endpointListenerTracker = NULL;
	activator->hook = NULL;
	activator->manager = NULL;
	activator->remoteServiceAdminTracker = NULL;
	activator->serviceListener = NULL;
	activator->scopeService = calloc(1, sizeof(*(activator->scopeService)));
	if (activator->scopeService == NULL)
	{
		free(activator);
		return CELIX_ENOMEM;
	}

	activator->scopeService->addExportScope = tm_addExportScope;
	activator->scopeService->removeExportScope = tm_removeExportScope;
	activator->scopeService->addImportScope = tm_addImportScope;
	activator->scopeService->removeImportScope = tm_removeImportScope;
	activator->scopeReg = NULL; // explicitly needed, otherwise exception

	logHelper_create(context, &activator->loghelper);
	logHelper_start(activator->loghelper);

	status = topologyManager_create(context, activator->loghelper, &activator->manager, &scope);
	activator->scopeService->handle = scope;

	if (status == CELIX_SUCCESS) {
		status = bundleActivator_createEPLTracker(activator, &activator->endpointListenerTracker);
		if (status == CELIX_SUCCESS) {
			status = bundleActivator_createRSATracker(activator, &activator->remoteServiceAdminTracker);
			if (status == CELIX_SUCCESS) {
				status = bundleActivator_createServiceListener(activator, &activator->serviceListener);
				if (status == CELIX_SUCCESS) {
					*userData = activator;
				}
			}
		}
	}

	if(status != CELIX_SUCCESS){
		bundleActivator_destroy(activator,context);
	}

	return status;
}

static celix_status_t bundleActivator_createEPLTracker(struct activator *activator, service_tracker_pt *tracker) {
	celix_status_t status;

	service_tracker_customizer_pt customizer = NULL;

	status = serviceTrackerCustomizer_create(activator->manager, topologyManager_endpointListenerAdding, topologyManager_endpointListenerAdded, topologyManager_endpointListenerModified,
			topologyManager_endpointListenerRemoved, &customizer);

	if (status == CELIX_SUCCESS) {
		status = serviceTracker_create(activator->context, (char *) OSGI_ENDPOINT_LISTENER_SERVICE, customizer, tracker);
	}

	return status;
}

static celix_status_t bundleActivator_createRSATracker(struct activator *activator, service_tracker_pt *tracker) {
	celix_status_t status;

	service_tracker_customizer_pt customizer = NULL;

	status = serviceTrackerCustomizer_create(activator->manager, topologyManager_rsaAdding, topologyManager_rsaAdded, topologyManager_rsaModified, topologyManager_rsaRemoved, &customizer);

	if (status == CELIX_SUCCESS) {
		status = serviceTracker_create(activator->context, OSGI_RSA_REMOTE_SERVICE_ADMIN, customizer, tracker);
	}

	return status;
}

static celix_status_t bundleActivator_createServiceListener(struct activator *activator, service_listener_pt *listener) {
	celix_status_t status = CELIX_SUCCESS;

	*listener = malloc(sizeof(**listener));
	if (!*listener) {
		return CELIX_ENOMEM;
	}

	(*listener)->handle = activator->manager;
	(*listener)->serviceChanged = topologyManager_serviceChanged;

	return status;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
	celix_status_t status;
	struct activator *activator = userData;

	endpoint_listener_pt endpointListener = malloc(sizeof(*endpointListener));
	endpointListener->handle = activator->manager;
	endpointListener->endpointAdded = topologyManager_addImportedService;
	endpointListener->endpointRemoved = topologyManager_removeImportedService;
	activator->endpointListener = endpointListener;

	const char *uuid = NULL;
	status = bundleContext_getProperty(activator->context, OSGI_FRAMEWORK_FRAMEWORK_UUID, &uuid);
	if (!uuid) {
		logHelper_log(activator->loghelper, OSGI_LOGSERVICE_ERROR, "TOPOLOGY_MANAGER: no framework UUID defined?!");
		return CELIX_ILLEGAL_STATE;
	}

	size_t len = 14 + strlen(OSGI_FRAMEWORK_OBJECTCLASS) + strlen(OSGI_RSA_ENDPOINT_FRAMEWORK_UUID) + strlen(uuid);
	char *scope = malloc(len);
	if (!scope) {
		return CELIX_ENOMEM;
	}

	snprintf(scope, len, "(&(%s=*)(!(%s=%s)))", OSGI_FRAMEWORK_OBJECTCLASS, OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, uuid);

	logHelper_log(activator->loghelper, OSGI_LOGSERVICE_INFO, "TOPOLOGY_MANAGER: endpoint listener scope is %s", scope);

	properties_pt props = properties_create();
	properties_set(props, (char *) OSGI_ENDPOINT_LISTENER_SCOPE, scope);

	// We can release the scope, as properties_set makes a copy of the key & value...
	free(scope);

	bundleContext_registerService(context, (char *) OSGI_ENDPOINT_LISTENER_SERVICE, endpointListener, props, &activator->endpointListenerService);

	listener_hook_service_pt hookService = malloc(sizeof(*hookService));
	hookService->handle = activator->manager;
	hookService->added = topologyManager_listenerAdded;
	hookService->removed = topologyManager_listenerRemoved;
	activator->hookService = hookService;

	bundleContext_registerService(context, (char *) OSGI_FRAMEWORK_LISTENER_HOOK_SERVICE_NAME, hookService, NULL, &activator->hook);
	bundleContext_addServiceListener(context, activator->serviceListener, "(service.exported.interfaces=*)");

	if (status == CELIX_SUCCESS) {
		serviceTracker_open(activator->remoteServiceAdminTracker);
	}

	if (status == CELIX_SUCCESS) {
		status = serviceTracker_open(activator->endpointListenerTracker);
	}

	bundleContext_registerService(context, (char *) TOPOLOGYMANAGER_SCOPE_SERVICE, activator->scopeService, NULL, &activator->scopeReg);

	array_list_pt references = NULL;
	bundleContext_getServiceReferences(context, NULL, "(service.exported.interfaces=*)", &references);
	int i;
	for (i = 0; i < arrayList_size(references); i++) {
		service_reference_pt reference = arrayList_get(references, i);
		const char* serviceId = NULL;
		status = CELIX_DO_IF(status, serviceReference_getProperty(reference, OSGI_FRAMEWORK_SERVICE_ID, &serviceId));

		CELIX_DO_IF(status, topologyManager_addExportedService(activator->manager, reference, (char*)serviceId));
	}
	arrayList_destroy(references);

	return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;

	if (serviceTracker_close(activator->remoteServiceAdminTracker) == CELIX_SUCCESS) {
		serviceTracker_destroy(activator->remoteServiceAdminTracker);
	}

	if (serviceTracker_close(activator->endpointListenerTracker) == CELIX_SUCCESS) {
		serviceTracker_destroy(activator->endpointListenerTracker);
	}

	bundleContext_removeServiceListener(context, activator->serviceListener);
	free(activator->serviceListener);

	serviceRegistration_unregister(activator->hook);
	free(activator->hookService);

	serviceRegistration_unregister(activator->endpointListenerService);
	free(activator->endpointListener);

	serviceRegistration_unregister(activator->scopeReg);

	topologyManager_closeImports(activator->manager);

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;

	struct activator *activator = userData;
	if (!activator || !activator->manager) {
		status = CELIX_BUNDLE_EXCEPTION;
	} else {
		logHelper_stop(activator->loghelper);
		logHelper_destroy(&activator->loghelper);

		status = topologyManager_destroy(activator->manager);

		if (activator->scopeService) {
			free(activator->scopeService);
		}

		free(activator);
	}

	return status;
}
