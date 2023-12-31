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

#include <stdlib.h>
#include <string.h>
#include <uuid/uuid.h>
#include <assert.h>

#include "celix_utils.h"
#include "celix_bundle.h"
#include "celix_stdlib_cleanup.h"
#include "celix_constants.h"
#include "celix_filter.h"
#include "dm_component_impl.h"
#include "celix_framework.h"

static const char * const CELIX_DM_PRINT_OK_COLOR = "\033[92m";
static const char * const CELIX_DM_PRINT_WARNING_COLOR = "\033[93m";
static const char * const CELIX_DM_PRINT_NOK_COLOR = "\033[91m";
static const char * const CELIX_DM_PRINT_END_COLOR = "\033[m";

struct celix_dm_component_struct {
    char uuid[DM_COMPONENT_MAX_ID_LENGTH];
    char name[DM_COMPONENT_MAX_NAME_LENGTH];

    celix_bundle_context_t* context;
    void* implementation;
    celix_dm_cmp_impl_destroy_fpt implementationDestroyFn;

    celix_dm_cmp_lifecycle_fpt callbackInit;
    celix_dm_cmp_lifecycle_fpt callbackStart;
    celix_dm_cmp_lifecycle_fpt callbackStop;
    celix_dm_cmp_lifecycle_fpt callbackDeinit;

    celix_dm_component_state_t state; //atomic

    celix_thread_mutex_t mutex; //protects below
    celix_array_list_t* providedInterfaces; //type = dm_interface_t*
    celix_array_list_t* dependencies; //type = celix_dm_service_dependency_t*

    /**
     * Removed dependencies, which are potential still being stopped async
     */
    celix_array_list_t* removedDependencies; //type = celix_dm_service_dependency_t*

    size_t nrOfTimesStarted;
    size_t nrOfTimesResumed;

    bool isEnabled;

    /**
     * Whether the component is an a transition (active performTransition call).
     * Should only be used inside the Celix event Thread -> no locking needed.
     */
    bool inTransition;
};

typedef struct dm_interface_struct {
    char* serviceName;
    const void* service;
    celix_properties_t *properties;
    long svcId;
} dm_interface_t;

static celix_status_t celix_dmComponent_registerServices(celix_dm_component_t *component, bool needLock);
static celix_status_t celix_dmComponent_unregisterServices(celix_dm_component_t *component, bool needLock);
static bool celix_dmComponent_areAllRequiredServiceDependenciesResolved(celix_dm_component_t *component);
static bool celix_dmComponent_performTransition(celix_dm_component_t *component, celix_dm_component_state_t oldState, celix_dm_component_state_t newState);
static celix_status_t celix_dmComponent_calculateNewState(celix_dm_component_t *component, celix_dm_component_state_t currentState, celix_dm_component_state_t *newState);
static celix_status_t celix_dmComponent_handleChange(celix_dm_component_t *component);
static celix_status_t celix_dmComponent_handleAdd(celix_dm_component_t *component, const celix_dm_event_t* event);
static celix_status_t celix_dmComponent_handleRemove(celix_dm_component_t *component, const celix_dm_event_t* event);
static celix_status_t celix_dmComponent_handleSet(celix_dm_component_t *component, const celix_dm_event_t* event);
static celix_status_t celix_dmComponent_enableDependencies(celix_dm_component_t *component);
static celix_status_t celix_dmComponent_suspend(celix_dm_component_t *component, celix_dm_service_dependency_t *dependency);
static celix_status_t celix_dmComponent_resume(celix_dm_component_t *component, celix_dm_service_dependency_t *dependency);
static celix_status_t celix_dmComponent_disable(celix_dm_component_t *component);
static void celix_dmComponent_disableDirectly(celix_dm_component_t *component);
static celix_status_t celix_dmComponent_disableDependencies(celix_dm_component_t *component);
static bool celix_dmComponent_isDisabled(celix_dm_component_t *component);
static void celix_dmComponent_cleanupRemovedDependencies(celix_dm_component_t* component);
static bool celix_dmComponent_isActiveInternal(celix_dm_component_t *component);
static void celix_dmComponent_setCurrentState(celix_dm_component_t* cmp, celix_dm_component_state_t s);
static void celix_dmComponent_logTransition(celix_dm_component_t* cmp, celix_dm_component_state_t currentState, celix_dm_component_state_t desiredState);

celix_dm_component_t* celix_dmComponent_create(bundle_context_t *context, const char* name) {
    return celix_dmComponent_createWithUUID(context, name, NULL);
}

celix_dm_component_t* celix_dmComponent_createWithUUID(bundle_context_t *context, const char* name, const char *uuidIn) {
    celix_dm_component_t *component = calloc(1, sizeof(*component));

    char uuidStr[DM_COMPONENT_MAX_ID_LENGTH];
    bool genRandomUUID = true;
    if (uuidIn != NULL) {
        uuid_t uuid;
        int rc = uuid_parse(uuidIn, uuid);
        if (rc == 0) {
            uuid_unparse(uuid, uuidStr);
            genRandomUUID = false;
        } else {
            //parsing went wrong
            celix_bundleContext_log(component->context, CELIX_LOG_LEVEL_WARNING,
                   "Cannot parse provided uuid '%s'. Not a valid UUID?. UUID will be generated", uuidIn);
        }
    }

    if (genRandomUUID) {
        //gen uuid
        uuid_t uuid;
        uuid_generate(uuid);
        uuid_unparse(uuid, uuidStr);
    }
    snprintf(component->uuid, DM_COMPONENT_MAX_ID_LENGTH, "%s", uuidStr);

    snprintf(component->name, DM_COMPONENT_MAX_NAME_LENGTH, "%s", name == NULL ? "n/a" : name);

    component->context = context;
    component->implementation = NULL;
    component->callbackInit = NULL;
    component->callbackStart = NULL;
    component->callbackStop = NULL;
    component->callbackDeinit = NULL;

    component->implementation= NULL;
    component->implementationDestroyFn = NULL;

    component->state = CELIX_DM_CMP_STATE_INACTIVE;

    component->providedInterfaces = celix_arrayList_create();
    component->dependencies = celix_arrayList_create();
    component->removedDependencies = celix_arrayList_create();
    celixThreadMutex_create(&component->mutex, NULL);
    component->isEnabled = false;
    component->inTransition = false;
    return component;
}

celix_status_t component_create(bundle_context_pt context, const char *name, celix_dm_component_t **out) {
    celix_status_t status = CELIX_SUCCESS;
    celix_dm_component_t *cmp = celix_dmComponent_create(context, name);
    if (cmp == NULL) {
        status = CELIX_BUNDLE_EXCEPTION;
    } else {
        *out = cmp;
    }
    return status;
}

