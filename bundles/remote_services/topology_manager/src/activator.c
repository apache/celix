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
 * activator.c
 *
 *  \date       Sep 29, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "celix_constants.h"
#include "celix_bundle_activator.h"
#include "service_tracker.h"
#include "service_registration.h"

#include "topology_manager.h"
#include "endpoint_listener.h"
#include "remote_constants.h"
#include "listener_hook_service.h"
#include "celix_log_helper.h"
#include "scope.h"
#include "tm_scope.h"
#include "topology_manager.h"

struct activator {
	celix_bundle_context_t *context;

	topology_manager_pt manager;

	service_tracker_t *endpointListenerTracker;
	service_tracker_t *remoteServiceAdminTracker;
	service_tracker_t *exportedServicesTracker;

	endpoint_listener_t *endpointListener;
	service_registration_t *endpointListenerService;

	listener_hook_service_pt hookService;
	service_registration_t *hook;

	tm_scope_service_pt	scopeService;
	service_registration_t *scopeReg;

    celix_log_helper_t *celix_logHelper;
};


static celix_status_t bundleActivator_createEPLTracker(struct activator *activator, service_tracker_t **tracker);
static celix_status_t bundleActivator_createRSATracker(struct activator *activator, service_tracker_t **tracker);
static celix_status_t bundleActivator_createExportedServicesTracker(struct activator *activator, service_tracker_t **tracker);

celix_status_t celix_bundleActivator_create(celix_bundle_context_t *context, void **userData) {
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
	activator->exportedServicesTracker = NULL;
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

    activator->celix_logHelper = celix_logHelper_create(context, "celix_rsa_topology_manager");

	status = topologyManager_create(context, activator->celix_logHelper, &activator->manager, &scope);
	activator->scopeService->handle = scope;

	if (status == CELIX_SUCCESS) {
		status = bundleActivator_createEPLTracker(activator, &activator->endpointListenerTracker);
		if (status == CELIX_SUCCESS) {
			status = bundleActivator_createRSATracker(activator, &activator->remoteServiceAdminTracker);
			if (status == CELIX_SUCCESS) {
				status = bundleActivator_createExportedServicesTracker(activator, &activator->exportedServicesTracker);
				if (status == CELIX_SUCCESS) {
					*userData = activator;
				}
			}
		}
	}

	if(status != CELIX_SUCCESS){
        celix_bundleActivator_destroy(activator,context);
	}

	return status;
}

static celix_status_t bundleActivator_createEPLTracker(struct activator *activator, service_tracker_t **tracker) {
	celix_status_t status;
	service_tracker_customizer_t *customizer = NULL;
	status = serviceTrackerCustomizer_create(activator->manager, topologyManager_endpointListenerAdding, topologyManager_endpointListenerAdded, topologyManager_endpointListenerModified,
			topologyManager_endpointListenerRemoved, &customizer);
	if (status == CELIX_SUCCESS) {
		status = serviceTracker_create(activator->context, (char *) CELIX_RSA_ENDPOINT_LISTENER_SERVICE_NAME, customizer, tracker);
	}
	return status;
}

static celix_status_t bundleActivator_createRSATracker(struct activator *activator, service_tracker_t **tracker) {
	celix_status_t status;
	service_tracker_customizer_t *customizer = NULL;
	status = serviceTrackerCustomizer_create(activator->manager, topologyManager_rsaAdding, topologyManager_rsaAdded, topologyManager_rsaModified, topologyManager_rsaRemoved, &customizer);
	if (status == CELIX_SUCCESS) {
		status = serviceTracker_create(activator->context, CELIX_RSA_REMOTE_SERVICE_ADMIN, customizer, tracker);
	}
	return status;
}

static celix_status_t bundleActivator_createExportedServicesTracker(struct activator *activator, service_tracker_t **tracker) {
    celix_status_t status;
    service_tracker_customizer_t *customizer = NULL;
    status = serviceTrackerCustomizer_create(activator->manager, NULL, topologyManager_addExportedService, NULL, topologyManager_removeExportedService, &customizer);
    if (status == CELIX_SUCCESS) {
        status = serviceTracker_createWithFilter(activator->context, "(&(objectClass=*)(service.exported.interfaces=*))", customizer, tracker);
    }
    return status;
}

