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
#include <errno.h>

#include "celix_errno.h"
#include "celix_bundle_activator.h"
#include "celix_dm_component.h"
#include "celix_dm_service_dependency.h"
#include "endpoint_listener.h"
#include "remote_constants.h"
#include "celix_event_constants.h"
#include "celix_event_admin_service.h"
#include "celix_event_handler_service.h"
#include "celix_event_remote_provider_service.h"
#include "celix_earpm_impl.h"
#include "celix_earpm_broker_discovery.h"

typedef struct celix_event_admin_remote_provider_mqtt_activator {
    celix_bundle_context_t *ctx;
    celix_log_helper_t *logHelper;
    celix_earpm_broker_discovery_t* brokerDiscovery;
    celix_event_admin_remote_provider_mqtt_t* providerMqtt;
    celix_event_remote_provider_service_t providerSvc;
    endpoint_listener_t endpointListener;
} celix_event_admin_remote_provider_mqtt_activator_t;

static celix_status_t celix_eventAdminRemoteProviderMqttActivator_start(celix_event_admin_remote_provider_mqtt_activator_t *act, celix_bundle_context_t *ctx) {
    assert(act != NULL);
    assert(ctx != NULL);
    act->ctx = ctx;
    celix_autoptr(celix_dm_component_t) earpmDiscoveryCmp = celix_dmComponent_create(ctx, "CELIX_EARPM_DISCOVERY_CMP");
    if (earpmDiscoveryCmp == NULL) {
        return ENOMEM;
    }
    act->brokerDiscovery = celix_earpmDiscovery_create(ctx);
    if (act->brokerDiscovery == NULL) {
        return CELIX_BUNDLE_EXCEPTION;
    }
    celix_dmComponent_setImplementation(earpmDiscoveryCmp, act->brokerDiscovery);
    CELIX_DM_COMPONENT_SET_IMPLEMENTATION_DESTROY_FUNCTION(earpmDiscoveryCmp, celix_earpm_broker_discovery_t, celix_earpmDiscovery_destroy);

    {
        celix_autoptr(celix_dm_service_dependency_t) endpointListenerDep = celix_dmServiceDependency_create();
        if (endpointListenerDep == NULL) {
            return ENOMEM;
        }
        celix_status_t status = celix_dmServiceDependency_setService(endpointListenerDep, CELIX_RSA_ENDPOINT_LISTENER_SERVICE_NAME, NULL, NULL);
        if (status != CELIX_SUCCESS) {
            return status;
        }
        celix_dmServiceDependency_setStrategy(endpointListenerDep, DM_SERVICE_DEPENDENCY_STRATEGY_LOCKING);
        celix_dm_service_dependency_callback_options_t opts = CELIX_EMPTY_DM_SERVICE_DEPENDENCY_CALLBACK_OPTIONS;
        opts.addWithProps = celix_earpmDiscovery_addEndpointListener;
        opts.removeWithProps = celix_earpmDiscovery_removeEndpointListener;
        celix_dmServiceDependency_setCallbacksWithOptions(endpointListenerDep, &opts);
        status = celix_dmComponent_addServiceDependency(earpmDiscoveryCmp, endpointListenerDep);
        if (status != CELIX_SUCCESS) {
            return status;
        }
        celix_steal_ptr(endpointListenerDep);
    }

    celix_autoptr(celix_dm_component_t) earpmCmp = celix_dmComponent_create(ctx, "CELIX_EARPM_CMP");
    if (earpmCmp == NULL) {
        return ENOMEM;
    }
    act->providerMqtt = celix_earpm_create(ctx);
    if (act->providerMqtt == NULL) {
        return CELIX_BUNDLE_EXCEPTION;
    }
    celix_dmComponent_setImplementation(earpmCmp, act->providerMqtt);
    CELIX_DM_COMPONENT_SET_IMPLEMENTATION_DESTROY_FUNCTION(earpmCmp, celix_event_admin_remote_provider_mqtt_t, celix_earpm_destroy);

    {
        celix_autoptr(celix_dm_service_dependency_t) eventHandlerDep = celix_dmServiceDependency_create();
        if (eventHandlerDep == NULL) {
            return ENOMEM;
        }
        celix_status_t status = celix_dmServiceDependency_setService(eventHandlerDep, CELIX_EVENT_HANDLER_SERVICE_NAME, CELIX_EVENT_HANDLER_SERVICE_USE_RANGE, "("CELIX_EVENT_TOPIC"=*)");//Event Handlers which have not specified the EVENT_TOPIC service property must not receive events.
        if (status != CELIX_SUCCESS) {
            return status;
        }
        celix_dmServiceDependency_setStrategy(eventHandlerDep, DM_SERVICE_DEPENDENCY_STRATEGY_LOCKING);
        celix_dm_service_dependency_callback_options_t opts = CELIX_EMPTY_DM_SERVICE_DEPENDENCY_CALLBACK_OPTIONS;
        opts.addWithProps = celix_earpm_addEventHandlerService;
        opts.removeWithProps = celix_earpm_removeEventHandlerService;
        celix_dmServiceDependency_setCallbacksWithOptions(eventHandlerDep, &opts);
        status = celix_dmComponent_addServiceDependency(earpmCmp, eventHandlerDep);
        if (status != CELIX_SUCCESS) {
            return status;
        }
        celix_steal_ptr(eventHandlerDep);
    }

    {
        celix_autoptr(celix_dm_service_dependency_t) eventAdminDep = celix_dmServiceDependency_create();
        if (eventAdminDep == NULL) {
            return ENOMEM;
        }
        celix_status_t status = celix_dmServiceDependency_setService(eventAdminDep, CELIX_EVENT_ADMIN_SERVICE_NAME,
                                                                     CELIX_EVENT_ADMIN_SERVICE_USE_RANGE, NULL);
        if (status != CELIX_SUCCESS) {
            return status;
        }
        celix_dmServiceDependency_setStrategy(eventAdminDep, DM_SERVICE_DEPENDENCY_STRATEGY_LOCKING);
        celix_dm_service_dependency_callback_options_t opts = CELIX_EMPTY_DM_SERVICE_DEPENDENCY_CALLBACK_OPTIONS;
        opts.set = celix_earpm_setEventAdminSvc;
        celix_dmServiceDependency_setCallbacksWithOptions(eventAdminDep, &opts);
        status = celix_dmComponent_addServiceDependency(earpmCmp, eventAdminDep);
        if (status != CELIX_SUCCESS) {
            return status;
        }
        celix_steal_ptr(eventAdminDep);
    }

    act->providerSvc.handle = act->providerMqtt;
    act->providerSvc.postEvent = celix_earpm_postEvent;
    act->providerSvc.sendEvent = celix_earpm_sendEvent;
    celix_status_t status = celix_dmComponent_addInterface(earpmCmp, CELIX_EVENT_REMOTE_PROVIDER_SERVICE_NAME,
                                                           CELIX_EVENT_REMOTE_PROVIDER_SERVICE_VERSION, &act->providerSvc, NULL);
    if (status != CELIX_SUCCESS) {
        return status;
    }

    act->endpointListener.handle = act->providerMqtt;
    act->endpointListener.endpointAdded = celix_earpm_mqttBrokerEndpointAdded;
    act->endpointListener.endpointRemoved = celix_earpm_mqttBrokerEndpointRemoved;
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    if (props == NULL) {
        return ENOMEM;
    }
    const char* scope = "(&("CELIX_FRAMEWORK_SERVICE_NAME"="CELIX_EARPM_MQTT_BROKER_INFO_SERVICE_NAME")("\
            CELIX_RSA_SERVICE_IMPORTED_CONFIGS"="CELIX_EARPM_MQTT_BROKER_SERVICE_CONFIG_TYPE"))";
    status = celix_properties_set(props, CELIX_RSA_ENDPOINT_LISTENER_SCOPE, scope);
    if (status != CELIX_SUCCESS) {
        return status;
    }
    status = celix_properties_setBool(props, CELIX_RSA_DISCOVERY_INTERFACE_SPECIFIC_ENDPOINTS_SUPPORT, true);
    if (status != CELIX_SUCCESS) {
        return status;
    }
    status = celix_dmComponent_addInterface(earpmCmp, CELIX_RSA_ENDPOINT_LISTENER_SERVICE_NAME, NULL, &act->endpointListener,
                                            celix_steal_ptr(props));
    if (status != CELIX_SUCCESS) {
        return status;
    }

    celix_dependency_manager_t* mng = celix_bundleContext_getDependencyManager(ctx);
    if (mng == NULL) {
        return ENOMEM;
    }
    status = celix_dependencyManager_addAsync(mng, earpmDiscoveryCmp);
    if (status != CELIX_SUCCESS) {
        return status;
    }
    status = celix_dependencyManager_addAsync(mng, earpmCmp);
    if (status != CELIX_SUCCESS) {
        celix_dependencyManager_removeAsync(mng, celix_steal_ptr(earpmDiscoveryCmp), NULL, NULL);
        return status;
    }
    celix_steal_ptr(earpmDiscoveryCmp);
    celix_steal_ptr(earpmCmp);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(celix_event_admin_remote_provider_mqtt_activator_t, celix_eventAdminRemoteProviderMqttActivator_start, NULL);