void component_destroy(celix_dm_component_t *component) {
    celix_dmComponent_destroy(component);
}

void celix_dmComponent_destroy(celix_dm_component_t *component) {
    if (component != NULL) {

        if (celix_framework_isCurrentThreadTheEventLoop(celix_bundleContext_getFramework(component->context))) {
            celix_bundleContext_log(component->context, CELIX_LOG_LEVEL_ERROR,
                   "Cannot synchronized destroy dm component on Celix event thread. Use celix_dmComponent_destroyAsync instead!");
        } else {
            celix_bundle_context_t* context = component->context;
            celix_dmComponent_destroyAsync(component, NULL, NULL);
            celix_bundleContext_waitForEvents(context);
        }
    }
}

struct celix_dm_component_destroy_data {
    celix_dm_component_t* cmp;
    void* doneData;
    void (*doneCallback)(void*);
};

static void celix_dmComponent_destroyCallback(void *voidData) {
    struct celix_dm_component_destroy_data *data = voidData;
    celix_dm_component_t *component = data->cmp;
    celix_dmComponent_disable(component); //all service unregistered // all svc tracker stopped
    if (celix_dmComponent_isDisabled(component)) {
        if (component->implementationDestroyFn) {
            if (component->implementation == NULL) {
                celix_bundleContext_log(component->context, CELIX_LOG_LEVEL_ERROR,
                                        "Component `%s` [uuid=%s] has a configured destroy component "
                                        "implementation callback, but the implementation pointer is NULL. "
                                        "Destroy callback will not be called!",
                                        component->name,
                                        component->uuid);
            } else {
                component->implementationDestroyFn(component->implementation);
            }
        }

        for (int i = 0; i < celix_arrayList_size(component->providedInterfaces); ++i) {
            dm_interface_t *interface = celix_arrayList_get(component->providedInterfaces, i);

            if (interface->properties != NULL) {
                celix_properties_destroy(interface->properties);
            }
            free(interface->serviceName);
            free(interface);
        }
        celix_arrayList_destroy(component->providedInterfaces);

        for (int i = 0; i < celix_arrayList_size(component->dependencies); ++i) {
            celix_dm_service_dependency_t *dep = celix_arrayList_get(component->dependencies, i);
            celix_dmServiceDependency_destroy(dep);
        }
        celix_arrayList_destroy(component->dependencies);

        for (int i = 0; i < celix_arrayList_size(component->removedDependencies); ++i) {
            celix_dm_service_dependency_t *dep = celix_arrayList_get(component->removedDependencies, i);
            celix_dmServiceDependency_destroy(dep);
        }
        celix_arrayList_destroy(component->removedDependencies);

        celixThreadMutex_destroy(&component->mutex);
        free(component);

        if (data->doneCallback) {
            data->doneCallback(data->doneData);
        }
        free(data);
    } else {
        //not yet disabled, adding a new event on the event queue
        celix_bundle_t* bnd = celix_bundleContext_getBundle(component->context);
        celix_framework_t* fw = celix_bundleContext_getFramework(component->context);
        celix_framework_fireGenericEvent(
                fw, -1, celix_bundle_getId(bnd),
                "destroy dm component",
                data,
                celix_dmComponent_destroyCallback,
                NULL,
                NULL);
    }
}

void celix_dmComponent_destroyAsync(celix_dm_component_t *component, void *doneData, void (*doneCallback)(void*)) {
    if (component != NULL) {
        struct celix_dm_component_destroy_data* data = malloc(sizeof(*data));
        data->cmp = component;
        data->doneData = doneData;
        data->doneCallback = doneCallback;

        celix_bundle_t* bnd = celix_bundleContext_getBundle(component->context);
        celix_framework_t* fw = celix_bundleContext_getFramework(component->context);
        celix_framework_fireGenericEvent(
                fw, -1, celix_bundle_getId(bnd),
                "destroy dm component",
                data,
                celix_dmComponent_destroyCallback,
                NULL,
                NULL);
	}
}


const char* celix_dmComponent_getUUID(celix_dm_component_t* cmp) {
    return cmp->uuid;
}

celix_status_t component_addServiceDependency(celix_dm_component_t *component, celix_dm_service_dependency_t *dep) {
    return celix_dmComponent_addServiceDependency(component, dep);
}

celix_status_t celix_dmComponent_addServiceDependency(celix_dm_component_t *component, celix_dm_service_dependency_t *dep) {
    celix_status_t status = CELIX_SUCCESS;
    celix_dmServiceDependency_setComponent(dep, component);

    celixThreadMutex_lock(&component->mutex);
    arrayList_add(component->dependencies, dep);
    bool startDep = celix_dmComponent_currentState(component) != CELIX_DM_CMP_STATE_INACTIVE;
    if (startDep) {
        celix_dmServiceDependency_enable(dep);
    }
    celix_dmComponent_cleanupRemovedDependencies(component);
    celixThreadMutex_unlock(&component->mutex);

    celix_dmComponent_handleChange(component);
    return status;
}

celix_status_t celix_dmComponent_removeServiceDependency(celix_dm_component_t *component, celix_dm_service_dependency_t *dep) {

    celixThreadMutex_lock(&component->mutex);
    celix_arrayList_remove(component->dependencies, dep);
    bool disableDependency = celix_dmComponent_currentState(component) != CELIX_DM_CMP_STATE_INACTIVE;
    if (disableDependency) {
        celix_dmServiceDependency_disable(dep);
    }
    celix_arrayList_add(component->removedDependencies, dep);
    celix_dmComponent_cleanupRemovedDependencies(component);
    celixThreadMutex_unlock(&component->mutex);

    celix_dmComponent_handleChange(component);
    return CELIX_SUCCESS;
}

celix_dm_component_state_t component_currentState(celix_dm_component_t *cmp) {
    return celix_dmComponent_currentState(cmp);
}

static void celix_dmComponent_setCurrentState(celix_dm_component_t* cmp, celix_dm_component_state_t s) {
    __atomic_store_n(&cmp->state, s, __ATOMIC_RELEASE);
}

celix_dm_component_state_t celix_dmComponent_currentState(celix_dm_component_t *cmp) {
   celix_dm_component_state_t state = __atomic_load_n(&cmp->state, __ATOMIC_ACQUIRE);
   return state;
}

void* component_getImplementation(celix_dm_component_t *cmp) {
    return celix_dmComponent_getImplementation(cmp);
}

void* celix_dmComponent_getImplementation(celix_dm_component_t *cmp) {
    return cmp->implementation;
}

const char* component_getName(celix_dm_component_t *cmp) {
    return celix_dmComponent_getName(cmp);
}

