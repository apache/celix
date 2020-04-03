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

#include <celix_bundle_activator.h>
#include "celix_constants.h"
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
	celix_bundle_context_t *context;

	topology_manager_t *manager;

    celix_service_tracker_t *endpointListenerTracker;
    celix_service_tracker_t *remoteServiceAdminTracker;
    celix_service_tracker_t *exportedServicesTracker;

	endpoint_listener_t *endpointListener;
    long endpointServiceId;

    celix_listener_hook_service_t *hookService;
    long hookServiceId;

	tm_scope_service_t *scopeService;
    long scopeServiceId;

	log_helper_t *loghelper;
};


static celix_status_t bundleActivator_createEPLTracker(struct activator *activator, celix_service_tracker_t **tracker);
static celix_status_t bundleActivator_createRSATracker(struct activator *activator, celix_service_tracker_t **tracker);
static celix_status_t bundleActivator_createExportedServicesTracker(struct activator *activator, celix_service_tracker_t **tracker);

static celix_status_t bundleActivator_createEPLTracker(struct activator *activator, celix_service_tracker_t **tracker) {
	celix_status_t status;

	service_tracker_customizer_t *customizer = NULL;
	status = serviceTrackerCustomizer_create(activator->manager, topologyManager_endpointListenerAdding, topologyManager_endpointListenerAdded, topologyManager_endpointListenerModified,
			topologyManager_endpointListenerRemoved, &customizer);
	if (status == CELIX_SUCCESS) {
		status = serviceTracker_create(activator->context, OSGI_ENDPOINT_LISTENER_SERVICE, customizer, tracker);
	}
	return status;
}

static celix_status_t bundleActivator_createRSATracker(struct activator *activator, celix_service_tracker_t **tracker) {
	celix_status_t status;
	service_tracker_customizer_t *customizer = NULL;
	status = serviceTrackerCustomizer_create(activator->manager, topologyManager_rsaAdding, topologyManager_rsaAdded, topologyManager_rsaModified, topologyManager_rsaRemoved, &customizer);
	if (status == CELIX_SUCCESS) {
		status = serviceTracker_create(activator->context, OSGI_RSA_REMOTE_SERVICE_ADMIN, customizer, tracker);
	}
	return status;
}

static celix_status_t bundleActivator_createExportedServicesTracker(struct activator *activator, celix_service_tracker_t **tracker) {
    celix_status_t status;
    service_tracker_customizer_t *customizer = NULL;
    status = serviceTrackerCustomizer_create(activator->manager, NULL, topologyManager_addExportedService, NULL, topologyManager_removeExportedService, &customizer);
    if (status == CELIX_SUCCESS) {
        status = serviceTracker_createWithFilter(activator->context, "(service.exported.interfaces=*)", customizer, tracker);
    }
    return status;
}

static celix_status_t rsa_topology_stop(struct activator *act, celix_bundle_context_t *ctx);

