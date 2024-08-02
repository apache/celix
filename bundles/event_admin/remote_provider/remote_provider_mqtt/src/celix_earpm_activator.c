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
#include "celix_log_helper.h"
#include "celix_bundle_activator.h"
#include "celix_dm_component.h"
#include "celix_dm_service_dependency.h"
#include "celix_event_constants.h"
#include "celix_event_admin_service.h"
#include "celix_event_handler_service.h"
#include "celix_event_remote_provider_service.h"
#include "celix_mqtt_broker_info_service.h"
#include "celix_earpm_impl.h"
#include "celix_earpm_broker_profile_parser.h"

typedef struct celix_event_admin_remote_provider_mqtt_activator {
    celix_bundle_context_t *ctx;
    celix_log_helper_t *logHelper;
    celix_earpm_broker_profile_parser_t* brokerProfileParser;
    celix_event_admin_remote_provider_mqtt_t* providerMqtt;
    celix_event_remote_provider_service_t providerSvc;
} celix_event_admin_remote_provider_mqtt_activator_t;

static celix_status_t celix_eventAdminRemoteProviderMqttActivator_start(celix_event_admin_remote_provider_mqtt_activator_t *act, celix_bundle_context_t *ctx) {
    assert(act != NULL);
    assert(ctx != NULL);
    act->ctx = ctx;
    celix_autoptr(celix_dm_component_t) earpmpCmp = celix_dmComponent_create(ctx, "CELIX_EARPMP_CMP");
    act->brokerProfileParser = celix_earpmp_create(ctx);
    if (act->brokerProfileParser == NULL) {
        return CELIX_BUNDLE_EXCEPTION;
    }
    celix_dmComponent_setImplementation(earpmpCmp, act->brokerProfileParser);
    CELIX_DM_COMPONENT_SET_IMPLEMENTATION_DESTROY_FUNCTION(earpmpCmp, celix_earpm_broker_profile_parser_t, celix_earpmp_destroy);

    celix_autoptr(celix_dm_component_t) earpmCmp = celix_dmComponent_create(ctx, "CELIX_EARPM_CMP");
    if (earpmCmp == NULL) {
        return CELIX_ENOMEM;
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
            return CELIX_ENOMEM;
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
        celix_autoptr(celix_dm_service_dependency_t) brokerInfoDep = celix_dmServiceDependency_create();
        if (brokerInfoDep == NULL) {
            return CELIX_ENOMEM;
        }
        celix_status_t status = celix_dmServiceDependency_setService(brokerInfoDep, CELIX_MQTT_BROKER_INFO_SERVICE_NAME, CELIX_MQTT_BROKER_INFO_SERVICE_RANGE, NULL);
        if (status != CELIX_SUCCESS) {
            return status;
        }
        celix_dmServiceDependency_setStrategy(brokerInfoDep, DM_SERVICE_DEPENDENCY_STRATEGY_LOCKING);
        celix_dmServiceDependency_setRequired(brokerInfoDep, true);
        celix_dm_service_dependency_callback_options_t opts = CELIX_EMPTY_DM_SERVICE_DEPENDENCY_CALLBACK_OPTIONS;
        opts.addWithProps = celix_earpm_addBrokerInfoService;
        opts.removeWithProps = celix_earpm_removeBrokerInfoService;
        celix_dmServiceDependency_setCallbacksWithOptions(brokerInfoDep, &opts);
        status = celix_dmComponent_addServiceDependency(earpmCmp, brokerInfoDep);
        if (status != CELIX_SUCCESS) {
            return status;
        }
        celix_steal_ptr(brokerInfoDep);
    }

    {
        celix_autoptr(celix_dm_service_dependency_t) eventAdminDep = celix_dmServiceDependency_create();
        if (eventAdminDep == NULL) {
            return CELIX_ENOMEM;
        }
        celix_status_t status = celix_dmServiceDependency_setService(eventAdminDep, CELIX_EVENT_ADMIN_SERVICE_NAME, CELIX_EVENT_ADMIN_SERVICE_USE_RANGE, NULL);
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
    celix_status_t status = celix_dmComponent_addInterface(earpmCmp, CELIX_EVENT_REMOTE_PROVIDER_SERVICE_NAME, CELIX_EVENT_REMOTE_PROVIDER_SERVICE_VERSION, &act->providerSvc, NULL);
    if (status != CELIX_SUCCESS) {
        return status;
    }

    celix_dependency_manager_t* mng = celix_bundleContext_getDependencyManager(ctx);
    if (mng == NULL) {
        return CELIX_ENOMEM;
    }
    status = celix_dependencyManager_addAsync(mng, earpmpCmp);
    if (status != CELIX_SUCCESS) {
        return status;
    }
    status = celix_dependencyManager_addAsync(mng, earpmCmp);
    if (status != CELIX_SUCCESS) {
        celix_dependencyManager_removeAsync(mng, celix_steal_ptr(earpmpCmp), NULL, NULL);
        return status;
    }
    celix_steal_ptr(earpmpCmp);
    celix_steal_ptr(earpmCmp);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(celix_event_admin_remote_provider_mqtt_activator_t, celix_eventAdminRemoteProviderMqttActivator_start, NULL);