const char * celix_dmComponent_getName(celix_dm_component_t *cmp) {
    return cmp->name;
}

celix_status_t component_removeServiceDependency(celix_dm_component_t *component, celix_dm_service_dependency_t *dependency) {
    return celix_dmComponent_removeServiceDependency(component, dependency);
}

celix_status_t celix_private_dmComponent_enable(celix_dm_component_t *component) {
    celixThreadMutex_lock(&component->mutex);
    bool changed = false;
    if (!component->isEnabled) {
        changed = !component->isEnabled;
        component->isEnabled = true;
    }
    celixThreadMutex_unlock(&component->mutex);
    if (changed) {
        celix_dmComponent_handleChange(component);
    }
    return CELIX_SUCCESS;
}

/**
 * @brief disables component and effectualize this through normal component transitions.
 */
static celix_status_t celix_dmComponent_disable(celix_dm_component_t *component) {
    celixThreadMutex_lock(&component->mutex);
    bool changed = false;
    if (component->isEnabled) {
        changed = component->isEnabled;
        component->isEnabled = false;
    }
    celixThreadMutex_unlock(&component->mutex);
    if (changed) {
        celix_dmComponent_handleChange(component);
    }
    return CELIX_SUCCESS;
}

/**
 * Disables component directly, this is needed for an error flow.
 *
 * Should be called with component mutex locked.
 */
static void celix_dmComponent_disableDirectly(celix_dm_component_t *component) {
    component->isEnabled = false;
    celix_dmComponent_setCurrentState(component, CELIX_DM_CMP_STATE_INACTIVE);
    celix_dmComponent_unregisterServices(component, false);
    celix_dmComponent_disableDependencies(component);
}

static void celix_dmComponent_cleanupRemovedDependencies(celix_dm_component_t* component) {
    //note should be called with lock
    bool removedDep = true;
    while (removedDep) {
        removedDep = false;
        for (int i = 0 ; i < celix_arrayList_size(component->removedDependencies); ++i) {
            celix_dm_service_dependency_t* dep = celix_arrayList_get(component->removedDependencies, i);
            if (celix_dmServiceDependency_isDisabled(dep)) {
                celix_arrayList_remove(component->removedDependencies, dep);
                celix_dmServiceDependency_destroy(dep);
                removedDep = true;
                break;
            }
        }
    }
}

static bool celix_dmComponent_areAllDependenciesDisabled(celix_dm_component_t* component) {
    //note should be called with lock
    bool allDisabled = true;
    for (int i = 0 ; allDisabled && i < celix_arrayList_size(component->dependencies); ++i) {
        celix_dm_service_dependency_t* dep = celix_arrayList_get(component->dependencies, i);
        if (!celix_dmServiceDependency_isDisabled(dep)) {
            allDisabled = false;
        }
    }
    for (int i = 0 ; allDisabled && i < celix_arrayList_size(component->removedDependencies); ++i) {
        celix_dm_service_dependency_t* dep = celix_arrayList_get(component->removedDependencies, i);
        if (!celix_dmServiceDependency_isDisabled(dep)) {
            allDisabled = false;
        }
    }
    return allDisabled;
}

static bool celix_dmComponent_isDisabled(celix_dm_component_t *component) {
    bool isStopped;
    celixThreadMutex_lock(&component->mutex);
    isStopped =
            !component->isEnabled &&
            celix_dmComponent_currentState(component) == CELIX_DM_CMP_STATE_INACTIVE &&
            celix_dmComponent_areAllDependenciesDisabled(component);
    celixThreadMutex_unlock(&component->mutex);
    return isStopped;
}


celix_status_t component_setCLanguageProperty(celix_dm_component_t *component, bool setCLangProp) {
    return celix_dmComponent_setCLanguageProperty(component, setCLangProp);
}

celix_status_t celix_dmComponent_setCLanguageProperty(celix_dm_component_t *component, bool setCLangProp) {
    //nop
    return CELIX_SUCCESS;
}

celix_status_t component_addInterface(celix_dm_component_t *component, const char* serviceName, const char* serviceVersion, const void* service, celix_properties_t* properties) {
    return celix_dmComponent_addInterface(component, serviceName, serviceVersion, service, properties);
}

celix_status_t celix_dmComponent_addInterface(celix_dm_component_t* component,
                                              const char* serviceName,
                                              const char* serviceVersion,
                                              const void* service,
                                              celix_properties_t* properties) {
    if (serviceName == NULL || celix_utils_stringEquals(serviceName, "")) {
        celix_bundleContext_log(
            component->context, CELIX_LOG_LEVEL_ERROR, "Cannot add interface with a NULL or empty serviceName");
        return CELIX_ILLEGAL_ARGUMENT;
    }

    celix_autofree dm_interface_t* interface = calloc(1, sizeof(*interface));
    celix_autofree char* name = celix_utils_strdup(serviceName);

    if (properties == NULL) {
        properties = celix_properties_create();
    }

    if (serviceVersion != NULL) {
        celix_autoptr(celix_version_t) version = celix_version_createVersionFromString(serviceVersion);
        if (!version) {
            celix_bundleContext_log(
                component->context, CELIX_LOG_LEVEL_ERROR, "Cannot add interface with an invalid serviceVersion");
            celix_properties_destroy(properties);
            return CELIX_ILLEGAL_ARGUMENT;
        }
        celix_status_t rc = celix_properties_setVersionWithoutCopy(
            properties, CELIX_FRAMEWORK_SERVICE_VERSION, celix_steal_ptr(version));
        if (rc != CELIX_SUCCESS) {
            celix_bundleContext_log(
                component->context, CELIX_LOG_LEVEL_ERROR, "Cannot add interface with an invalid serviceVersion");
            celix_properties_destroy(properties);
            return CELIX_ILLEGAL_ARGUMENT;
        }
    }

    celix_properties_set(properties, CELIX_DM_COMPONENT_UUID, (char*)component->uuid);

    celixThreadMutex_lock(&component->mutex);
    interface->serviceName = celix_steal_ptr(name);
    interface->service = service;
    interface->properties = properties;
    interface->svcId = -1L;
    celix_arrayList_add(component->providedInterfaces, celix_steal_ptr(interface));
    if (celix_dmComponent_currentState(component) == CELIX_DM_CMP_STATE_TRACKING_OPTIONAL) {
        celix_dmComponent_registerServices(component, false);
    }
    celixThreadMutex_unlock(&component->mutex);

    return CELIX_SUCCESS;
}

celix_status_t component_removeInterface(celix_dm_component_t *component, const void* service) {
    return celix_dmComponent_removeInterface(component, service);
}

