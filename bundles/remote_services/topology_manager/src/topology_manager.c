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

#include "topology_manager.h"
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

struct topology_manager {
	celix_bundle_context_t *context;

	celix_array_list_t *rsaList;

	hash_map_pt listenerList;

	hash_map_pt exportedServices;

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

static celix_status_t topologyManager_addImportedService_nolock(void *handle, endpoint_description_t *endpoint, char *matchedFilter);
static celix_status_t topologyManager_removeImportedService_nolock(void *handle, endpoint_description_t *endpoint, char *matchedFilter);
static celix_status_t topologyManager_addExportedService_nolock(void * handle, service_reference_pt reference, void * service);
static celix_status_t topologyManager_removeExportedService_nolock(void * handle, service_reference_pt reference, void * service);

celix_status_t topologyManager_create(celix_bundle_context_t *context, celix_log_helper_t *logHelper, topology_manager_pt *manager, void **scope) {
	celix_status_t status = CELIX_SUCCESS;

	*manager = calloc(1, sizeof(**manager));

	if (!*manager) {
		return CELIX_ENOMEM;
	}

	(*manager)->context = context;
	(*manager)->rsaList = NULL;

	celixThreadMutex_create(&(*manager)->lock, NULL);

	(*manager)->rsaList = celix_arrayList_create();
	(*manager)->listenerList = hashMap_create(serviceReference_hashCode, NULL, serviceReference_equals2, NULL);
	(*manager)->exportedServices = hashMap_create(serviceReference_hashCode, NULL, serviceReference_equals2, NULL);
	(*manager)->importedServices = hashMap_create(NULL, NULL, NULL, NULL);

	(*manager)->closed = false;

	status = scope_scopeCreate(*manager, &(*manager)->scope);
	assert(status == CELIX_SUCCESS);
	scope_setExportScopeChangedCallback((*manager)->scope, topologyManager_exportScopeChanged);
	scope_setImportScopeChangedCallback((*manager)->scope, topologyManager_importScopeChanged);
	*scope = (*manager)->scope;

	(*manager)->loghelper = logHelper;


	return status;
}

celix_status_t topologyManager_destroy(topology_manager_pt manager) {
	celix_status_t status = CELIX_SUCCESS;

	scope_scopeDestroy(manager->scope);

	celixThreadMutex_lock(&manager->lock);

	hashMap_destroy(manager->importedServices, false, false);
	hashMap_destroy(manager->exportedServices, false, false);
	hashMap_destroy(manager->listenerList, false, false);
	celix_arrayList_destroy(manager->rsaList);

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

celix_status_t topologyManager_rsaAdded(void * handle, service_reference_pt unusedRef CELIX_UNUSED, void * service) {
	topology_manager_pt manager = (topology_manager_pt) handle;
	celix_properties_t *serviceProperties = NULL;
	remote_service_admin_service_t *rsa = (remote_service_admin_service_t *) service;
	celix_logHelper_log(manager->loghelper, CELIX_LOG_LEVEL_INFO, "TOPOLOGY_MANAGER: Added RSA");


    celixThreadMutex_lock(&manager->lock);

    celix_arrayList_add(manager->rsaList, rsa);

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
    hash_map_iterator_pt exportedServicesIterator = hashMapIterator_create(manager->exportedServices);

    while (hashMapIterator_hasNext(exportedServicesIterator)) {
        hash_map_entry_pt entry = hashMapIterator_nextEntry(exportedServicesIterator);
        service_reference_pt reference = hashMapEntry_getKey(entry);
        const char* serviceId = NULL;

        serviceReference_getProperty(reference, CELIX_FRAMEWORK_SERVICE_ID, &serviceId);

        scope_getExportProperties(manager->scope, reference, &serviceProperties);

        celix_array_list_t *endpoints = NULL;
        celix_status_t status = rsa->exportService(rsa->admin, (char*)serviceId, serviceProperties, &endpoints);

        if (status == CELIX_SUCCESS) {
            hash_map_pt exports = hashMapEntry_getValue(entry);
            assert(exports != NULL);// It must not be NULL, It has been created in function topologyManager_addExportedService_nolock
            hashMap_put(exports, rsa, endpoints);
            topologyManager_notifyListenersEndpointAdded(manager, rsa, endpoints);
        }
    }

    hashMapIterator_destroy(exportedServicesIterator);
    celixThreadMutex_unlock(&manager->lock);

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

	celixThreadMutex_lock(&manager->lock);

	hash_map_iterator_pt exportedSvcIter = hashMapIterator_create(manager->exportedServices);

	while (hashMapIterator_hasNext(exportedSvcIter)) {

		hash_map_entry_pt entry = hashMapIterator_nextEntry(exportedSvcIter);
		hash_map_pt exports = hashMapEntry_getValue(entry);

		celix_array_list_t *exports_list = (celix_array_list_t *)hashMap_remove(exports, rsa);
		if (exports_list != NULL) {
			int exportsIter = 0;
			int exportListSize = celix_arrayList_size(exports_list);
			for (exportsIter = 0; exports_list != NULL && exportsIter < exportListSize; exportsIter++) {
				export_registration_t *export = celix_arrayList_get(exports_list, exportsIter);
				topologyManager_notifyListenersEndpointRemoved(manager, rsa, export);
				rsa->exportRegistration_close(rsa->admin, export);
			}
			celix_arrayList_destroy(exports_list);
		}
	}
	hashMapIterator_destroy(exportedSvcIter);

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

	celix_arrayList_remove(manager->rsaList, rsa);

	celixThreadMutex_unlock(&manager->lock);

	celix_logHelper_log(manager->loghelper, CELIX_LOG_LEVEL_INFO, "TOPOLOGY_MANAGER: Removed RSA");

	return status;
}

celix_status_t topologyManager_exportScopeChanged(void *handle, char *filterStr) {
	celix_status_t status = CELIX_SUCCESS;
	topology_manager_pt manager = (topology_manager_pt) handle;
	service_registration_t *reg = NULL;
	const char* serviceId = NULL;
	bool found;
	celix_properties_t *props;
	celix_autoptr(celix_filter_t) filter = celix_filter_create(filterStr);

	if (filter == NULL) {
		printf("filter creating failed\n");
		return CELIX_ENOMEM;
	}

	// add already exported services to new rsa
	celixThreadMutex_lock(&manager->lock);

	hash_map_iterator_pt exportedServicesIterator = hashMapIterator_create(manager->exportedServices);
	int size = hashMap_size(manager->exportedServices);
	service_reference_pt *srvRefs = (service_reference_pt *) calloc(size, sizeof(service_reference_pt));
	char **srvIds = (char **) calloc(size, sizeof(char*));
	int nrFound = 0;

	found = false;

	while (hashMapIterator_hasNext(exportedServicesIterator)) {
		hash_map_entry_pt entry = hashMapIterator_nextEntry(exportedServicesIterator);
		service_reference_pt reference = hashMapEntry_getKey(entry);
		reg = NULL;
		serviceReference_getServiceRegistration(reference, &reg);
		if (reg != NULL) {
			props = NULL;
			serviceRegistration_getProperties(reg, &props);
			status = filter_match(filter, props, &found);
			if (found) {
				srvRefs[nrFound] = reference;
				serviceReference_getProperty(reference, CELIX_FRAMEWORK_SERVICE_ID, &serviceId);
				srvIds[nrFound++] = (char*)serviceId;
			}
		}
	}

	hashMapIterator_destroy(exportedServicesIterator);

	if (nrFound > 0) {
		for (int i = 0; i < nrFound; i++) {
			// Question: can srvRefs become invalid meanwhile??
			const char* export = NULL;
			serviceReference_getProperty(srvRefs[i], (char *) CELIX_RSA_SERVICE_EXPORTED_INTERFACES, &export);

			if (export) {
				celix_status_t substatus = topologyManager_removeExportedService_nolock(manager, srvRefs[i], srvIds[i]);

				if (substatus != CELIX_SUCCESS) {
					celix_logHelper_log(manager->loghelper, CELIX_LOG_LEVEL_ERROR, "TOPOLOGY_MANAGER: Removal of exported service (%s) failed.", srvIds[i]);
				} else {
					substatus = topologyManager_addExportedService_nolock(manager, srvRefs[i], srvIds[i]);
				}

				if (substatus != CELIX_SUCCESS) {
					status = substatus;
				}
			}
		}
	}

	free(srvRefs);
	free(srvIds);

	// should unlock until here ?, avoid srvRefs[i] is released during topologyManager_removeExportedService
	celixThreadMutex_unlock(&manager->lock);

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
		int size = celix_arrayList_size(manager->rsaList);

		for (int iter = 0; iter < size; iter++) {
			import_registration_t *import = NULL;
			remote_service_admin_service_t *rsa = celix_arrayList_get(manager->rsaList, iter);
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

static celix_status_t topologyManager_addExportedService_nolock(void * handle, service_reference_pt reference, void * service CELIX_UNUSED) {
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
	hash_map_pt exports = hashMap_create(NULL, NULL, NULL, NULL);
	assert(exports != NULL);
	hashMap_put(manager->exportedServices, reference, exports);

	int size = celix_arrayList_size(manager->rsaList);

	if (size == 0) {
		celix_logHelper_log(manager->loghelper, CELIX_LOG_LEVEL_WARNING, "TOPOLOGY_MANAGER: No RSA available yet.");
	}

	for (int iter = 0; iter < size; iter++) {
		remote_service_admin_service_t *rsa = celix_arrayList_get(manager->rsaList, iter);

		celix_array_list_t *endpoints = NULL;
		celix_status_t substatus = rsa->exportService(rsa->admin, serviceIdStr, serviceProperties, &endpoints);

		if (substatus == CELIX_SUCCESS) {
			hashMap_put(exports, rsa, endpoints);
			topologyManager_notifyListenersEndpointAdded(manager, rsa, endpoints);
		} else {
			status = substatus;
		}
	}

	return status;
}

celix_status_t topologyManager_addExportedService(void * handle, service_reference_pt reference, void * service) {
	topology_manager_pt manager = handle;
	celix_status_t status = CELIX_SUCCESS;

	celix_logHelper_log(manager->loghelper, CELIX_LOG_LEVEL_INFO, "TOPOLOGY_MANAGER: Add exported service");

	celixThreadMutex_lock(&manager->lock);

	status = topologyManager_addExportedService_nolock(handle, reference, service);

	celixThreadMutex_unlock(&manager->lock);

	return status;
}

static celix_status_t topologyManager_removeExportedService_nolock(void * handle, service_reference_pt reference, void * service CELIX_UNUSED) {
    topology_manager_pt manager = handle;
	celix_status_t status = CELIX_SUCCESS;
	long serviceId = serviceReference_getServiceId(reference);

	celix_logHelper_log(manager->loghelper, CELIX_LOG_LEVEL_DEBUG, "TOPOLOGY_MANAGER: Remove exported service (%li).", serviceId);

	hash_map_pt exports = hashMap_get(manager->exportedServices, reference);
	if (exports) {
		hash_map_iterator_pt iter = hashMapIterator_create(exports);
		while (hashMapIterator_hasNext(iter)) {
			hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
			remote_service_admin_service_t *rsa = hashMapEntry_getKey(entry);
			celix_array_list_t *exportRegistrations = hashMap_remove(exports, rsa);
			if (exportRegistrations != NULL) {
				int size = celix_arrayList_size(exportRegistrations);
				for (int exportsIter = 0; exportsIter < size; exportsIter++) {
					export_registration_t *export = celix_arrayList_get(exportRegistrations, exportsIter);
					topologyManager_notifyListenersEndpointRemoved(manager, rsa, export);
					rsa->exportRegistration_close(rsa->admin, export);
				}
				celix_arrayList_destroy(exportRegistrations);
			}

			hashMapIterator_destroy(iter);
			iter = hashMapIterator_create(exports);

		}
		hashMapIterator_destroy(iter);
	}
	exports = hashMap_remove(manager->exportedServices, reference);

	if (exports != NULL) {
		hashMap_destroy(exports, false, false);
	}

	return status;
}

celix_status_t topologyManager_removeExportedService(void * handle, service_reference_pt reference, void * service) {
	topology_manager_pt manager = handle;
	celix_status_t status = CELIX_SUCCESS;

	celix_logHelper_log(manager->loghelper, CELIX_LOG_LEVEL_INFO, "TOPOLOGY_MANAGER: Remove exported service");

	celixThreadMutex_lock(&manager->lock);

	status = topologyManager_removeExportedService_nolock(handle, reference, service);

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

	hash_map_iterator_pt refIter = hashMapIterator_create(manager->exportedServices);

	while (hashMapIterator_hasNext(refIter)) {
		hash_map_pt rsaExports = hashMapIterator_nextValue(refIter);
		hash_map_iterator_pt rsaIter = hashMapIterator_create(rsaExports);

		while (hashMapIterator_hasNext(rsaIter)) {
			hash_map_entry_pt entry = hashMapIterator_nextEntry(rsaIter);
			remote_service_admin_service_t *rsa = hashMapEntry_getKey(entry);
			celix_array_list_t *registrations = hashMapEntry_getValue(entry);

			int arrayListSize = celix_arrayList_size(registrations);
			int cnt = 0;

			for (; cnt < arrayListSize; cnt++) {
				export_registration_t *export = celix_arrayList_get(registrations, cnt);
				endpoint_description_t *endpoint = NULL;

				status = topologyManager_getEndpointDescriptionForExportRegistration(rsa, export, &endpoint);
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
		hashMapIterator_destroy(rsaIter);
	}
	hashMapIterator_destroy(refIter);

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

celix_status_t topologyManager_listenerAdded(void *handle, array_list_pt listeners) {
	celix_status_t status = CELIX_SUCCESS;
	topology_manager_pt manager = handle;

	for (int i = 0; i < arrayList_size(listeners); i++) {
		listener_hook_info_pt info = arrayList_get(listeners, i);
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

celix_status_t topologyManager_listenerRemoved(void *handle, array_list_pt listeners) {
	celix_status_t status = CELIX_SUCCESS;
	topology_manager_pt manager = handle;

	for (int i = 0; i < arrayList_size(listeners); i++) {
		listener_hook_info_pt info = arrayList_get(listeners, i);

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

