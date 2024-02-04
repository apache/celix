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
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
/**
 * topology_manager.c
 *
 *  \date       Sep 29, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include <uuid/uuid.h>

#include "topology_manager.h"
#include "celix_build_assert.h"
#include "celix_long_hash_map.h"
#include "celix_stdlib_cleanup.h"
#include "bundle_context.h"
#include "celix_compiler.h"
#include "celix_constants.h"
#include "bundle.h"
#include "remote_service_admin.h"
#include "remote_constants.h"
#include "filter.h"
#include "listener_hook_service.h"
#include "utils.h"
#include "service_reference.h"
#include "service_registration.h"
#include "celix_log_helper.h"
#include "topology_manager.h"
#include "scope.h"
#include "hash_map.h"
#include "celix_array_list.h"


//The prefix of the config property which is used to store the interfaces of a port. e.g. CELIX_RSA_INTERFACES_OF_PORT_8080. The value is a comma-separated list of interface names.
#define CELIX_RSA_INTERFACES_OF_PORT_PREFIX "CELIX_RSA_INTERFACES_OF_PORT_"

typedef struct celix_rsa_service_entry {
    remote_service_admin_service_t* rsa;
    bool dynamicIpSupport;
} celix_rsa_service_entry_t;

typedef struct celix_exported_service_entry {
    service_reference_pt reference;
    celix_long_hash_map_t* registrations; //key:rsa service id, val:celix_array_list_t<export_registration_t*>
} celix_exported_service_entry_t;

struct topology_manager {
	celix_bundle_context_t *context;

    celix_long_hash_map_t* rsaMap;//key:rsa service id, val:celix_rsa_service_entry_t*
    celix_long_hash_map_t* dynamicIpEndpoints;//key:rsa service id, val: celix_long_hash_map_t<exported service id, celix_array_list_t<endpoint_description_t*>>
    celix_long_hash_map_t* networkIfNames;//key:server port, val:celix_array_list_t<char*>. a list of network interface names

	hash_map_pt listenerList;

    celix_long_hash_map_t* exportedServices;//key:service id, val:celix_exported_service_entry_t*

	hash_map_pt importedServices;

	bool closed;

	//The mutex is used to protect rsaList,listenerList,exportedServices,importedServices,closed,and their related operations.
	celix_thread_mutex_t lock;

	scope_pt scope;

	celix_log_helper_t *loghelper;
};

celix_status_t topologyManager_exportScopeChanged(void *handle, char *service_name);
celix_status_t topologyManager_importScopeChanged(void *handle, char *service_name);
static celix_status_t topologyManager_notifyListenersEndpointAdded(topology_manager_pt manager, remote_service_admin_service_t *rsa, celix_array_list_t *registrations);
static celix_status_t topologyManager_notifyListenersEndpointRemoved(topology_manager_pt manager, remote_service_admin_service_t *rsa, export_registration_t *export);

static celix_status_t topologyManager_getEndpointDescriptionForExportRegistration(remote_service_admin_service_t *rsa, export_registration_t *export, endpoint_description_t **endpoint);
static celix_status_t topologyManager_addImportedService_nolock(void *handle, endpoint_description_t *endpoint, char *matchedFilter);
static celix_status_t topologyManager_removeImportedService_nolock(void *handle, endpoint_description_t *endpoint, char *matchedFilter);
static celix_status_t topologyManager_addExportedService_nolock(void * handle, service_reference_pt reference);
static void topologyManager_removeExportedService_nolock(void * handle, service_reference_pt reference);

celix_status_t topologyManager_create(celix_bundle_context_t *context, celix_log_helper_t *logHelper, topology_manager_pt *manager, void **scope) {
	celix_status_t status = CELIX_SUCCESS;

	celix_autofree topology_manager_t* tm = *manager = calloc(1, sizeof(**manager));
	if (tm == NULL) {
        celix_logHelper_error(logHelper, "TOPOLOGY_MANAGER: Error allocating memory for topology_manager_t.");
		return CELIX_ENOMEM;
	}
	tm->context = context;
    tm->loghelper = logHelper;
    tm->closed = false;

	status = celixThreadMutex_create(&tm->lock, NULL);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(logHelper, "TOPOLOGY_MANAGER: Error creating mutex.");
        return status;
    }
    celix_autoptr(celix_thread_mutex_t) lock = &tm->lock;
    celix_autoptr(celix_long_hash_map_t) rsaMap = tm->rsaMap = celix_longHashMap_create();
    if (rsaMap == NULL) {
        celix_logHelper_logTssErrors(logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(logHelper, "TOPOLOGY_MANAGER: Error creating long hash map for rsa services.");
        return CELIX_ENOMEM;
    }
    celix_autoptr(celix_long_hash_map_t) dynamicIpEndpoints = tm->dynamicIpEndpoints = celix_longHashMap_create();
    if (dynamicIpEndpoints == NULL) {
        celix_logHelper_logTssErrors(logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(logHelper, "TOPOLOGY_MANAGER: Error creating long hash map for dynamic ip endpoints.");
        return CELIX_ENOMEM;
    }
    celix_autoptr(celix_long_hash_map_t) networkIfNames = tm->networkIfNames = celix_longHashMap_create();
    if (networkIfNames == NULL) {
        celix_logHelper_logTssErrors(logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(logHelper, "TOPOLOGY_MANAGER: Error creating long hash map for network interface names.");
        return CELIX_ENOMEM;
    }
    celix_autoptr(celix_long_hash_map_t) exportedServices = tm->exportedServices = celix_longHashMap_create();
    if (exportedServices == NULL) {
        celix_logHelper_logTssErrors(logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(logHelper, "TOPOLOGY_MANAGER: Error creating long hash map for exported services.");
        return CELIX_ENOMEM;
    }

    //TODO remove deprecated hashMap
	(*manager)->listenerList = hashMap_create(serviceReference_hashCode, NULL, serviceReference_equals2, NULL);
	(*manager)->importedServices = hashMap_create(NULL, NULL, NULL, NULL);

    status = scope_scopeCreate(tm, &tm->scope);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(logHelper, "TOPOLOGY_MANAGER: Error creating scope.");
        hashMap_destroy(tm->importedServices, false, false);
        hashMap_destroy(tm->listenerList, false, false);
        return status;
    }
	scope_setExportScopeChangedCallback(tm->scope, topologyManager_exportScopeChanged);
	scope_setImportScopeChangedCallback(tm->scope, topologyManager_importScopeChanged);
	*scope = tm->scope;

    celix_steal_ptr(exportedServices);
    celix_steal_ptr(networkIfNames);
    celix_steal_ptr(dynamicIpEndpoints);
    celix_steal_ptr(rsaMap);
    celix_steal_ptr(lock);
    celix_steal_ptr(tm);

	return status;
}

celix_status_t topologyManager_destroy(topology_manager_pt manager) {
	celix_status_t status = CELIX_SUCCESS;

	scope_scopeDestroy(manager->scope);

	celixThreadMutex_lock(&manager->lock);

	hashMap_destroy(manager->importedServices, false, false);
	hashMap_destroy(manager->listenerList, false, false);

    assert(celix_longHashMap_size(manager->exportedServices) == 0);
    celix_longHashMap_destroy(manager->exportedServices);
    CELIX_LONG_HASH_MAP_ITERATE(manager->networkIfNames, iter) {
        celix_array_list_t* ifNames = iter.value.ptrValue;
        for (int i = 0; i < celix_arrayList_size(ifNames); ++i) {
            free(celix_arrayList_get(ifNames, i));
        }
        celix_arrayList_destroy(ifNames);
    }
    celix_longHashMap_destroy(manager->networkIfNames);
    assert(celix_longHashMap_size(manager->dynamicIpEndpoints) == 0);
    celix_longHashMap_destroy(manager->dynamicIpEndpoints);
    assert(celix_longHashMap_size(manager->rsaMap) == 0);
    celix_longHashMap_destroy(manager->rsaMap);

	celixThreadMutex_unlock(&manager->lock);
	celixThreadMutex_destroy(&manager->lock);

	free(manager);

	return status;
}

celix_status_t topologyManager_closeImports(topology_manager_pt manager) {
	celix_status_t status;

	status = celixThreadMutex_lock(&manager->lock);

	manager->closed = true;

	hash_map_iterator_pt iter = hashMapIterator_create(manager->importedServices);

	while (hashMapIterator_hasNext(iter)) {
		hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
		endpoint_description_t *ep = hashMapEntry_getKey(entry);
		hash_map_pt imports = hashMapEntry_getValue(entry);

		if (imports != NULL) {
			celix_logHelper_log(manager->loghelper, CELIX_LOG_LEVEL_INFO, "TOPOLOGY_MANAGER: Remove imported service (%s; %s).", ep->serviceName, ep->id);
			hash_map_iterator_pt importsIter = hashMapIterator_create(imports);

			while (hashMapIterator_hasNext(importsIter)) {
				hash_map_entry_pt entry = hashMapIterator_nextEntry(importsIter);

				remote_service_admin_service_t *rsa = hashMapEntry_getKey(entry);
				import_registration_t *import = hashMapEntry_getValue(entry);

				status = rsa->importRegistration_close(rsa->admin, import);
				if (status == CELIX_SUCCESS) {
					hashMapIterator_remove(importsIter);
				}
			}
			hashMapIterator_destroy(importsIter);

			hashMapIterator_remove(iter);

			hashMap_destroy(imports, false, false);
		}
	}
	hashMapIterator_destroy(iter);

	status = celixThreadMutex_unlock(&manager->lock);

	return status;
}

celix_status_t topologyManager_rsaAdding(void * handle, service_reference_pt reference, void **service) {
	celix_status_t status;
	topology_manager_pt manager = (topology_manager_pt) handle;

	status = bundleContext_getService(manager->context, reference, service);

	return status;
}

static celix_array_list_t*  topologyManager_getNetworkInterfacesForPort(topology_manager_t* tm, unsigned int port) {
    celix_status_t status = CELIX_SUCCESS;
    celix_array_list_t* ifNames = celix_longHashMap_get(tm->networkIfNames, port);
    if (ifNames == NULL) {
        char ifNamesKey[64];
        CELIX_BUILD_ASSERT(sizeof(ifNamesKey) >= (sizeof(CELIX_RSA_INTERFACES_OF_PORT_PREFIX) + 10));//10 is max int len
        (void)snprintf(ifNamesKey, sizeof(ifNamesKey), "%s%u", CELIX_RSA_INTERFACES_OF_PORT_PREFIX, port);
        const char* ifNamesStr = celix_bundleContext_getProperty(tm->context, ifNamesKey, NULL);
        if (ifNamesStr == NULL) {
            celix_logHelper_error(tm->loghelper, "TOPOLOGY_MANAGER: Error getting interface names from config properties.");
            return NULL;
        }
        celix_autoptr(celix_array_list_t) _ifNames = celix_arrayList_create();
        if (_ifNames == NULL) {
            celix_logHelper_error(tm->loghelper, "TOPOLOGY_MANAGER: Error creating array list for interface names.");
            return NULL;
        }
        celix_autofree char* ifNamesStrCopy = celix_utils_strdup(ifNamesStr);
        if (ifNamesStrCopy == NULL) {
            celix_logHelper_error(tm->loghelper, "TOPOLOGY_MANAGER: Error copying interface names string.");
            return NULL;
        }
        char* savePtr = NULL;
        char* token = strtok_r(ifNamesStrCopy, ",", &savePtr);
        while (token != NULL && status == CELIX_SUCCESS) {
            celix_autofree char* ifname = celix_utils_strdup(token);
            if (ifname == NULL) {
                status = CELIX_ENOMEM;
                celix_logHelper_error(tm->loghelper, "TOPOLOGY_MANAGER: Error copying interface name.");
                continue;
            }
            status = celix_arrayList_add(_ifNames, ifname);
            if (status != CELIX_SUCCESS) {
                celix_logHelper_error(tm->loghelper, "TOPOLOGY_MANAGER: Error adding interface name to array list.");
                continue;
            }
            celix_steal_ptr(ifname);
            token = strtok_r(NULL, ",", &savePtr);
        }

        CELIX_DO_IF(status, status = celix_longHashMap_put(tm->networkIfNames, port, _ifNames));

        if (status != CELIX_SUCCESS) {
            for (int i = 0; i < celix_arrayList_size(_ifNames); ++i) {
                free(celix_arrayList_get(_ifNames, i));
            }
            return NULL;
        }
        ifNames = celix_steal_ptr(_ifNames);
    }
    return ifNames;
}

static endpoint_description_t* topologyManager_createEndpointWithNetworkInterface(topology_manager_t* tm, const char* ifname,
                                                                         endpoint_description_t* exportedRegEndpoint) {
    celix_autoptr(endpoint_description_t) endpoint = endpointDescription_clone(exportedRegEndpoint);
    if (endpoint == NULL) {
        celix_logHelper_error(tm->loghelper, "TOPOLOGY_MANAGER: Error cloning endpoint description.");
        return NULL;
    }
    celix_status_t status = celix_properties_set(endpoint->properties, CELIX_RSA_EXPORTED_ENDPOINT_EXPOSURE_INTERFACE, ifname);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_logTssErrors(tm->loghelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(tm->loghelper, "TOPOLOGY_MANAGER: Error setting property %s.", CELIX_RSA_EXPORTED_ENDPOINT_EXPOSURE_INTERFACE);
        return NULL;
    }
    uuid_t endpointUuid;
    uuid_generate(endpointUuid);
    char endpointUuidStr[37];
    uuid_unparse_lower(endpointUuid, endpointUuidStr);
    status = celix_properties_set(endpoint->properties, CELIX_RSA_ENDPOINT_ID, endpointUuidStr);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_logTssErrors(tm->loghelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(tm->loghelper, "TOPOLOGY_MANAGER: Error setting property %s.", CELIX_RSA_ENDPOINT_ID);
        return NULL;
    }
    endpoint->id = celix_properties_get(endpoint->properties, CELIX_RSA_ENDPOINT_ID, "");
    status = celix_properties_set(endpoint->properties, CELIX_RSA_IP_ADDRESSES, "");//need to be filled in by discovery implementation
    if (status) {
        celix_logHelper_logTssErrors(tm->loghelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(tm->loghelper, "TOPOLOGY_MANAGER: Error setting property %s.", CELIX_RSA_IP_ADDRESSES);
        return NULL;
    }
    return celix_steal_ptr(endpoint);
}

static celix_status_t topologyManager_generateEndpointsForDynamicIpExportedRegistration(topology_manager_t* tm, endpoint_description_t* exportedRegEndpoint,
                                                celix_array_list_t* endpoints) {
    celix_status_t status = CELIX_SUCCESS;
    int port = celix_properties_getAsLong(exportedRegEndpoint->properties, CELIX_RSA_PORT, -1);
    if (port < 0) {
        celix_logHelper_error(tm->loghelper, "TOPOLOGY_MANAGER: Error getting port from dynamic ip endpoint description.");
        return CELIX_ILLEGAL_ARGUMENT;
    }
    celix_array_list_t* ifNames = topologyManager_getNetworkInterfacesForPort(tm, port);
    if (ifNames == NULL) {
        celix_logHelper_error(tm->loghelper, "TOPOLOGY_MANAGER: Error getting network interface names for port %d.", port);
        return CELIX_ILLEGAL_STATE;
    }

    int size = celix_arrayList_size(ifNames);
    for (int i = 0; i < size; ++i) {
        const char* ifname = celix_arrayList_get(ifNames, i);
        celix_autoptr(endpoint_description_t) endpoint = topologyManager_createEndpointWithNetworkInterface(tm, ifname, exportedRegEndpoint);
        if (endpoint == NULL) {
            celix_logHelper_error(tm->loghelper, "TOPOLOGY_MANAGER: Error creating endpoint for service %s on %s", exportedRegEndpoint->serviceName, ifname);
            continue;
        }
        status = celix_arrayList_add(endpoints, endpoint);
        if (status != CELIX_SUCCESS) {
            celix_logHelper_error(tm->loghelper, "TOPOLOGY_MANAGER: Error adding endpoint for service %s on %s", exportedRegEndpoint->serviceName, ifname);
            continue;
        }
        celix_steal_ptr(endpoint);
    }
    return CELIX_SUCCESS;
}

static void topologyManager_notifyListenersDynamicIpEndpointAdded(topology_manager_t* tm, celix_array_list_t* endpoints) {
    celix_status_t status = CELIX_SUCCESS;

    hash_map_iterator_pt iter = hashMapIterator_create(tm->listenerList);
    while (hashMapIterator_hasNext(iter)) {
        service_reference_pt reference = hashMapIterator_nextKey(iter);

        const char *interfaceSpecEndpointSupport = NULL;
        serviceReference_getProperty(reference, CELIX_RSA_DISCOVERY_INTERFACE_SPECIFIC_ENDPOINTS_SUPPORT, &interfaceSpecEndpointSupport);
        if (interfaceSpecEndpointSupport == NULL || strcmp(interfaceSpecEndpointSupport, "true") != 0) {
            continue;
        }

        const char* scope = NULL;
        serviceReference_getProperty(reference, CELIX_RSA_ENDPOINT_LISTENER_SCOPE, &scope);
        celix_autoptr(celix_filter_t) filter = celix_filter_create(scope);
        if (filter == NULL) {
            celix_logHelper_logTssErrors(tm->loghelper, CELIX_LOG_LEVEL_ERROR);
            celix_logHelper_error(tm->loghelper, "TOPOLOGY_MANAGER: Error creating filter for endpoint listener.");
            continue;
        }

        endpoint_listener_t *epl = NULL;
        status = bundleContext_getService(tm->context, reference, (void **) &epl);
        if (status != CELIX_SUCCESS) {
            celix_logHelper_error(tm->loghelper, "TOPOLOGY_MANAGER: Error getting endpoint listener.");
            continue;
        }
        int size = celix_arrayList_size(endpoints);
        for (int i = 0; i < size; i++) {
            endpoint_description_t* endpoint  = celix_arrayList_get(endpoints, i);
            if (celix_filter_match(filter, endpoint->properties)) {
                status = epl->endpointAdded(epl->handle, endpoint, (char*)scope);
                if (status != CELIX_SUCCESS) {
                    celix_logHelper_error(tm->loghelper, "TOPOLOGY_MANAGER: Failed to add endpoint to endpoint listener.");
                }
            }
        }
        bundleContext_ungetService(tm->context, reference, NULL);
    }
    hashMapIterator_destroy(iter);

    return;
}

static void topologyManager_notifyListenersDynamicIpEndpointRemoved(topology_manager_t* tm, celix_array_list_t* endpoints) {
    celix_status_t status = CELIX_SUCCESS;

    hash_map_iterator_pt iter = hashMapIterator_create(tm->listenerList);
    while (hashMapIterator_hasNext(iter)) {
        endpoint_listener_t *epl = NULL;
        service_reference_pt reference = hashMapIterator_nextKey(iter);

        status = bundleContext_getService(tm->context, reference, (void **) &epl);
        if (status != CELIX_SUCCESS) {
            celix_logHelper_error(tm->loghelper, "TOPOLOGY_MANAGER: Error getting endpoint listener.");
            continue;
        }
        int size = celix_arrayList_size(endpoints);
        for (int i = 0; i < size; i++) {
            endpoint_description_t* endpoint  = celix_arrayList_get(endpoints, i);
            status = epl->endpointRemoved(epl->handle, endpoint, NULL);
            if (status != CELIX_SUCCESS) {
                celix_logHelper_error(tm->loghelper, "TOPOLOGY_MANAGER: Failed to remove endpoint to endpoint listener.");
            }
        }
        bundleContext_ungetService(tm->context, reference, NULL);
    }
    hashMapIterator_destroy(iter);

    return;
}

static celix_long_hash_map_t* topologyManager_getDynamicIpEndpointMapForRsa(topology_manager_t* tm, long rsaSvcId) {
    celix_autoptr(celix_long_hash_map_t) endpointMap = celix_longHashMap_get(tm->dynamicIpEndpoints, rsaSvcId);
    if (endpointMap != NULL) {
        return celix_steal_ptr(endpointMap);
    }

    endpointMap = celix_longHashMap_create();
    if (endpointMap == NULL) {
        celix_logHelper_error(tm->loghelper, "TOPOLOGY_MANAGER: Error creating long hash map for exported services.");
        return NULL;
    }
    celix_status_t status = celix_longHashMap_put(tm->dynamicIpEndpoints, rsaSvcId, endpointMap);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_logTssErrors(tm->loghelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(tm->loghelper, "TOPOLOGY_MANAGER: Error adding exported services to map.");
        return NULL;
    }
    return celix_steal_ptr(endpointMap);
}

static void topologyManager_addDynamicIpEndpointsForExportedService(topology_manager_t* tm, long rsaSvcId, long exportedSvcId,
                                                                    celix_array_list_t *registrations) {
    celix_status_t status = CELIX_SUCCESS;
    int regSize = celix_arrayList_size(registrations);
    if (regSize == 0) {
        return;
    }
    celix_long_hash_map_t* endpointMap = topologyManager_getDynamicIpEndpointMapForRsa(tm, rsaSvcId);
    if (endpointMap == NULL) {
        return;
    }

    celix_array_list_t* endpointList = celix_arrayList_create();
    if (endpointList == NULL) {
        celix_logHelper_error(tm->loghelper, "TOPOLOGY_MANAGER: Error creating array list for dynamic ip endpoints.");
        return;
    }
    status = celix_longHashMap_put(endpointMap, exportedSvcId, endpointList);
    if (status != CELIX_SUCCESS) {
        celix_arrayList_destroy(endpointList);
        celix_logHelper_logTssErrors(tm->loghelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(tm->loghelper, "TOPOLOGY_MANAGER: Error adding dynamic ip endpoints list to map.");
        return;
    }

    celix_rsa_service_entry_t* rsaSvcEntry = celix_longHashMap_get(tm->rsaMap, rsaSvcId);
    assert(rsaSvcEntry != NULL);//It must be not null, because the rsa service is already added to the rsaMap.
    for (int i = 0; i < regSize; ++i) {
        export_registration_t* exportReg = celix_arrayList_get(registrations, i);
        endpoint_description_t* exportedRegEndpoint = NULL;
        status = topologyManager_getEndpointDescriptionForExportRegistration(rsaSvcEntry->rsa, exportReg, &exportedRegEndpoint);
        if (status != CELIX_SUCCESS) {
            celix_logHelper_error(tm->loghelper, "TOPOLOGY_MANAGER: Error getting endpoint description for exported registration.");
            continue;
        }
        status = topologyManager_generateEndpointsForDynamicIpExportedRegistration(tm, exportedRegEndpoint, endpointList);
        if (status != CELIX_SUCCESS) {
            celix_logHelper_error(tm->loghelper, "TOPOLOGY_MANAGER: Error generating endpoints for dynamic ip exported registration");
        }
    }

    if (celix_arrayList_size(endpointList) > 0) {
        topologyManager_notifyListenersDynamicIpEndpointAdded(tm, endpointList);
    }

    return;
}

static void topologyManager_removeDynamicIpEndpointsForExportedService(topology_manager_t* tm, long rsaSvcId, long exportedSvcId) {
    celix_long_hash_map_t* endpointMap = celix_longHashMap_get(tm->dynamicIpEndpoints, rsaSvcId);
    if (endpointMap == NULL) {
        return;
    }
    celix_array_list_t* endpointList = celix_longHashMap_get(endpointMap, exportedSvcId);
    if (endpointList != NULL) {
        topologyManager_notifyListenersDynamicIpEndpointRemoved(tm, endpointList);
        for (int i = 0; i < celix_arrayList_size(endpointList); ++i) {
            endpoint_description_t* endpoint = celix_arrayList_get(endpointList, i);
            endpointDescription_destroy(endpoint);
        }
        celix_longHashMap_remove(endpointMap, exportedSvcId);
        celix_arrayList_destroy(endpointList);
    }

    if (celix_longHashMap_size(endpointMap) == 0) {
        celix_longHashMap_remove(tm->dynamicIpEndpoints, rsaSvcId);
        celix_longHashMap_destroy(endpointMap);
    }
    return;
}

celix_status_t topologyManager_rsaAdded(void * handle, service_reference_pt rsaSvcRef, void * service) {
	topology_manager_pt manager = (topology_manager_pt) handle;
	celix_properties_t *serviceProperties = NULL;
	remote_service_admin_service_t *rsa = (remote_service_admin_service_t *) service;
	celix_logHelper_log(manager->loghelper, CELIX_LOG_LEVEL_INFO, "TOPOLOGY_MANAGER: Added RSA");

    bool dynamicIpSupport = false;
    const char *dynamicIpSupportStr = NULL;
    serviceReference_getProperty(rsaSvcRef, CELIX_RSA_DYNAMIC_IP_SUPPORT, &dynamicIpSupportStr);
    if (dynamicIpSupportStr != NULL && celix_utils_stringEquals(dynamicIpSupportStr, "true")) {
        dynamicIpSupport = true;
    }

    celix_autofree celix_rsa_service_entry_t* rsaSvcEntry = calloc(1, sizeof(*rsaSvcEntry));
    if (rsaSvcEntry == NULL) {
        return CELIX_ENOMEM;
    }
    rsaSvcEntry->dynamicIpSupport = dynamicIpSupport;
    rsaSvcEntry->rsa = rsa;

    celix_auto(celix_mutex_lock_guard_t) lockGuard = celixMutexLockGuard_init(&manager->lock);

    long rsaSvcId = serviceReference_getServiceId(rsaSvcRef);
    celix_status_t ret = celix_longHashMap_put(manager->rsaMap, rsaSvcId, rsaSvcEntry);
    if (ret != CELIX_SUCCESS) {
        celix_logHelper_logTssErrors(manager->loghelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(manager->loghelper, "TOPOLOGY_MANAGER: Error adding rsa service entry to map.");
        return ret;
    }
    celix_steal_ptr(rsaSvcEntry);

    // add already imported services to new rsa
    hash_map_iterator_pt importedServicesIterator = hashMapIterator_create(manager->importedServices);

    while (hashMapIterator_hasNext(importedServicesIterator)) {
        hash_map_entry_pt entry = hashMapIterator_nextEntry(importedServicesIterator);
        endpoint_description_t *endpoint = hashMapEntry_getKey(entry);
        if (scope_allowImport(manager->scope, endpoint)) {
            import_registration_t *import = NULL;
            celix_status_t status = rsa->importService(rsa->admin, endpoint, &import);
            if (status == CELIX_SUCCESS) {
                hash_map_pt imports = hashMapEntry_getValue(entry);

                if (imports == NULL) {
                    imports = hashMap_create(NULL, NULL, NULL, NULL);
                    hashMap_put(manager->importedServices, endpoint, imports);
                }

                hashMap_put(imports, service, import);
            }
        }
    }

    hashMapIterator_destroy(importedServicesIterator);

	// add already exported services to new rsa
    CELIX_LONG_HASH_MAP_ITERATE(manager->exportedServices, iter) {
        celix_exported_service_entry_t* svcEntry = iter.value.ptrValue;
        service_reference_pt reference = svcEntry->reference;
        long serviceId = iter.key;
        char serviceIdStr[32];
        (void)snprintf(serviceIdStr, sizeof(serviceIdStr), "%li", serviceId);

        scope_getExportProperties(manager->scope, reference, &serviceProperties);

        celix_autoptr(celix_array_list_t) registrations = NULL;
        celix_status_t status = rsa->exportService(rsa->admin, serviceIdStr, serviceProperties, &registrations);
        if (status != CELIX_SUCCESS) {
            celix_logHelper_error(manager->loghelper, "TOPOLOGY_MANAGER: Error exporting service.");
            continue;
        }
        if (celix_longHashMap_put(svcEntry->registrations, rsaSvcId, registrations) != CELIX_SUCCESS) {
            celix_logHelper_error(manager->loghelper, "TOPOLOGY_MANAGER: Error adding exported endpoints to map.");
            for (int i = 0; i < celix_arrayList_size(registrations); ++i) {
                export_registration_t* export = celix_arrayList_get(registrations, i);
                rsa->exportRegistration_close(rsa->admin, export);
            }
            continue;
        }
        if (dynamicIpSupport) {
            topologyManager_addDynamicIpEndpointsForExportedService(manager, rsaSvcId,serviceId, registrations);
        } else {
            topologyManager_notifyListenersEndpointAdded(manager, rsa, registrations);
        }
        celix_steal_ptr(registrations);
    }

	return CELIX_SUCCESS;
}

celix_status_t topologyManager_rsaModified(void * handle, service_reference_pt reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;

	// Nop...

	return status;
}

celix_status_t topologyManager_rsaRemoved(void * handle, service_reference_pt reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;
	topology_manager_pt manager = (topology_manager_pt) handle;
	remote_service_admin_service_t *rsa = (remote_service_admin_service_t *) service;
    long rsaSvcId = serviceReference_getServiceId(reference);

	celixThreadMutex_lock(&manager->lock);

    CELIX_LONG_HASH_MAP_ITERATE(manager->exportedServices, iter) {
        celix_exported_service_entry_t* svcEntry = iter.value.ptrValue;
        service_reference_pt exportSvcRef = svcEntry->reference;
        long exportedSvcId = serviceReference_getServiceId(exportSvcRef);

        topologyManager_removeDynamicIpEndpointsForExportedService(manager, rsaSvcId, exportedSvcId);

		celix_array_list_t *exports_list = (celix_array_list_t *)celix_longHashMap_get(svcEntry->registrations, rsaSvcId);
		if (exports_list != NULL) {
			int exportsIter = 0;
			int exportListSize = celix_arrayList_size(exports_list);
			for (exportsIter = 0; exportsIter < exportListSize; exportsIter++) {
				export_registration_t *export = celix_arrayList_get(exports_list, exportsIter);
				topologyManager_notifyListenersEndpointRemoved(manager, rsa, export);
				rsa->exportRegistration_close(rsa->admin, export);
			}
			celix_arrayList_destroy(exports_list);
            celix_longHashMap_remove(svcEntry->registrations, rsaSvcId);
		}
	}

	hash_map_iterator_pt importedSvcIter = hashMapIterator_create(manager->importedServices);

	while (hashMapIterator_hasNext(importedSvcIter)) {
		hash_map_entry_pt entry = hashMapIterator_nextEntry(importedSvcIter);
		hash_map_pt imports = hashMapEntry_getValue(entry);

		import_registration_t *import = (import_registration_t *)hashMap_remove(imports, rsa);

		if (import != NULL) {
			celix_status_t subStatus = rsa->importRegistration_close(rsa->admin, import);
			if (subStatus != CELIX_SUCCESS) {
				celix_logHelper_error(manager->loghelper, "TOPOLOGY_MANAGER: Failed to close imported endpoint.");
			}
		}
	}
	hashMapIterator_destroy(importedSvcIter);

    free(celix_longHashMap_get(manager->rsaMap, rsaSvcId));
    celix_longHashMap_remove(manager->rsaMap, rsaSvcId);

	celixThreadMutex_unlock(&manager->lock);

	celix_logHelper_log(manager->loghelper, CELIX_LOG_LEVEL_INFO, "TOPOLOGY_MANAGER: Removed RSA");

	return status;
}

celix_status_t topologyManager_exportScopeChanged(void *handle, char *filterStr) {
	celix_status_t status = CELIX_SUCCESS;
	topology_manager_pt manager = (topology_manager_pt) handle;
	service_registration_t *reg = NULL;
	bool found;
	celix_properties_t *props;
	celix_autoptr(celix_filter_t) filter = celix_filter_create(filterStr);

	if (filter == NULL) {
        celix_logHelper_error(manager->loghelper,"filter creating failed\n");
		return CELIX_ENOMEM;
	}

	// add already exported services to new rsa
    celix_auto(celix_mutex_lock_guard_t) lockGuard = celixMutexLockGuard_init(&manager->lock);

	int size = celix_longHashMap_size(manager->exportedServices);
	celix_autofree service_reference_pt *srvRefs = (service_reference_pt *) calloc(size, sizeof(service_reference_pt));
    if (srvRefs == NULL) {
        return CELIX_ENOMEM;
    }
	int nrFound = 0;

	found = false;

    CELIX_LONG_HASH_MAP_ITERATE(manager->exportedServices, iter) {
        celix_exported_service_entry_t* svcEntry = iter.value.ptrValue;
		service_reference_pt reference = svcEntry->reference;
		reg = NULL;
		serviceReference_getServiceRegistration(reference, &reg);
		if (reg != NULL) {
			props = NULL;
			serviceRegistration_getProperties(reg, &props);
			status = filter_match(filter, props, &found);
			if (found) {
				srvRefs[nrFound++] = reference;
			}
		}
	}

	if (nrFound > 0) {
		for (int i = 0; i < nrFound; i++) {
			// Question: can srvRefs become invalid meanwhile??
			const char* export = NULL;
			serviceReference_getProperty(srvRefs[i], (char *) CELIX_RSA_SERVICE_EXPORTED_INTERFACES, &export);

			if (export) {
				topologyManager_removeExportedService_nolock(manager, srvRefs[i]);
                celix_status_t substatus = topologyManager_addExportedService_nolock(manager, srvRefs[i]);
				if (substatus != CELIX_SUCCESS) {
					status = substatus;
				}
			}
		}
	}

	return status;
}

celix_status_t topologyManager_importScopeChanged(void *handle, char *service_name) {
	celix_status_t status = CELIX_SUCCESS;
	endpoint_description_t *endpoint;
	topology_manager_pt manager = (topology_manager_pt) handle;
	bool found = false;

	// add already imported services to new rsa
	celixThreadMutex_lock(&manager->lock);

	hash_map_iterator_pt importedServicesIterator = hashMapIterator_create(manager->importedServices);
	while (!found && hashMapIterator_hasNext(importedServicesIterator)) {
		hash_map_entry_pt entry = hashMapIterator_nextEntry(importedServicesIterator);
		endpoint = hashMapEntry_getKey(entry);

        const char* name = celix_properties_get(endpoint->properties, CELIX_FRAMEWORK_SERVICE_NAME, "");
		// Test if a service with the same name is imported
		if (strcmp(name, service_name) == 0) {
			found = true;
		}
	}
	hashMapIterator_destroy(importedServicesIterator);

	if (found) {
		status = topologyManager_removeImportedService_nolock(manager, endpoint, NULL);

		if (status != CELIX_SUCCESS) {
			celix_logHelper_log(manager->loghelper, CELIX_LOG_LEVEL_ERROR, "TOPOLOGY_MANAGER: Removal of imported service (%s; %s) failed.", endpoint->serviceName, endpoint->id);
		} else {
			status = topologyManager_addImportedService_nolock(manager, endpoint, NULL);
		}
	}

	//should unlock until here ?, avoid endpoint is released during topologyManager_removeImportedService
	celixThreadMutex_unlock(&manager->lock);

	return status;
}

static celix_status_t topologyManager_addImportedService_nolock(void *handle, endpoint_description_t *endpoint, char *matchedFilter) {
	celix_status_t status = CELIX_SUCCESS;
	topology_manager_pt manager = handle;

	celix_logHelper_log(manager->loghelper, CELIX_LOG_LEVEL_DEBUG, "TOPOLOGY_MANAGER: Add imported service (%s; %s).", endpoint->serviceName, endpoint->id);

	// We should not try to add imported services to a closed listener.
	if (manager->closed) {
		celix_logHelper_log(manager->loghelper, CELIX_LOG_LEVEL_TRACE,"TOPOLOGY_MANAGER: Endpointer listener will close, Ignore imported service (%s; %s).", endpoint->serviceName, endpoint->id);
		return CELIX_SUCCESS;
	}

	hash_map_pt imports = hashMap_create(NULL, NULL, NULL, NULL);
	hashMap_put(manager->importedServices, endpoint, imports);

	if (scope_allowImport(manager->scope, endpoint)) {
        CELIX_LONG_HASH_MAP_ITERATE(manager->rsaMap, iter) {
            celix_rsa_service_entry_t* rsaSvcEntry = iter.value.ptrValue;
			import_registration_t *import = NULL;
			remote_service_admin_service_t *rsa = rsaSvcEntry->rsa;
			celix_status_t substatus = rsa->importService(rsa->admin, endpoint, &import);
			if (substatus == CELIX_SUCCESS) {
				hashMap_put(imports, rsa, import);
			} else {
				status = substatus;
			}
		}
	}

	return status;
}

celix_status_t topologyManager_addImportedService(void *handle, endpoint_description_t *endpoint, char *matchedFilter) {
	celix_status_t status = CELIX_SUCCESS;
	topology_manager_pt manager = handle;

	celix_logHelper_log(manager->loghelper, CELIX_LOG_LEVEL_INFO, "TOPOLOGY_MANAGER: Add imported service");

	celixThreadMutex_lock(&manager->lock);

	status = topologyManager_addImportedService_nolock(handle, endpoint, matchedFilter);

	celixThreadMutex_unlock(&manager->lock);

	return status;
}

static celix_status_t topologyManager_removeImportedService_nolock(void *handle, endpoint_description_t *endpoint, char *matchedFilter) {
	celix_status_t status = CELIX_SUCCESS;
	topology_manager_pt manager = handle;

	celix_logHelper_log(manager->loghelper, CELIX_LOG_LEVEL_DEBUG, "TOPOLOGY_MANAGER: Remove imported service (%s; %s).", endpoint->serviceName, endpoint->id);

	hash_map_iterator_pt iter = hashMapIterator_create(manager->importedServices);
	while (hashMapIterator_hasNext(iter)) {
		hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
		endpoint_description_t *ep = hashMapEntry_getKey(entry);
		hash_map_pt imports = hashMapEntry_getValue(entry);

		if (imports != NULL && strcmp(endpoint->id, ep->id) == 0) {
			hash_map_iterator_pt importsIter = hashMapIterator_create(imports);

			while (hashMapIterator_hasNext(importsIter)) {
				hash_map_entry_pt entry = hashMapIterator_nextEntry(importsIter);
				remote_service_admin_service_t *rsa = hashMapEntry_getKey(entry);
				import_registration_t *import = hashMapEntry_getValue(entry);
				celix_status_t substatus = rsa->importRegistration_close(rsa->admin, import);
				if (substatus == CELIX_SUCCESS) {
					hashMapIterator_remove(importsIter);
				} else {
					status = substatus;
				}
			}
			hashMapIterator_destroy(importsIter);
			hashMapIterator_remove(iter);

			hashMap_destroy(imports, false, false);
		}
	}
	hashMapIterator_destroy(iter);

	return status;
}

celix_status_t topologyManager_removeImportedService(void *handle, endpoint_description_t *endpoint, char *matchedFilter) {
	celix_status_t status = CELIX_SUCCESS;
	topology_manager_pt manager = handle;

	celix_logHelper_log(manager->loghelper, CELIX_LOG_LEVEL_INFO, "TOPOLOGY_MANAGER: Remove imported service");

	celixThreadMutex_lock(&manager->lock);

	status = topologyManager_removeImportedService_nolock(handle, endpoint, matchedFilter);

	celixThreadMutex_unlock(&manager->lock);

	return status;
}

static celix_exported_service_entry_t* exportedServiceEntry_create(topology_manager_t* tm, service_reference_pt reference) {
    celix_autofree celix_exported_service_entry_t* svcEntry = calloc(1, sizeof(*svcEntry));
    if (svcEntry == NULL) {
        celix_logHelper_error(tm->loghelper, "TOPOLOGY_MANAGER: Error allocating exported service entry.");
        return NULL;
    }
    svcEntry->reference = reference;
    celix_autoptr(celix_long_hash_map_t) registrations = svcEntry->registrations = celix_longHashMap_create();
    if (registrations == NULL) {
        celix_logHelper_logTssErrors(tm->loghelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(tm->loghelper, "TOPOLOGY_MANAGER: Error creating hash map for exported service registrations.");
        return NULL;
    }
    celix_steal_ptr(registrations);
    return celix_steal_ptr(svcEntry);
}

static void exportedServiceEntry_destroy(celix_exported_service_entry_t* entry) {
    celix_longHashMap_destroy(entry->registrations);
    free(entry);
    return;
}

static celix_status_t topologyManager_addExportedService_nolock(void * handle, service_reference_pt reference) {
    topology_manager_pt manager = handle;
	celix_status_t status = CELIX_SUCCESS;
    long serviceId = serviceReference_getServiceId(reference);
    char serviceIdStr[64];
    snprintf(serviceIdStr, 64, "%li", serviceId);
	celix_properties_t *serviceProperties = NULL;

	const char *export = NULL;
    serviceReference_getProperty(reference, CELIX_RSA_SERVICE_EXPORTED_INTERFACES, &export);
    assert(export != NULL);

	celix_logHelper_log(manager->loghelper, CELIX_LOG_LEVEL_DEBUG, "TOPOLOGY_MANAGER: Add exported service (%li).", serviceId);


	scope_getExportProperties(manager->scope, reference, &serviceProperties);

    celix_exported_service_entry_t* svcEntry = exportedServiceEntry_create(manager, reference);
    if (svcEntry == NULL) {
        celix_logHelper_error(manager->loghelper, "TOPOLOGY_MANAGER: Error allocating exported service entry.");
        return CELIX_ENOMEM;
    }
    status = celix_longHashMap_put(manager->exportedServices, serviceId, svcEntry);
    if (status != CELIX_SUCCESS) {
        exportedServiceEntry_destroy(svcEntry);
        celix_logHelper_logTssErrors(manager->loghelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(manager->loghelper, "TOPOLOGY_MANAGER: Error adding exported service entry to map.");
        return status;
    }

    if (celix_longHashMap_size(manager->rsaMap) == 0) {
        celix_logHelper_log(manager->loghelper, CELIX_LOG_LEVEL_WARNING, "TOPOLOGY_MANAGER: No RSA available yet.");
    }

    CELIX_LONG_HASH_MAP_ITERATE(manager->rsaMap, iter) {
        long rsaSvcId = iter.key;
        celix_rsa_service_entry_t* rsaSvcEntry = iter.value.ptrValue;
		remote_service_admin_service_t *rsa = rsaSvcEntry->rsa;

		celix_autoptr(celix_array_list_t) registrationList = NULL;
		celix_status_t substatus = rsa->exportService(rsa->admin, serviceIdStr, serviceProperties, &registrationList);
        if (substatus != CELIX_SUCCESS) {
            status = substatus;
            celix_logHelper_error(manager->loghelper, "TOPOLOGY_MANAGER: Error exporting service.");
            continue;
        }
        substatus = celix_longHashMap_put(svcEntry->registrations, rsaSvcId, registrationList);
        if (substatus != CELIX_SUCCESS) {
            celix_logHelper_logTssErrors(manager->loghelper, CELIX_LOG_LEVEL_ERROR);
            celix_logHelper_error(manager->loghelper, "TOPOLOGY_MANAGER: Error adding exported endpoints to map.");
            for (int i = 0; i < celix_arrayList_size(registrationList); ++i) {
                export_registration_t* reg = celix_arrayList_get(registrationList, i);
                rsa->exportRegistration_close(rsa->admin, reg);
            }
            status = substatus;
            continue;
        }
        if (rsaSvcEntry->dynamicIpSupport) {
            topologyManager_addDynamicIpEndpointsForExportedService(manager, rsaSvcId, serviceId, registrationList);
        } else {
            topologyManager_notifyListenersEndpointAdded(manager, rsa, registrationList);
        }
        celix_steal_ptr(registrationList);
	}

	return status;
}

celix_status_t topologyManager_addExportedService(void * handle, service_reference_pt reference, void * service CELIX_UNUSED) {
	topology_manager_pt manager = handle;
	celix_status_t status = CELIX_SUCCESS;

	celix_logHelper_log(manager->loghelper, CELIX_LOG_LEVEL_INFO, "TOPOLOGY_MANAGER: Add exported service");

	celixThreadMutex_lock(&manager->lock);

	status = topologyManager_addExportedService_nolock(handle, reference);

	celixThreadMutex_unlock(&manager->lock);

	return status;
}

static void topologyManager_removeExportedService_nolock(void * handle, service_reference_pt reference) {
    topology_manager_pt manager = handle;
	long serviceId = serviceReference_getServiceId(reference);

	celix_logHelper_log(manager->loghelper, CELIX_LOG_LEVEL_DEBUG, "TOPOLOGY_MANAGER: Remove exported service (%li).", serviceId);

    celix_exported_service_entry_t* svcEntry = celix_longHashMap_get(manager->exportedServices, serviceId);
    if (svcEntry == NULL) {
        celix_logHelper_debug(manager->loghelper, "TOPOLOGY_MANAGER: No exported service entry found for service id %li", serviceId);
        return;
    }

    CELIX_LONG_HASH_MAP_ITERATE(svcEntry->registrations, iter) {
        long rsaSvcId = iter.key;
        celix_rsa_service_entry_t* rsaSvcEntry = celix_longHashMap_get(manager->rsaMap, rsaSvcId);
        assert(rsaSvcEntry != NULL);//It must be not null, because the rsa will be removed from manager->exportedServices when rsa is removed
        remote_service_admin_service_t* rsa = rsaSvcEntry->rsa;

        topologyManager_removeDynamicIpEndpointsForExportedService(manager, rsaSvcId, serviceId);

        celix_array_list_t *exportRegistrations = iter.value.ptrValue;
        if (exportRegistrations != NULL) {
            int size = celix_arrayList_size(exportRegistrations);
            for (int exportsIter = 0; exportsIter < size; exportsIter++) {
                export_registration_t *export = celix_arrayList_get(exportRegistrations, exportsIter);
                topologyManager_notifyListenersEndpointRemoved(manager, rsa, export);
                rsa->exportRegistration_close(rsa->admin, export);
            }
            celix_arrayList_destroy(exportRegistrations);
        }
    }

    celix_longHashMap_remove(manager->exportedServices, serviceId);
    exportedServiceEntry_destroy(svcEntry);

	return;
}

celix_status_t topologyManager_removeExportedService(void * handle, service_reference_pt reference, void * service CELIX_UNUSED) {
	topology_manager_pt manager = handle;
	celix_status_t status = CELIX_SUCCESS;

	celix_logHelper_log(manager->loghelper, CELIX_LOG_LEVEL_INFO, "TOPOLOGY_MANAGER: Remove exported service");

	celixThreadMutex_lock(&manager->lock);

	topologyManager_removeExportedService_nolock(handle, reference);

	celixThreadMutex_unlock(&manager->lock);

	return status;
}

celix_status_t topologyManager_getEndpointDescriptionForExportRegistration(remote_service_admin_service_t *rsa, export_registration_t *export, endpoint_description_t **endpoint) {
	celix_status_t status;

	export_reference_t *reference = NULL;
	status = rsa->exportRegistration_getExportReference(export, &reference);

	if (status == CELIX_SUCCESS) {
		status = rsa->exportReference_getExportedEndpoint(reference, endpoint);
	}

	free(reference);

	return status;
}

celix_status_t topologyManager_endpointListenerAdding(void* handle, service_reference_pt reference, void** service) {
	celix_status_t status = CELIX_SUCCESS;
	topology_manager_pt manager = (topology_manager_pt) handle;

	bundleContext_getService(manager->context, reference, service);

	return status;
}

static void topologyManager_notifyDynamicIpEndpointsToListener(topology_manager_t* tm, endpoint_listener_t* listener,
                                                               char* scope, celix_filter_t* filter) {
    CELIX_LONG_HASH_MAP_ITERATE(tm->dynamicIpEndpoints, iter) {
        celix_long_hash_map_t* endpointMap = iter.value.ptrValue;
        CELIX_LONG_HASH_MAP_ITERATE(endpointMap, iter2) {
            celix_array_list_t* endpointList = (celix_array_list_t*)iter2.value.ptrValue;
            int size = celix_arrayList_size(endpointList);
            for (int i = 0; i < size; ++i) {
                endpoint_description_t* endpoint = celix_arrayList_get(endpointList, i);
                if (celix_filter_match(filter, endpoint->properties)) {
                    celix_status_t status = listener->endpointAdded(listener->handle, endpoint, scope);
                    if (status != CELIX_SUCCESS) {
                        celix_logHelper_error(tm->loghelper, "TOPOLOGY_MANAGER: Error adding endpoint to listener. %d", status);
                    }
                }
            }
        }
    }
    return;
}

celix_status_t topologyManager_endpointListenerAdded(void* handle, service_reference_pt reference, void* service) {
	celix_status_t status = CELIX_SUCCESS;
	topology_manager_pt manager = handle;
	const char* scope = NULL;

	const char *topologyManagerEPL = NULL;
	serviceReference_getProperty(reference, "TOPOLOGY_MANAGER", &topologyManagerEPL);
	if (topologyManagerEPL != NULL && strcmp(topologyManagerEPL, "true") == 0) {
		celix_logHelper_log(manager->loghelper, CELIX_LOG_LEVEL_DEBUG, "TOPOLOGY_MANAGER: Ignored itself ENDPOINT_LISTENER");
		return CELIX_SUCCESS;
	}

	celix_logHelper_log(manager->loghelper, CELIX_LOG_LEVEL_INFO, "TOPOLOGY_MANAGER: Added ENDPOINT_LISTENER");

	celixThreadMutex_lock(&manager->lock);

	hashMap_put(manager->listenerList, reference, NULL);

	serviceReference_getProperty(reference, CELIX_RSA_ENDPOINT_LISTENER_SCOPE, &scope);

	celix_autoptr(celix_filter_t) filter = celix_filter_create(scope);


    CELIX_LONG_HASH_MAP_ITERATE(manager->exportedServices, iter) {
        celix_exported_service_entry_t* svcEntry = iter.value.ptrValue;
        CELIX_LONG_HASH_MAP_ITERATE(svcEntry->registrations, iter2) {
			long rsaSvcId = iter2.key;
            celix_rsa_service_entry_t* rsaSvcEntry = celix_longHashMap_get(manager->rsaMap, rsaSvcId);
			celix_array_list_t *registrations = iter2.value.ptrValue;

            if (rsaSvcEntry->dynamicIpSupport) {
                continue;//Dynamic ip endpoints are added later
            }

			int arrayListSize = celix_arrayList_size(registrations);
			for (int cnt = 0; cnt < arrayListSize; cnt++) {
				export_registration_t *export = celix_arrayList_get(registrations, cnt);
				endpoint_description_t *endpoint = NULL;

				status = topologyManager_getEndpointDescriptionForExportRegistration(rsaSvcEntry->rsa, export, &endpoint);
				if (status == CELIX_SUCCESS) {
					bool matchResult = false;
					filter_match(filter, endpoint->properties, &matchResult);
					if (matchResult) {
						endpoint_listener_t *listener = (endpoint_listener_t *) service;
						status = listener->endpointAdded(listener->handle, endpoint, (char*)scope);
					}
				}
			}
		}
	}

    const char *interfaceSpecEndpointSupport = NULL;
    serviceReference_getProperty(reference, CELIX_RSA_DISCOVERY_INTERFACE_SPECIFIC_ENDPOINTS_SUPPORT, &interfaceSpecEndpointSupport);
    if (interfaceSpecEndpointSupport != NULL && strcmp(interfaceSpecEndpointSupport, "true") == 0) {
        topologyManager_notifyDynamicIpEndpointsToListener(manager, (endpoint_listener_t *) service, (char *) scope,
                                                           filter);
    }

	celixThreadMutex_unlock(&manager->lock);

	return status;
}

celix_status_t topologyManager_endpointListenerModified(void * handle, service_reference_pt reference, void * service) {
	celix_status_t status;

	status = topologyManager_endpointListenerRemoved(handle, reference, service);

	if (status == CELIX_SUCCESS) {
		status = topologyManager_endpointListenerAdded(handle, reference, service);
	}

	return status;
}

celix_status_t topologyManager_endpointListenerRemoved(void * handle, service_reference_pt reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;
	topology_manager_pt manager = handle;
	celixThreadMutex_lock(&manager->lock);

	if (hashMap_remove(manager->listenerList, reference)) {
		celix_logHelper_log(manager->loghelper, CELIX_LOG_LEVEL_INFO, "EndpointListener Removed");
	}

	celixThreadMutex_unlock(&manager->lock);


	return status;
}

static celix_status_t topologyManager_notifyListenersEndpointAdded(topology_manager_pt manager, remote_service_admin_service_t *rsa, celix_array_list_t *registrations) {
	celix_status_t status = CELIX_SUCCESS;

	hash_map_iterator_pt iter = hashMapIterator_create(manager->listenerList);
	while (hashMapIterator_hasNext(iter)) {
		const char* scope = NULL;
		endpoint_listener_t *epl = NULL;
		service_reference_pt reference = hashMapIterator_nextKey(iter);

		serviceReference_getProperty(reference, CELIX_RSA_ENDPOINT_LISTENER_SCOPE, &scope);

		status = bundleContext_getService(manager->context, reference, (void **) &epl);
		if (status == CELIX_SUCCESS) {
			celix_autoptr(celix_filter_t) filter = celix_filter_create(scope);

			int regSize = celix_arrayList_size(registrations);
			for (int regIt = 0; regIt < regSize; regIt++) {
				export_registration_t *export = celix_arrayList_get(registrations, regIt);
				endpoint_description_t *endpoint = NULL;
				celix_status_t substatus = topologyManager_getEndpointDescriptionForExportRegistration(rsa, export, &endpoint);
				if (substatus == CELIX_SUCCESS) {
					bool matchResult = false;
					filter_match(filter, endpoint->properties, &matchResult);
					if (matchResult) {
						status = epl->endpointAdded(epl->handle, endpoint, (char*)scope);
					}
				} else {
					status = substatus;
				}
			}
			bundleContext_ungetService(manager->context, reference, NULL);
		}
	}
	hashMapIterator_destroy(iter);

	return status;
}

static celix_status_t topologyManager_notifyListenersEndpointRemoved(topology_manager_pt manager, remote_service_admin_service_t *rsa, export_registration_t *export) {
    celix_status_t status = CELIX_SUCCESS;

	hash_map_iterator_pt iter = hashMapIterator_create(manager->listenerList);
	while (hashMapIterator_hasNext(iter)) {
		endpoint_description_t *endpoint = NULL;
		endpoint_listener_t *epl = NULL;
		celix_status_t substatus;
		const char* scope = NULL;

		service_reference_pt reference = hashMapIterator_nextKey(iter);
		serviceReference_getProperty(reference, CELIX_RSA_ENDPOINT_LISTENER_SCOPE, &scope);

		substatus = bundleContext_getService(manager->context, reference, (void **) &epl);

		if (substatus == CELIX_SUCCESS) {
			substatus = topologyManager_getEndpointDescriptionForExportRegistration(rsa, export, &endpoint);
		}

		if (substatus == CELIX_SUCCESS) {
			substatus = epl->endpointRemoved(epl->handle, endpoint, NULL);
		}
		bundleContext_ungetService(manager->context, reference, NULL);
	}
	hashMapIterator_destroy(iter);

    return status;
}

static celix_status_t topologyManager_extendFilter(topology_manager_pt manager,  const char *filter, char **updatedFilter) {
	celix_status_t status;
	celix_bundle_context_t *context = manager->context;
	const char* uuid = NULL;

	status = bundleContext_getProperty(context, CELIX_FRAMEWORK_UUID, &uuid);

	if (!uuid) {
		celix_logHelper_log(manager->loghelper, CELIX_LOG_LEVEL_ERROR, "TOPOLOGY_MANAGER: no framework UUID defined?!");
		return CELIX_BUNDLE_EXCEPTION;
	}

	int len = 10 + strlen(filter) + strlen(CELIX_RSA_ENDPOINT_FRAMEWORK_UUID) + strlen(uuid);
	*updatedFilter = malloc(len);
	if (!*updatedFilter) {
		return CELIX_ENOMEM;
	}

	snprintf(*updatedFilter, len, "(&%s(!(%s=%s)))", filter, CELIX_RSA_ENDPOINT_FRAMEWORK_UUID, uuid);

	return status;
}

celix_status_t topologyManager_listenerAdded(void *handle, celix_array_list_t* listeners) {
	celix_status_t status = CELIX_SUCCESS;
	topology_manager_pt manager = handle;

	for (int i = 0; i < celix_arrayList_size(listeners); i++) {
		listener_hook_info_pt info = celix_arrayList_get(listeners, i);
		celix_bundle_t *bundle = NULL, *self = NULL;
		bundleContext_getBundle(info->context, &bundle);
		bundleContext_getBundle(manager->context, &self);
		if (bundle == self) {
			celix_logHelper_log(manager->loghelper, CELIX_LOG_LEVEL_DEBUG, "TOPOLOGY_MANAGER: Ignore myself.");
			continue;
		}

		celix_logHelper_log(manager->loghelper, CELIX_LOG_LEVEL_INFO, "TOPOLOGY_MANAGER: listener with filter \"%s\" added", info->filter);

		char *filter = NULL;
		bool free_filter = true;
		status = topologyManager_extendFilter(manager, info->filter, &filter);
#if 0
		if(filter != NULL){
			// TODO: add status handling
			status = celixThreadMutex_lock(&manager->importScopesLock);

			struct scope *interest = hashMap_get(manager->importScopes, filter);
			if (interest) {
				interest->refs++;
				free(filter);
				filter = NULL;
			} else {
				interest = malloc(sizeof(*interest));
				interest->filter = filter;
				interest->refs = 1;
				hashMap_put(manager->importScopes, filter, interest);
				free_filter = false;
			}

			status = celixThreadMutex_unlock(&manager->importScopesLock);
		}
#endif

		if (filter != NULL && free_filter) {
			free(filter);
		}

	}

	return status;
}

celix_status_t topologyManager_listenerRemoved(void *handle, celix_array_list_t* listeners) {
	celix_status_t status = CELIX_SUCCESS;
	topology_manager_pt manager = handle;

	for (int i = 0; i < celix_arrayList_size(listeners); i++) {
		listener_hook_info_pt info = celix_arrayList_get(listeners, i);

		celix_bundle_t *bundle = NULL, *self = NULL;
		bundleContext_getBundle(info->context, &bundle);
		bundleContext_getBundle(manager->context, &self);
		if (bundle == self) {
			celix_logHelper_log(manager->loghelper, CELIX_LOG_LEVEL_DEBUG, "TOPOLOGY_MANAGER: Ignore myself.");
			continue;
		}

		celix_logHelper_log(manager->loghelper, CELIX_LOG_LEVEL_INFO, "TOPOLOGY_MANAGER: listener with filter \"%s\" removed.", info->filter);

		char *filter = NULL;
		topologyManager_extendFilter(manager, info->filter, &filter);
#if 0
		status = celixThreadMutex_lock(&manager->importScopesLock);

		struct scope *interest = hashMap_get(manager->importScopes, filter);
		if (interest != NULL && --interest->refs <= 0) {
			// last reference, remove from scope
			hash_map_entry_pt entry = hashMap_getEntry(manager->importScopes, filter);
			char* key = (char*) hashMapEntry_getKey(entry);
			interest = hashMap_remove(manager->importScopes, filter);
			free(key);
			free(interest);
		}
#endif

		if (filter != NULL) {
			free(filter);
		}
#if 0
		status = celixThreadMutex_unlock(&manager->importScopesLock);
#endif
	}

	return status;
}