celix_status_t celix_dmComponent_removeInterface(celix_dm_component_t *component, const void* service) {
    celix_status_t status = CELIX_ILLEGAL_ARGUMENT;

    celixThreadMutex_lock(&component->mutex);
    int nof_interfaces = celix_arrayList_size(component->providedInterfaces);
    dm_interface_t* removedInterface = NULL;
    for (int i = 0; i < nof_interfaces; ++i) {
        dm_interface_t *interface = celix_arrayList_get(component->providedInterfaces, i);
        if (interface->service == service) {
            celix_arrayList_removeAt(component->providedInterfaces, i);
            removedInterface = interface;
            break;
        }
    }
    celixThreadMutex_unlock(&component->mutex);

    if (removedInterface != NULL) {
        celix_bundleContext_unregisterService(component->context, removedInterface->svcId);
        free(removedInterface->serviceName);
        free(removedInterface);
    }

    return status;
}

celix_status_t component_getInterfaces(celix_dm_component_t *component, celix_array_list_t **out) {
    return celix_dmComponent_getInterfaces(component, out);
}

celix_status_t celix_dmComponent_getInterfaces(celix_dm_component_t *component, celix_array_list_t **out) {
    celix_array_list_t* names = celix_arrayList_create();

    celixThreadMutex_lock(&component->mutex);
    int size = celix_arrayList_size(component->providedInterfaces);
    for (int i = 0; i < size; i += 1) {
        dm_interface_info_t* info = calloc(1, sizeof(*info));
        dm_interface_t *interface = celix_arrayList_get(component->providedInterfaces, i);
        info->name = celix_utils_strdup(interface->serviceName);
        info->properties = celix_properties_copy(interface->properties);
        celix_arrayList_add(names, info);
    }
    celixThreadMutex_unlock(&component->mutex);
    *out = names;

    return CELIX_SUCCESS;
}

celix_status_t celix_private_dmComponent_handleEvent(celix_dm_component_t *component, const celix_dm_event_t* event) {
    celix_status_t status = CELIX_SUCCESS;
    switch (event->eventType) {
        case CELIX_DM_EVENT_SVC_ADD:
            celix_dmComponent_handleAdd(component, event);
            break;
        case CELIX_DM_EVENT_SVC_REM:
            celix_dmComponent_handleRemove(component, event);
            break;
        case CELIX_DM_EVENT_SVC_SET:
            celix_dmComponent_handleSet(component, event);
            break;
        default:
            break;
    }
    return status;
}

static celix_status_t celix_dmComponent_suspend(celix_dm_component_t *component, celix_dm_service_dependency_t *dependency) {
	celix_status_t status = CELIX_SUCCESS;
    celixThreadMutex_lock(&component->mutex);
    celix_dmComponent_logTransition(component, celix_dmComponent_currentState(component),
                                    CELIX_DM_CMP_STATE_SUSPENDING);
    celix_dmComponent_setCurrentState(component, CELIX_DM_CMP_STATE_SUSPENDING);
    celix_dmComponent_unregisterServices(component, false);
    if (component->callbackStop != NULL ) {
        status = component->callbackStop(component->implementation);
    }
    if (status == CELIX_SUCCESS) {
        celix_dmComponent_logTransition(component, celix_dmComponent_currentState(component),
                                        CELIX_DM_CMP_STATE_SUSPENDED);
        celix_dmComponent_setCurrentState(component, CELIX_DM_CMP_STATE_SUSPENDED);
    } else {
        celix_bundleContext_log(component->context, CELIX_LOG_LEVEL_ERROR,
                                "Error stopping component %s (uuid=%s) using the stop callback. Disabling component.",
                                component->name,
                                component->uuid);
        celix_dmComponent_disableDirectly(component);
    }
    celixThreadMutex_unlock(&component->mutex);
	return status;
}

static celix_status_t celix_dmComponent_resume(celix_dm_component_t *component, celix_dm_service_dependency_t *dependency) {
	celix_status_t status = CELIX_SUCCESS;
    if (celix_dmComponent_currentState(component) == CELIX_DM_CMP_STATE_SUSPENDED) {
        celixThreadMutex_lock(&component->mutex);
        celix_dmComponent_logTransition(component, celix_dmComponent_currentState(component),
                                        CELIX_DM_CMP_STATE_RESUMING);
        celix_dmComponent_setCurrentState(component, CELIX_DM_CMP_STATE_RESUMING);
        if (component->callbackStart != NULL) {
            status = component->callbackStart(component->implementation);
        }
        if (status == CELIX_SUCCESS) {
            celix_dmComponent_registerServices(component, false);
            component->nrOfTimesResumed += 1;
            celix_dmComponent_logTransition(component, celix_dmComponent_currentState(component),
                                            CELIX_DM_CMP_STATE_TRACKING_OPTIONAL);
            celix_dmComponent_setCurrentState(component, CELIX_DM_CMP_STATE_TRACKING_OPTIONAL);
        } else {
            celix_bundleContext_log(component->context, CELIX_LOG_LEVEL_ERROR,
                                    "Error starting component %s (uuid=%s) using the start callback. Disabling component.",
                                    component->name,
                                    component->uuid);
            celix_dmComponent_disableDirectly(component);
        }
        celixThreadMutex_unlock(&component->mutex);
    }
    return status;
}

static bool celix_dmComponent_needsSuspend(celix_dm_component_t *component, const celix_dm_event_t* event) {
    if (celix_dmComponent_currentState(component) == CELIX_DM_CMP_STATE_TRACKING_OPTIONAL) {
        bool callbackConfigured = event->eventType == CELIX_DM_EVENT_SVC_SET ?
                                  celix_dmServiceDependency_isSetCallbackConfigured(event->dep) :
                                 /*add or rem*/ celix_dmServiceDependency_isAddRemCallbacksConfigured(event->dep);
        dm_service_dependency_strategy_t strategy = celix_dmServiceDependency_getStrategy(event->dep);
        bool suspendStrategy = strategy == DM_SERVICE_DEPENDENCY_STRATEGY_SUSPEND;
        return suspendStrategy && callbackConfigured;
    }
    return false;
}