celix_status_t celix_bundleActivator_start(void * userData, celix_bundle_context_t *context) {
	celix_status_t status;
	struct activator *activator = userData;

	endpoint_listener_t *endpointListener = malloc(sizeof(*endpointListener));
	endpointListener->handle = activator->manager;
	endpointListener->endpointAdded = topologyManager_addImportedService;
	endpointListener->endpointRemoved = topologyManager_removeImportedService;
	activator->endpointListener = endpointListener;

	const char *uuid = NULL;
	status = bundleContext_getProperty(activator->context, CELIX_FRAMEWORK_UUID, &uuid);
	if (!uuid) {
		celix_logHelper_log(activator->celix_logHelper, CELIX_LOG_LEVEL_ERROR, "TOPOLOGY_MANAGER: no framework UUID defined?!");
		return CELIX_ILLEGAL_STATE;
	}

	size_t len = 14 + strlen(CELIX_FRAMEWORK_SERVICE_NAME) + strlen(CELIX_RSA_ENDPOINT_FRAMEWORK_UUID) + strlen(uuid);
	char *scope = malloc(len);
	if (!scope) {
		return CELIX_ENOMEM;
	}

	snprintf(scope, len, "(&(%s=*)(!(%s=%s)))", CELIX_FRAMEWORK_SERVICE_NAME, CELIX_RSA_ENDPOINT_FRAMEWORK_UUID, uuid);

	celix_logHelper_log(activator->celix_logHelper, CELIX_LOG_LEVEL_INFO, "TOPOLOGY_MANAGER: endpoint listener scope is %s", scope);

	celix_properties_t *props = celix_properties_create();
	// topology manager should ingore itself endpoint listener service
	celix_properties_set(props, "TOPOLOGY_MANAGER", "true");
	celix_properties_set(props, (char *) CELIX_RSA_ENDPOINT_LISTENER_SCOPE, scope);

	// We can release the scope, as celix_properties_set makes a copy of the key & value...
	free(scope);

	bundleContext_registerService(context, (char *) CELIX_RSA_ENDPOINT_LISTENER_SERVICE_NAME, endpointListener, props, &activator->endpointListenerService);

	listener_hook_service_pt hookService = malloc(sizeof(*hookService));
	hookService->handle = activator->manager;
	hookService->added = topologyManager_listenerAdded;
	hookService->removed = topologyManager_listenerRemoved;
	activator->hookService = hookService;

	bundleContext_registerService(context, (char *) TOPOLOGYMANAGER_SCOPE_SERVICE, activator->scopeService, NULL, &activator->scopeReg);

    bundleContext_registerService(context, (char *) OSGI_FRAMEWORK_LISTENER_HOOK_SERVICE_NAME, hookService, NULL, &activator->hook);

    if (status == CELIX_SUCCESS) {
        serviceTracker_open(activator->remoteServiceAdminTracker);
    }

    if (status == CELIX_SUCCESS) {
        status = serviceTracker_open(activator->exportedServicesTracker);
    }

    if (status == CELIX_SUCCESS) {
        status = serviceTracker_open(activator->endpointListenerTracker);
    }

	return status;
}

celix_status_t celix_bundleActivator_stop(void * userData, celix_bundle_context_t *context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;

	if (serviceTracker_close(activator->remoteServiceAdminTracker) == CELIX_SUCCESS) {
		serviceTracker_destroy(activator->remoteServiceAdminTracker);
	}

	if (serviceTracker_close(activator->endpointListenerTracker) == CELIX_SUCCESS) {
		serviceTracker_destroy(activator->endpointListenerTracker);
	}

    if (serviceTracker_close(activator->exportedServicesTracker) == CELIX_SUCCESS) {
        serviceTracker_destroy(activator->exportedServicesTracker);
    }

	serviceRegistration_unregister(activator->hook);
	free(activator->hookService);


	topologyManager_closeImports(activator->manager);
	serviceRegistration_unregister(activator->endpointListenerService);
	free(activator->endpointListener);

	serviceRegistration_unregister(activator->scopeReg);


	return status;
}

celix_status_t celix_bundleActivator_destroy(void * userData, celix_bundle_context_t *context) {
	celix_status_t status = CELIX_SUCCESS;

	struct activator *activator = userData;
	if (!activator || !activator->manager) {
		status = CELIX_BUNDLE_EXCEPTION;
	} else {
		celix_logHelper_destroy(activator->celix_logHelper);

		status = topologyManager_destroy(activator->manager);

		if (activator->scopeService) {
			free(activator->scopeService);
		}

		free(activator);
	}

	return status;
}
