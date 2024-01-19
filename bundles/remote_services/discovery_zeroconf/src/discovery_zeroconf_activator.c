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
#include "discovery_zeroconf_announcer.h"
#include "discovery_zeroconf_watcher.h"
#include "endpoint_listener.h"
#include "remote_constants.h"
#include "remote_service_admin.h"
#include "celix_log_helper.h"
#include "celix_bundle_activator.h"
#include "celix_dm_component.h"
#include "celix_dm_service_dependency.h"
#include "celix_constants.h"
#include "celix_errno.h"
#include <stdio.h>
#include <assert.h>

typedef struct discovery_zeroconf_activator {
    celix_log_helper_t *logHelper;
    discovery_zeroconf_announcer_t *announcer;
    discovery_zeroconf_watcher_t  *watcher;
    endpoint_listener_t endpointListener;
}discovery_zeroconf_activator_t;

celix_status_t discoveryZeroconfActivator_start(discovery_zeroconf_activator_t *act, celix_bundle_context_t *ctx) {
    celix_status_t status = CELIX_SUCCESS;
    const char *fwUuid = celix_bundleContext_getProperty(ctx, CELIX_FRAMEWORK_UUID, NULL);
    if (fwUuid == NULL) {
        return CELIX_BUNDLE_EXCEPTION;
    }
    celix_autoptr(celix_log_helper_t) logger = celix_logHelper_create(ctx, "celix_rsa_zeroconf_discovery");
    if (logger == NULL) {
        return CELIX_ENOMEM;
    }
    //init announcer
    celix_autoptr(celix_dm_component_t) announcerCmp = celix_dmComponent_create(ctx, "DZC_ANNOUNCER_CMP");
    if (announcerCmp == NULL) {
        return CELIX_ENOMEM;
    }
    status = discoveryZeroconfAnnouncer_create(ctx, logger, &act->announcer);
    if (status != CELIX_SUCCESS) {
        return status;
    }
    celix_dmComponent_setImplementation(announcerCmp, act->announcer);
    CELIX_DM_COMPONENT_SET_IMPLEMENTATION_DESTROY_FUNCTION(announcerCmp, discovery_zeroconf_announcer_t, discoveryZeroconfAnnouncer_destroy);
    celix_properties_t *props = celix_properties_create();
    if (props == NULL) {
        return CELIX_ENOMEM;
    }
    char scope[256] = {0};
    (void)snprintf(scope, sizeof(scope), "(&(%s=*)(%s=%s))", CELIX_FRAMEWORK_SERVICE_NAME,
                   CELIX_RSA_ENDPOINT_FRAMEWORK_UUID, fwUuid);
    celix_properties_set(props, CELIX_RSA_ENDPOINT_LISTENER_SCOPE, scope);
    celix_properties_set(props, "DISCOVERY", "true");//Only use to avoid the discovery calls to unnecessary endpoint listener service. Endpoint should be filtered by the scope.
    act->endpointListener.handle = act->announcer;
    act->endpointListener.endpointAdded = discoveryZeroconfAnnouncer_endpointAdded;
    act->endpointListener.endpointRemoved = discoveryZeroconfAnnouncer_endpointRemoved;
    celix_dmComponent_addInterface(announcerCmp, CELIX_RSA_ENDPOINT_LISTENER_SERVICE_NAME, NULL,
                                   &act->endpointListener, props);

    //init watcher
    celix_autoptr(celix_dm_component_t) watcherCmp = celix_dmComponent_create(ctx, "DZC_WATCHER_CMP");
    if (watcherCmp == NULL) {
        return CELIX_ENOMEM;
    }
    status = discoveryZeroconfWatcher_create(ctx, logger, &act->watcher);
    if (status != CELIX_SUCCESS) {
        return status;
    }
    celix_dmComponent_setImplementation(watcherCmp, act->watcher);
    CELIX_DM_COMPONENT_SET_IMPLEMENTATION_DESTROY_FUNCTION(watcherCmp, discovery_zeroconf_watcher_t, discoveryZeroconfWatcher_destroy);
    {
        celix_dm_service_dependency_t *discoveredEplDep = celix_dmServiceDependency_create();
        if (discoveredEplDep == NULL) {
            return CELIX_ENOMEM;
        }
        celix_dmServiceDependency_setService(discoveredEplDep, CELIX_RSA_ENDPOINT_LISTENER_SERVICE_NAME, NULL, "(!(DISCOVERY=true))");
        celix_dm_service_dependency_callback_options_t opts = CELIX_EMPTY_DM_SERVICE_DEPENDENCY_CALLBACK_OPTIONS;
        opts.addWithProps = discoveryZeroconfWatcher_addEPL;
        opts.removeWithProps = discoveryZeroconfWatcher_removeEPL;
        celix_dmServiceDependency_setCallbacksWithOptions(discoveredEplDep, &opts);
        celix_dmServiceDependency_setStrategy(discoveredEplDep, DM_SERVICE_DEPENDENCY_STRATEGY_LOCKING);
        celix_dmComponent_addServiceDependency(watcherCmp, discoveredEplDep);
    }
    {
        celix_dm_service_dependency_t *rsaDep = celix_dmServiceDependency_create();
        if (rsaDep == NULL) {
            return CELIX_ENOMEM;
        }
        celix_dmServiceDependency_setService(rsaDep, CELIX_RSA_REMOTE_SERVICE_ADMIN,
                                             NULL, "(remote.configs.supported=*)");
        celix_dm_service_dependency_callback_options_t opts = CELIX_EMPTY_DM_SERVICE_DEPENDENCY_CALLBACK_OPTIONS;
        opts.addWithProps = discoveryZeroConfWatcher_addRSA;
        opts.removeWithProps = discoveryZeroConfWatcher_removeRSA;
        celix_dmServiceDependency_setCallbacksWithOptions(rsaDep, &opts);
        celix_dmServiceDependency_setStrategy(rsaDep, DM_SERVICE_DEPENDENCY_STRATEGY_LOCKING);
        celix_dmComponent_addServiceDependency(watcherCmp, rsaDep);
    }

    celix_dependency_manager_t *mng = celix_bundleContext_getDependencyManager(ctx);
    if (mng == NULL) {
        return CELIX_ENOMEM;
    }
    celix_dependencyManager_addAsync(mng, celix_steal_ptr(announcerCmp));
    celix_dependencyManager_addAsync(mng, celix_steal_ptr(watcherCmp));

    act->logHelper = celix_steal_ptr(logger);
    return CELIX_SUCCESS;
}

celix_status_t discoveryZeroconfActivator_stop(discovery_zeroconf_activator_t *act, celix_bundle_context_t *ctx) {
    celix_dependency_manager_t *mng = celix_bundleContext_getDependencyManager(ctx);
    assert(mng != NULL);
    celix_dependencyManager_removeAllComponents(mng);
    celix_bundleContext_waitForEvents(ctx);
    celix_logHelper_destroy(act->logHelper);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(discovery_zeroconf_activator_t, discoveryZeroconfActivator_start, discoveryZeroconfActivator_stop)