static celix_status_t celix_dmComponent_handleEvent(celix_dm_component_t *component, const celix_dm_event_t* event, celix_status_t (*setAddOrRemFp)(celix_dm_service_dependency_t *dependency, void* svc, const celix_properties_t* props), const char *invokeName) {
    celix_bundleContext_log(component->context, CELIX_LOG_LEVEL_TRACE,
                            "Calling %s service for component %s (uuid=%s) on service dependency with type %s",
                            invokeName,
                            component->name,
                            component->uuid,
                            event->dep->serviceName);


    if (component->inTransition) {
        /* Note if the component is already in transition (stopping, starting, etc) then only remove the svc
         * function pointers. This can happen when with dependency loops or if a component also depends on a
         * services it provides. (e.g. during stopping the component a handle event will be created to remove the
         * service dependency).
         */
        setAddOrRemFp(event->dep, event->svc, event->props);
        return CELIX_SUCCESS;
    }

    bool eventHandled = false;
    if (event->eventType == CELIX_DM_EVENT_SVC_ADD || (event->eventType == CELIX_DM_EVENT_SVC_SET && event->svc != NULL)) {
        //note adding service or setting new service, so cmp will not be stopped in handleChange -> use suspend / resume now
        eventHandled = true;
        bool needSuspend = celix_dmComponent_needsSuspend(component, event);
        if (needSuspend) {
            celix_dmComponent_suspend(component, event->dep);
        }
        setAddOrRemFp(event->dep, event->svc, event->props);
        if (needSuspend) {
            celix_dmComponent_resume(component, event->dep);
        }
    };

    celix_dmComponent_handleChange(component);

    if (!eventHandled /*remove or set null*/) {
        //removing svc or set svc to null -> if still active check if suspend is needed before invoking
        bool needSuspend = celix_dmComponent_needsSuspend(component, event);
        if (needSuspend) {
            celix_dmComponent_suspend(component, event->dep);
        }
        setAddOrRemFp(event->dep, event->svc, event->props);
        if (needSuspend) {
            celix_dmComponent_resume(component, event->dep);
        }
    }

    return CELIX_SUCCESS;
}

static celix_status_t celix_dmComponent_handleAdd(celix_dm_component_t *component, const celix_dm_event_t* event) {
    return celix_dmComponent_handleEvent(component, event, celix_dmServiceDependency_invokeAdd, "add");
}

static celix_status_t celix_dmComponent_handleRemove(celix_dm_component_t *component, const celix_dm_event_t* event) {
    return celix_dmComponent_handleEvent(component, event, celix_dmServiceDependency_invokeRemove, "remove");
}

static celix_status_t celix_dmComponent_handleSet(celix_dm_component_t *component, const celix_dm_event_t* event) {
    return celix_dmComponent_handleEvent(component, event, celix_dmServiceDependency_invokeSet, "set");
}

/**
 * perform state transition. This function should be called with the component->mutex locked.
 */
static celix_status_t celix_dmComponent_enableDependencies(celix_dm_component_t *component) {
    for (int i = 0; i < celix_arrayList_size(component->dependencies); i++) {
        celix_dm_service_dependency_t *dependency = arrayList_get(component->dependencies, i);
        celix_dmServiceDependency_enable(dependency);
    }
    return CELIX_SUCCESS;
}

/**
 * perform state transition. This function should be called with the component->mutex locked.
 */
static celix_status_t celix_dmComponent_disableDependencies(celix_dm_component_t *component) {
    for (int i = 0; i < celix_arrayList_size(component->dependencies); i++) {
        celix_dm_service_dependency_t *dependency = arrayList_get(component->dependencies, i);
        celix_dmServiceDependency_disable(dependency);
    }
    return CELIX_SUCCESS;
}


/**
 * Calculate and handle state change.
 */
static void celix_dmComponent_handleChangeOnEventThread(void *data) {
    celix_dm_component_t* component = data;
    assert(celix_framework_isCurrentThreadTheEventLoop(celix_bundleContext_getFramework(component->context)));

    celixThreadMutex_lock(&component->mutex);
    celix_dm_component_state_t oldState;
    celix_dm_component_state_t newState;
    bool transition = false;
    do {
        oldState = celix_dmComponent_currentState(component);
        celix_status_t status = celix_dmComponent_calculateNewState(component, oldState, &newState);
        if (status == CELIX_SUCCESS) {
            transition = celix_dmComponent_performTransition(component, oldState, newState);
            if (transition) {
                celix_dmComponent_setCurrentState(component, newState);
            }
        }

        if (status != CELIX_SUCCESS) {
            break;
        }
    } while (transition);
    celixThreadMutex_unlock(&component->mutex);
}

static celix_status_t celix_dmComponent_handleChange(celix_dm_component_t *component) {
    celix_framework_t* fw = celix_bundleContext_getFramework(component->context);
    if (celix_framework_isCurrentThreadTheEventLoop(fw)) {
        celix_dmComponent_handleChangeOnEventThread(component);
    } else {
        celix_framework_fireGenericEvent(
                fw,
                -1,
                celix_bundleContext_getBundleId(component->context),
                "dm component handle change",
                component,
                celix_dmComponent_handleChangeOnEventThread,
                NULL,
                NULL);
    }
    return CELIX_SUCCESS;
}

/**
 * Calculate possible state change. This function should be called with the component->mutex locked.
 */
static celix_status_t celix_dmComponent_calculateNewState(celix_dm_component_t *component, celix_dm_component_state_t currentState, celix_dm_component_state_t *newState) {
    celix_status_t status = CELIX_SUCCESS;

    bool allResolved = celix_dmComponent_areAllRequiredServiceDependenciesResolved(component);
    if (currentState == CELIX_DM_CMP_STATE_INACTIVE) {
        if (component->isEnabled) {
            *newState = CELIX_DM_CMP_STATE_WAITING_FOR_REQUIRED;
        } else {
            *newState = currentState;
        }
    } else if (currentState == CELIX_DM_CMP_STATE_WAITING_FOR_REQUIRED) {
        if (!component->isEnabled) {
            *newState = CELIX_DM_CMP_STATE_INACTIVE;
        } else {
            if (allResolved) {
                *newState = CELIX_DM_CMP_STATE_INITIALIZING;
            } else {
                *newState = currentState;
            }
        }
    } else if (currentState == CELIX_DM_CMP_STATE_INITIALIZING) {
        *newState = CELIX_DM_CMP_STATE_INITIALIZED_AND_WAITING_FOR_REQUIRED;
    } else if (currentState == CELIX_DM_CMP_STATE_DEINITIALIZING) {
        *newState = CELIX_DM_CMP_STATE_INACTIVE;
    } else if (currentState == CELIX_DM_CMP_STATE_INITIALIZED_AND_WAITING_FOR_REQUIRED) {
        if (!component->isEnabled) {
            *newState = CELIX_DM_CMP_STATE_DEINITIALIZING;
        } else {
            if (allResolved) {
                *newState = CELIX_DM_CMP_STATE_STARTING;
            } else {
                *newState = currentState;
            }
        }
    } else if (currentState == CELIX_DM_CMP_STATE_STARTING) {
        *newState = CELIX_DM_CMP_STATE_TRACKING_OPTIONAL;
    } else if (currentState == CELIX_DM_CMP_STATE_STOPPING) {
        *newState = CELIX_DM_CMP_STATE_INITIALIZED_AND_WAITING_FOR_REQUIRED;
    } else if (currentState == CELIX_DM_CMP_STATE_TRACKING_OPTIONAL) {
        if (component->isEnabled && allResolved) {
            *newState = currentState;
        } else {
            *newState = CELIX_DM_CMP_STATE_STOPPING;
        }
    } else {
        //should not reach
        *newState = CELIX_DM_CMP_STATE_INACTIVE;
        status = CELIX_BUNDLE_EXCEPTION;
    }

    return status;
}