static celix_status_t rsa_topology_start(struct activator *act, celix_bundle_context_t *ctx) {
    celix_status_t status = CELIX_SUCCESS;
    void *topology_scope;

    act->context = ctx;
    act->manager = NULL;
    act->endpointListener = NULL;
    act->endpointServiceId = -1;
    act->hookServiceId = -1;
    act->hookService = NULL;
    act->scopeService = calloc(1, sizeof(tm_scope_service_t));
    act->scopeServiceId = -1;
    act->endpointListenerTracker = NULL;
    act->remoteServiceAdminTracker = NULL;
    act->exportedServicesTracker = NULL;
    if (act->scopeService == NULL) {
        return CELIX_ENOMEM;
    }

    act->scopeService->addExportScope = tm_addExportScope;
    act->scopeService->removeExportScope = tm_removeExportScope;
    act->scopeService->addImportScope = tm_addImportScope;
    act->scopeService->removeImportScope = tm_removeImportScope;

    logHelper_create(ctx, &act->loghelper);
    logHelper_start(act->loghelper);

    status = topologyManager_create(ctx, act->loghelper, &act->manager, &topology_scope);
    act->scopeService->handle = topology_scope;

    if (status == CELIX_SUCCESS) {
        status = bundleActivator_createEPLTracker(act, &act->endpointListenerTracker);
        if (status == CELIX_SUCCESS) {
            status = bundleActivator_createRSATracker(act, &act->remoteServiceAdminTracker);
            if (status == CELIX_SUCCESS) {
                status = bundleActivator_createExportedServicesTracker(act, &act->exportedServicesTracker);
            }
        }
    }

    const char *uuid = NULL;
    if(status == CELIX_SUCCESS) {
        status = bundleContext_getProperty(act->context, OSGI_FRAMEWORK_FRAMEWORK_UUID, &uuid);
    }

    if(status == CELIX_SUCCESS) {
        if (!uuid) {
            logHelper_log(act->loghelper, OSGI_LOGSERVICE_ERROR, "TOPOLOGY_MANAGER: no framework UUID defined?!");
            return CELIX_ILLEGAL_STATE;
        }

        size_t len = 14 + strlen(OSGI_FRAMEWORK_OBJECTCLASS) + strlen(OSGI_RSA_ENDPOINT_FRAMEWORK_UUID) + strlen(uuid);
        char *scope = malloc(len);
        if (!scope) {
            return CELIX_ENOMEM;
        }

        snprintf(scope, len, "(&(%s=*)(!(%s=%s)))", OSGI_FRAMEWORK_OBJECTCLASS, OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, uuid);

        logHelper_log(act->loghelper, OSGI_LOGSERVICE_INFO, "TOPOLOGY_MANAGER: endpoint listener scope is %s", scope);

        celix_properties_t *props = celix_properties_create();
        celix_properties_set(props, OSGI_ENDPOINT_LISTENER_SCOPE, scope);

        // We can release the scope, as celix_properties_set makes a copy of the key & value...
        free(scope);

        endpoint_listener_t *endpointListener = malloc(sizeof(*endpointListener));
        endpointListener->handle = act->manager;
        endpointListener->endpointAdded = topologyManager_addImportedService;
        endpointListener->endpointRemoved = topologyManager_removeImportedService;
        act->endpointListener = endpointListener;

        act->endpointServiceId = celix_bundleContext_registerService(ctx, act->endpointListener, OSGI_ENDPOINT_LISTENER_SERVICE, props);
        if(act->endpointServiceId < 0) {
            status = CELIX_BUNDLE_EXCEPTION;
        }
    }

    if(status == CELIX_SUCCESS) {
        act->scopeServiceId = celix_bundleContext_registerService(ctx, act->scopeService, TOPOLOGYMANAGER_SCOPE_SERVICE, NULL);
        if(act->scopeServiceId < 0) {
            status = CELIX_BUNDLE_EXCEPTION;
        }
    }

    if(status == CELIX_SUCCESS) {
        listener_hook_service_pt hookService = malloc(sizeof(*hookService));
        hookService->handle = act->manager;
        hookService->added = topologyManager_listenerAdded;
        hookService->removed = topologyManager_listenerRemoved;
        act->hookService = hookService;

        act->hookServiceId = celix_bundleContext_registerService(ctx, act->hookService, OSGI_FRAMEWORK_LISTENER_HOOK_SERVICE_NAME, NULL);
        if(act->hookServiceId < 0) {
            status = CELIX_BUNDLE_EXCEPTION;
        }
    }

    if (status == CELIX_SUCCESS) {
        status = serviceTracker_open(act->exportedServicesTracker);
    }

    if (status == CELIX_SUCCESS) {
        status = serviceTracker_open(act->remoteServiceAdminTracker);
    }

    if (status == CELIX_SUCCESS) {
        status = serviceTracker_open(act->endpointListenerTracker);
    }

    if(status != CELIX_SUCCESS) {
        rsa_topology_stop(act, ctx);
    }

    return status;
}


static celix_status_t rsa_topology_stop(struct activator *act, celix_bundle_context_t *ctx) {
    celix_status_t status = CELIX_SUCCESS;

    if (!act || !act->manager) {
        return CELIX_BUNDLE_EXCEPTION;
    }

    if (serviceTracker_close(act->remoteServiceAdminTracker) == CELIX_SUCCESS) {
        status |= serviceTracker_destroy(act->remoteServiceAdminTracker);
    }

    if (serviceTracker_close(act->endpointListenerTracker) == CELIX_SUCCESS) {
        status |= serviceTracker_destroy(act->endpointListenerTracker);
    }

    if (serviceTracker_close(act->exportedServicesTracker) == CELIX_SUCCESS) {
        status |= serviceTracker_destroy(act->exportedServicesTracker);
    }

    celix_bundleContext_unregisterService(ctx, act->endpointServiceId);
    celix_bundleContext_unregisterService(ctx, act->scopeServiceId);
    celix_bundleContext_unregisterService(ctx, act->hookServiceId);

    free(act->scopeService);
    free(act->hookService);
    free(act->endpointListener);

    status |= topologyManager_closeImports(act->manager);

    status |= logHelper_stop(act->loghelper);
    status |= logHelper_destroy(&act->loghelper);

    status |= topologyManager_destroy(act->manager);

    return status;
}

CELIX_GEN_BUNDLE_ACTIVATOR(struct activator, rsa_topology_start, rsa_topology_stop);