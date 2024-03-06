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
#include <assert.h>

#include "celix_bundle_activator.h"
#include "celix_event_admin.h"
#include "celix_event_adapter.h"
#include "celix_event_admin_service.h"
#include "celix_event_handler_service.h"
#include "celix_event_constants.h"

typedef struct celix_event_admin_activator {
    celix_event_admin_t *eventAdmin;
    celix_event_admin_service_t eventAdminService;
    celix_event_adapter_t *eventAdapter;
} celix_event_admin_activator_t;

celix_status_t celix_eventAdminActivator_start(celix_event_admin_activator_t *act, celix_bundle_context_t *ctx) {
    assert(act != NULL);
    assert(ctx != NULL);

    celix_autoptr(celix_dm_component_t) adminCmp = celix_dmComponent_create(ctx, "EVENT_ADMIN_CMP");
    if (adminCmp == NULL) {
        return CELIX_ENOMEM;
    }

    act->eventAdmin = celix_eventAdmin_create(ctx);
    if (act->eventAdmin == NULL) {
        return CELIX_ENOMEM;
    }
    celix_dmComponent_setImplementation(adminCmp, act->eventAdmin);
    CELIX_DM_COMPONENT_SET_CALLBACKS(adminCmp, celix_event_admin_t, NULL, celix_eventAdmin_start, celix_eventAdmin_stop, NULL);
    CELIX_DM_COMPONENT_SET_IMPLEMENTATION_DESTROY_FUNCTION(adminCmp, celix_event_admin_t, celix_eventAdmin_destroy);

    celix_dm_service_dependency_t *eventHandlerDep = celix_dmServiceDependency_create();
    if (eventHandlerDep == NULL) {
        return CELIX_ENOMEM;
    }
    celix_dmServiceDependency_setService(eventHandlerDep, CELIX_EVENT_HANDLER_SERVICE_NAME, CELIX_EVENT_HANDLER_SERVICE_USE_RANGE, "("CELIX_EVENT_TOPIC"=*)");//Event Handlers which have not specified the EVENT_TOPIC service property must not receive events.
    celix_dmServiceDependency_setStrategy(eventHandlerDep, DM_SERVICE_DEPENDENCY_STRATEGY_LOCKING);
    celix_dm_service_dependency_callback_options_t opts = CELIX_EMPTY_DM_SERVICE_DEPENDENCY_CALLBACK_OPTIONS;
    opts.addWithProps = celix_eventAdmin_addEventHandlerWithProperties;
    opts.removeWithProps = celix_eventAdmin_removeEventHandlerWithProperties;
    celix_dmServiceDependency_setCallbacksWithOptions(eventHandlerDep, &opts);
    celix_dmComponent_addServiceDependency(adminCmp, eventHandlerDep);

    act->eventAdminService.handle = act->eventAdmin;
    act->eventAdminService.postEvent = celix_eventAdmin_postEvent;
    act->eventAdminService.sendEvent = celix_eventAdmin_sendEvent;
    celix_dmComponent_addInterface(adminCmp, CELIX_EVENT_ADMIN_SERVICE_NAME, CELIX_EVENT_ADMIN_SERVICE_VERSION, &act->eventAdminService, NULL);

    celix_autoptr(celix_dm_component_t) adapterCmp = celix_dmComponent_create(ctx, "EVENT_ADAPTER_CMP");
    if (adapterCmp == NULL) {
        return CELIX_ENOMEM;
    }
    act->eventAdapter = celix_eventAdapter_create(ctx);
    if (act->eventAdapter == NULL) {
        return CELIX_ENOMEM;
    }
    celix_dmComponent_setImplementation(adapterCmp, act->eventAdapter);
    CELIX_DM_COMPONENT_SET_CALLBACKS(adapterCmp, celix_event_adapter_t, NULL, celix_eventAdapter_start, celix_eventAdapter_stop, NULL);
    CELIX_DM_COMPONENT_SET_IMPLEMENTATION_DESTROY_FUNCTION(adapterCmp, celix_event_adapter_t, celix_eventAdapter_destroy);
    celix_dm_service_dependency_t *eventAdminDep = celix_dmServiceDependency_create();
    if (eventAdminDep == NULL) {
        return CELIX_ENOMEM;
    }
    celix_dmServiceDependency_setService(eventAdminDep, CELIX_EVENT_ADMIN_SERVICE_NAME, CELIX_EVENT_ADMIN_SERVICE_USE_RANGE, NULL);
    celix_dmServiceDependency_setRequired(eventAdminDep, true);
    celix_dmServiceDependency_setStrategy(eventAdminDep, DM_SERVICE_DEPENDENCY_STRATEGY_LOCKING);
    celix_dm_service_dependency_callback_options_t opts2 = CELIX_EMPTY_DM_SERVICE_DEPENDENCY_CALLBACK_OPTIONS;
    opts2.set = celix_eventAdapter_setEventAdminService;
    celix_dmServiceDependency_setCallbacksWithOptions(eventAdminDep, &opts2);
    celix_dmComponent_addServiceDependency(adapterCmp, eventAdminDep);


    celix_dependency_manager_t *mng = celix_bundleContext_getDependencyManager(ctx);
    if (mng == NULL) {
        return CELIX_ENOMEM;
    }

    celix_dependencyManager_addAsync(mng, celix_steal_ptr(adapterCmp));
    celix_dependencyManager_addAsync(mng, celix_steal_ptr(adminCmp));

    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(celix_event_admin_activator_t, celix_eventAdminActivator_start, NULL)