/**
 * perform state transition. This function should be called with the component->mutex locked.
 *
 * Function can also indicate if the component needs to be disabled. This can be needed if a stop or deinit fails,
 * because the component cannot be destroyed but apparently also not cleanly stopped
 */
static bool celix_dmComponent_performTransition(celix_dm_component_t *component, celix_dm_component_state_t currentState, celix_dm_component_state_t desiredState) {
    if (currentState == desiredState) {
        return false;
    }
    component->inTransition = true;

    celix_dmComponent_logTransition(component, currentState, desiredState);

    celix_status_t status = CELIX_SUCCESS;
    if (currentState == CELIX_DM_CMP_STATE_INACTIVE && desiredState == CELIX_DM_CMP_STATE_WAITING_FOR_REQUIRED) {
        celix_dmComponent_enableDependencies(component);
    } else if (currentState == CELIX_DM_CMP_STATE_WAITING_FOR_REQUIRED && desiredState == CELIX_DM_CMP_STATE_INITIALIZING) {
        //nop
    } else if (currentState == CELIX_DM_CMP_STATE_INITIALIZING && desiredState == CELIX_DM_CMP_STATE_INITIALIZED_AND_WAITING_FOR_REQUIRED) {
        if (component->callbackInit) {
            status = component->callbackInit(component->implementation);
        }
    } else if (currentState == CELIX_DM_CMP_STATE_INITIALIZED_AND_WAITING_FOR_REQUIRED && desiredState == CELIX_DM_CMP_STATE_DEINITIALIZING) {
        //nop
    } else if (currentState == CELIX_DM_CMP_STATE_DEINITIALIZING && desiredState == CELIX_DM_CMP_STATE_INACTIVE) {
        if (component->callbackDeinit) {
            status = component->callbackDeinit(component->implementation);
        }
        celix_dmComponent_disableDependencies(component);
    } else if (currentState == CELIX_DM_CMP_STATE_INITIALIZED_AND_WAITING_FOR_REQUIRED && desiredState == CELIX_DM_CMP_STATE_STARTING) {
        //nop
    } else if (currentState == CELIX_DM_CMP_STATE_STARTING && desiredState == CELIX_DM_CMP_STATE_TRACKING_OPTIONAL) {
        if (component->callbackStart) {
        	status = component->callbackStart(component->implementation);
        }
        if (status == CELIX_SUCCESS) {
            celix_dmComponent_registerServices(component, false);
            component->nrOfTimesStarted += 1;
        }
    } else if (currentState == CELIX_DM_CMP_STATE_TRACKING_OPTIONAL && desiredState == CELIX_DM_CMP_STATE_STOPPING) {
        //nop
    } else if (currentState == CELIX_DM_CMP_STATE_STOPPING && desiredState == CELIX_DM_CMP_STATE_INITIALIZED_AND_WAITING_FOR_REQUIRED) {
        celix_dmComponent_unregisterServices(component, false);
        if (component->callbackStop) {
        	status = component->callbackStop(component->implementation);
        }
    } else if (currentState == CELIX_DM_CMP_STATE_WAITING_FOR_REQUIRED && desiredState == CELIX_DM_CMP_STATE_INACTIVE) {
        celix_dmComponent_disableDependencies(component);
    } else {
        assert(false); //should not be reached.
    }

    bool transition = true;
    if (status != CELIX_SUCCESS) {
        celix_bundleContext_log(component->context, CELIX_LOG_LEVEL_ERROR,
                                "Error in component %s (uuid=%s) transition from %s to %s. Disabling component.",
                                component->name,
                                component->uuid,
                                celix_dmComponent_stateToString(currentState),
                                celix_dmComponent_stateToString(desiredState));

        //If a lifecycle method fails, the component will disable itself (but no higher level transition is needed anymore)
        transition = false;
        celix_dmComponent_disableDirectly(component);
    }

    component->inTransition = false;
    return transition;
}

/**
 * Check if all required dependencies are resolved. This function should be called with the component->mutex locked.
 */
static bool celix_dmComponent_areAllRequiredServiceDependenciesResolved(celix_dm_component_t *component) {
    bool allResolved = true;
    for (int i = 0; i < celix_arrayList_size(component->dependencies); i++) {
        celix_dm_service_dependency_t *dependency = celix_arrayList_get(component->dependencies, i);
        bool started = celix_dmServiceDependency_isTrackerOpen(dependency);
        bool required = celix_dmServiceDependency_isRequired(dependency);
        bool available = celix_dmServiceDependency_isAvailable(dependency);
        if (!started) {
            allResolved = false;
            break;
        }
        if (required && !available) {
            allResolved = false;
            break;
        }
    }
    return allResolved;
}

/**
 * Register component services (if not already registered).
 */
static celix_status_t celix_dmComponent_registerServices(celix_dm_component_t *component, bool needLock) {
    if (needLock) {
        celixThreadMutex_lock(&component->mutex);
    }
    for (int i = 0; i < celix_arrayList_size(component->providedInterfaces); i++) {
        dm_interface_t *interface = arrayList_get(component->providedInterfaces, i);
        if (interface->svcId == -1L) {
            celix_properties_t *regProps = celix_properties_copy(interface->properties);
            celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
            opts.properties = regProps;
            opts.svc = (void*)interface->service;
            opts.serviceName = interface->serviceName;
            celix_bundleContext_log(component->context, CELIX_LOG_LEVEL_TRACE,
                   "Async registering service %s for component %s (uuid=%s)",
                   interface->serviceName,
                   component->name,
                   component->uuid);
            long svcId = celix_bundleContext_registerServiceWithOptionsAsync(component->context, &opts);
            interface->svcId = svcId;
        }
    }
    if (needLock) {
        celixThreadMutex_unlock(&component->mutex);
    }

    return CELIX_SUCCESS;
}

/**
 * Unregister component services. This function should be called with the component->mutex locked.
 * If needLock is false, this function should be called with the component->mutex locked.
 */
static celix_status_t celix_dmComponent_unregisterServices(celix_dm_component_t *component, bool needLock) {
    celix_status_t status = CELIX_SUCCESS;

    if (needLock) {
        celixThreadMutex_lock(&component->mutex);
    }
    celix_array_list_t* ids = NULL;
    for (int i = 0; i < celix_arrayList_size(component->providedInterfaces); ++i) {
	    dm_interface_t *interface = arrayList_get(component->providedInterfaces, i);
	    if (interface->svcId == -1) {
	        continue;
	    }
	    if (ids == NULL) {
            ids = celix_arrayList_create();
	    }
	    celix_arrayList_addLong(ids, interface->svcId);
	    interface->svcId = -1L;
        celix_bundleContext_log(component->context, CELIX_LOG_LEVEL_TRACE,
               "Unregistering service %s for component %s (uuid=%s)",
               interface->serviceName,
               component->name,
               component->uuid);
    }
    if (needLock) {
        celixThreadMutex_unlock(&component->mutex);
    }


    if (ids != NULL) {
        if (!needLock /*already locked when calling*/) {
            celixThreadMutex_unlock(&component->mutex);
        }
        for (int i = 0; i < celix_arrayList_size(ids); ++i) {
            long svcId = celix_arrayList_getLong(ids, i);
            celix_bundleContext_unregisterService(component->context, svcId);
        }
        celix_arrayList_destroy(ids);
        if (!needLock) {
            celixThreadMutex_lock(&component->mutex);
        }
    }

    return status;
}

celix_status_t component_setCallbacks(celix_dm_component_t *component, celix_dm_cmp_lifecycle_fpt init, celix_dm_cmp_lifecycle_fpt start, celix_dm_cmp_lifecycle_fpt stop, celix_dm_cmp_lifecycle_fpt deinit) {
    return celix_dmComponent_setCallbacks(component, init, start, stop, deinit);
}

celix_status_t celix_dmComponent_setCallbacks(celix_dm_component_t *component, celix_dm_cmp_lifecycle_fpt init, celix_dm_cmp_lifecycle_fpt start, celix_dm_cmp_lifecycle_fpt stop, celix_dm_cmp_lifecycle_fpt deinit) {
	component->callbackInit = init;
	component->callbackStart = start;
	component->callbackStop = stop;
	component->callbackDeinit = deinit;
	return CELIX_SUCCESS;
}

celix_status_t component_setImplementation(celix_dm_component_t *component, void *implementation) {
    return celix_dmComponent_setImplementation(component, implementation);
}

celix_status_t celix_dmComponent_setImplementation(celix_dm_component_t *component, void *implementation) {
    component->implementation = implementation;
    return CELIX_SUCCESS;
}

void celix_dmComponent_setImplementationDestroyFunction(celix_dm_component_t* component, celix_dm_cmp_impl_destroy_fpt destroyFn) {
    component->implementationDestroyFn = destroyFn;
}

celix_status_t component_getBundleContext(celix_dm_component_t *component, bundle_context_pt *context) {
    *context = celix_dmComponent_getBundleContext(component);
    return *context == NULL ? CELIX_BUNDLE_EXCEPTION : CELIX_SUCCESS;
}

celix_bundle_context_t* celix_dmComponent_getBundleContext(celix_dm_component_t *component) {
	celix_bundle_context_t *result = NULL;
	if (component) {
		result = component->context;
	}
    return result;
}

celix_status_t component_getComponentInfo(celix_dm_component_t *component, celix_dm_component_info_t** out) {
    return celix_dmComponent_getComponentInfo(component, out);
}

celix_status_t celix_dmComponent_getComponentInfo(celix_dm_component_t *component, celix_dm_component_info_t** out) {
    celix_dm_component_info_t* info = calloc(1, sizeof(*info));
    info->dependency_list = celix_arrayList_create();
    celix_dmComponent_getInterfaces(component, &info->interfaces);

    celixThreadMutex_lock(&component->mutex);
    info->bundleId = celix_bundleContext_getBundleId(component->context);
    info->bundleSymbolicName = celix_bundleContext_getBundleSymbolicName(component->context, info->bundleId);
    info->active = false;
    info->id = celix_utils_strdup(component->uuid);
    info->name = celix_utils_strdup(component->name);
    info->nrOfTimesStarted = component->nrOfTimesStarted;
    info->nrOfTimesResumed = component->nrOfTimesResumed;
    celix_dm_component_state_t state = celix_dmComponent_currentState(component);
    info->state = celix_utils_strdup(celix_dmComponent_stateToString(state));
    info->active = celix_dmComponent_isActiveInternal(component);

    for (int i = 0; i < celix_arrayList_size(component->dependencies); i += 1) {
        celix_dm_service_dependency_t *dep = celix_arrayList_get(component->dependencies, i);
        dm_service_dependency_info_pt depInfo = celix_dmServiceDependency_createInfo(dep);
        celix_arrayList_add(info->dependency_list, depInfo);
    }

    celixThreadMutex_unlock(&component->mutex);

    *out = info;
    return CELIX_SUCCESS;
}


static void celix_dmComponent_printFullInfo(FILE *out, bool colors, celix_dm_component_info_t* compInfo) {
    const char *startColors = "";
    const char *endColors = "";
    if (colors) {
        startColors = compInfo->active ? CELIX_DM_PRINT_OK_COLOR : CELIX_DM_PRINT_NOK_COLOR;
        endColors = CELIX_DM_PRINT_END_COLOR;
    }
    fprintf(out, "%sComponent: Name=%s%s\n", startColors, compInfo->name, endColors);
    fprintf(out, "|- UUID   = %s\n", compInfo->id);
    fprintf(out, "|- Active = %s\n", compInfo->active ? "true" : "false");
    fprintf(out, "|- State  = %s\n", compInfo->state);
    fprintf(out, "|- Bundle = %li (%s)\n", compInfo->bundleId, compInfo->bundleSymbolicName);
    fprintf(out, "|- Nr of times started = %i\n", (int)compInfo->nrOfTimesStarted);
    fprintf(out, "|- Nr of times resumed = %i\n", (int)compInfo->nrOfTimesResumed);

    fprintf(out, "|- Interfaces (%d):\n", celix_arrayList_size(compInfo->interfaces));
    for (int interfCnt = 0; interfCnt < celix_arrayList_size(compInfo->interfaces); interfCnt++) {
        dm_interface_info_pt intfInfo = celix_arrayList_get(compInfo->interfaces, interfCnt);
        fprintf(out, "   |- %sInterface %i: %s%s\n", startColors, (interfCnt+1), intfInfo->name, endColors);

        hash_map_iterator_t iter = hashMapIterator_construct((hash_map_pt) intfInfo->properties);
        char *key = NULL;
        while ((key = hashMapIterator_nextKey(&iter)) != NULL) {
            fprintf(out, "      | %15s = %s\n", key, celix_properties_get(intfInfo->properties, key, "!ERROR!"));
        }
    }

    fprintf(out, "|- Dependencies (%d):\n", celix_arrayList_size(compInfo->dependency_list));
    for (int depCnt = 0; depCnt < celix_arrayList_size(compInfo->dependency_list); ++depCnt) {
        dm_service_dependency_info_pt dependency;
        dependency = celix_arrayList_get(compInfo->dependency_list, depCnt);
        const char *depStartColors = "";
        if (colors) {
            if (dependency->required) {
                depStartColors = dependency->available ? CELIX_DM_PRINT_OK_COLOR : CELIX_DM_PRINT_NOK_COLOR;
            } else {
                depStartColors = dependency->available ? CELIX_DM_PRINT_OK_COLOR : CELIX_DM_PRINT_WARNING_COLOR;
            }
        }
        fprintf(out, "   |- %sDependency %i: %s%s\n", depStartColors, (depCnt+1), dependency->serviceName == NULL ? "(any)" : dependency->serviceName, endColors);
        fprintf(out, "      | %15s = %s\n", "Available", dependency->available ? "true " : "false");
        fprintf(out, "      | %15s = %s\n", "Required", dependency->required ? "true " : "false");
        fprintf(out, "      | %15s = %s\n", "Version Range", dependency->versionRange == NULL ? "N/A" : dependency->versionRange);
        fprintf(out, "      | %15s = %s\n", "Filter", dependency->filter == NULL ? "N/A" : dependency->filter);
    }
    fprintf(out, "\n");
}

static void celix_dmComponent_printBasicInfo(FILE *out, bool colors, celix_dm_component_info_t* compInfo) {
    const char *startColors = "";
    const char *endColors = "";
    if (colors) {
        startColors = compInfo->active ? CELIX_DM_PRINT_OK_COLOR : CELIX_DM_PRINT_NOK_COLOR;
        endColors = CELIX_DM_PRINT_END_COLOR;
    }
    fprintf(out, "Component: Name=%s, ID=%s, %sActive=%s%s, State=%s, Bundle=%li (%s)\n",
            compInfo->name, compInfo->id, startColors, compInfo->active ? "true " : "false", endColors,
            compInfo->state, compInfo->bundleId, compInfo->bundleSymbolicName);
}


void celix_dmComponent_printComponentInfo(celix_dm_component_info_t* info, bool printFullInfo, bool useAnsiColors, FILE* stream) {
    if (printFullInfo) {
        celix_dmComponent_printFullInfo(stream, useAnsiColors, info);
    } else {
        celix_dmComponent_printBasicInfo(stream, useAnsiColors, info);
    }
}

void component_destroyComponentInfo(celix_dm_component_info_t* info) {
    return celix_dmComponent_destroyComponentInfo(info);
}

void celix_dmComponent_destroyComponentInfo(celix_dm_component_info_t* info) {
    int i;
    int size;
    if (info != NULL) {
        free(info->bundleSymbolicName);
        free(info->id);
        free(info->name);
        free(info->state);

        if (info->interfaces != NULL) {
            size = celix_arrayList_size(info->interfaces);
            for (i = 0; i < size; i += 1) {
                dm_interface_info_pt intfInfo = celix_arrayList_get(info->interfaces, i);
                free(intfInfo->name);
                celix_properties_destroy(intfInfo->properties);
                free(intfInfo);
            }
            celix_arrayList_destroy(info->interfaces);
        }
        if (info->dependency_list != NULL) {
            size = celix_arrayList_size(info->dependency_list);
            for (i = 0; i < size; i += 1) {
                dm_service_dependency_info_pt depInfo = celix_arrayList_get(info->dependency_list, i);
                dependency_destroyDependencyInfo(depInfo);
            }
            celix_arrayList_destroy(info->dependency_list);
        }
    }
    free(info);
}

static bool celix_dmComponent_isActiveInternal(celix_dm_component_t *component) {
    //precondition: mutex component->mutex taken.
    celix_dm_component_state_t state = celix_dmComponent_currentState(component);
    return
            state == CELIX_DM_CMP_STATE_STARTING ||
            state == CELIX_DM_CMP_STATE_TRACKING_OPTIONAL ||
            state == CELIX_DM_CMP_STATE_SUSPENDING ||
            state == CELIX_DM_CMP_STATE_SUSPENDED ||
            state == CELIX_DM_CMP_STATE_RESUMING ||
            state == CELIX_DM_CMP_STATE_STOPPING;
}

bool celix_dmComponent_isActive(celix_dm_component_t *component) {
    bool isActivate = celix_dmComponent_isActiveInternal(component);
    return isActivate;
}

const char* celix_dmComponent_stateToString(celix_dm_component_state_t state) {
    switch(state) {
        case CELIX_DM_CMP_STATE_WAITING_FOR_REQUIRED:
            return "WAITING_FOR_REQUIRED";
        case CELIX_DM_CMP_STATE_INITIALIZING:
            return "INITIALIZING";
        case CELIX_DM_CMP_STATE_DEINITIALIZING:
            return "DEINITIALIZING";
        case CELIX_DM_CMP_STATE_INITIALIZED_AND_WAITING_FOR_REQUIRED:
            return "INITIALIZED_AND_WAITING_FOR_REQUIRED";
        case CELIX_DM_CMP_STATE_STARTING:
            return "STARTING";
        case CELIX_DM_CMP_STATE_STOPPING:
            return "STOPPING";
        case CELIX_DM_CMP_STATE_TRACKING_OPTIONAL:
            return "TRACKING_OPTIONAL";
        case CELIX_DM_CMP_STATE_SUSPENDING:
            return "SUSPENDING";
        case CELIX_DM_CMP_STATE_SUSPENDED:
            return "SUSPENDED";
        case CELIX_DM_CMP_STATE_RESUMING:
            return "RESUMING";
        default: //only CELIX_DM_CMP_STATE_INACTIVE left
            return "INACTIVE";
    }
}


static void celix_dmComponent_logTransition(celix_dm_component_t* cmp, celix_dm_component_state_t currentState, celix_dm_component_state_t desiredState) {
    celix_bundleContext_log(cmp->context,
    CELIX_LOG_LEVEL_TRACE,
        "Component transition from %s -> %s for '%s' (uuid=%s).",
        celix_dmComponent_stateToString(currentState),
        celix_dmComponent_stateToString(desiredState),
        cmp->name,
        cmp->uuid);
}
