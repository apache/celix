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
 * topology_manager.c
 *
 *  \date       Sep 29, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdio.h>
#include <stdlib.h>

#include "topology_manager.h"
#include "bundle_context.h"
#include "constants.h"
#include "module.h"
#include "bundle.h"
#include "remote_service_admin.h"
#include "remote_constants.h"
#include "filter.h"
#include "listener_hook_service.h"
#include "utils.h"
#include "service_reference.h"
#include "service_registration.h"

struct topology_manager {
	bundle_context_pt context;

	celix_thread_mutex_t rsaListLock;
	array_list_pt rsaList;

	celix_thread_mutex_t exportedServicesLock;
	hash_map_pt exportedServices;

	celix_thread_mutex_t importedServicesLock;
	hash_map_pt importedServices;

	celix_thread_mutex_t importInterestsLock;
	hash_map_pt importInterests;
};

struct import_interest {
	char *filter;
	int refs;
};

celix_status_t topologyManager_notifyListenersEndpointAdded(topology_manager_pt manager, remote_service_admin_service_pt rsa,  array_list_pt registrations);
celix_status_t topologyManager_notifyListenersEndpointRemoved(topology_manager_pt manager, remote_service_admin_service_pt rsa,  export_registration_pt export);

celix_status_t topologyManager_create(bundle_context_pt context, topology_manager_pt *manager) {
	celix_status_t status = CELIX_SUCCESS;

	*manager = malloc(sizeof(**manager));
	if (!*manager) {
		return CELIX_ENOMEM;
	}

	(*manager)->context = context;
	(*manager)->rsaList = NULL;
	arrayList_create(&(*manager)->rsaList);

	status = celixThreadMutex_create(&(*manager)->rsaListLock, NULL);
	status = celixThreadMutex_create(&(*manager)->exportedServicesLock, NULL);
	status = celixThreadMutex_create(&(*manager)->importedServicesLock, NULL);
	status = celixThreadMutex_create(&(*manager)->importInterestsLock, NULL);

	(*manager)->exportedServices = hashMap_create(serviceReference_hashCode, NULL, serviceReference_equals2, NULL);
	(*manager)->importedServices = hashMap_create(NULL, NULL, NULL, NULL);
	(*manager)->importInterests = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);

	return status;
}

celix_status_t topologyManager_destroy(topology_manager_pt manager) {
	celix_status_t status = CELIX_SUCCESS;

	status = celixThreadMutex_lock(&manager->rsaListLock);

	arrayList_destroy(manager->rsaList);

	status = celixThreadMutex_unlock(&manager->rsaListLock);
	status = celixThreadMutex_destroy(&manager->rsaListLock);

	status = celixThreadMutex_lock(&manager->exportedServicesLock);

	hashMap_destroy(manager->exportedServices, false, false);

	status = celixThreadMutex_unlock(&manager->exportedServicesLock);
	status = celixThreadMutex_destroy(&manager->exportedServicesLock);

	status = celixThreadMutex_lock(&manager->importedServicesLock);

	hashMap_destroy(manager->importedServices, false, false);

	status = celixThreadMutex_unlock(&manager->importedServicesLock);
	status = celixThreadMutex_destroy(&manager->importedServicesLock);

	status = celixThreadMutex_lock(&manager->importInterestsLock);

	hashMap_destroy(manager->importInterests, false, false);

	status = celixThreadMutex_unlock(&manager->importInterestsLock);
	status = celixThreadMutex_destroy(&manager->importInterestsLock);

	free(manager);

	return status;
}


celix_status_t topologyManager_rsaAdding(void * handle, service_reference_pt reference, void **service) {
	celix_status_t status = CELIX_SUCCESS;
	topology_manager_pt manager = handle;

	status = bundleContext_getService(manager->context, reference, service);

	return status;
}

celix_status_t topologyManager_rsaAdded(void * handle, service_reference_pt reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;
	topology_manager_pt manager = handle;

	fw_log(logger, OSGI_FRAMEWORK_LOG_INFO, "TOPOLOGY_MANAGER: Added RSA");

	status = celixThreadMutex_lock(&manager->rsaListLock);
	arrayList_add(manager->rsaList, service);
	status = celixThreadMutex_unlock(&manager->rsaListLock);

	// TODO add the imported/exported services to the given RSA...

	return status;
}

celix_status_t topologyManager_rsaModified(void * handle, service_reference_pt reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;

	// Nop...

	return status;
}

celix_status_t topologyManager_rsaRemoved(void * handle, service_reference_pt reference, void * service) {
    celix_status_t status = CELIX_SUCCESS;
    topology_manager_pt manager = handle;

    fw_log(logger, OSGI_FRAMEWORK_LOG_INFO, "TOPOLOGY_MANAGER: Removed RSA");

    remote_service_admin_service_pt rsa = (remote_service_admin_service_pt) service;

    status = celixThreadMutex_lock(&manager->exportedServicesLock);

    hash_map_iterator_pt iter = hashMapIterator_create(manager->exportedServices);

    while (hashMapIterator_hasNext(iter)) {
        int exportsIter = 0;

        hash_map_pt exports = hashMapIterator_nextValue(iter);
        array_list_pt exports_list = hashMap_get(exports, rsa);

        if (exports_list != NULL) {
            for (exportsIter = 0; exportsIter < arrayList_size(exports_list); exportsIter++) {
                export_registration_pt export = arrayList_get(exports_list, exportsIter);
                rsa->exportRegistration_close(export);
                topologyManager_notifyListenersEndpointRemoved(manager, rsa, export);
            }

            arrayList_destroy(exports_list);
            exports_list = NULL;
        }

        hashMap_remove(exports, rsa);
        rsa = NULL;
    }
    status = celixThreadMutex_unlock(&manager->exportedServicesLock);

    fw_log(logger, OSGI_FRAMEWORK_LOG_INFO, "TOPOLOGY_MANAGER: Removed RSA");

    status = celixThreadMutex_lock(&manager->rsaListLock);
    arrayList_removeElement(manager->rsaList, rsa);
    status = celixThreadMutex_unlock(&manager->rsaListLock);

    return status;
}

celix_status_t topologyManager_getRSAs(topology_manager_pt manager, array_list_pt *rsaList) {
	celix_status_t status = CELIX_SUCCESS;

	status = arrayList_create(rsaList);
	if (status != CELIX_SUCCESS) {
		return CELIX_ENOMEM;
	}

	status = celixThreadMutex_lock(&manager->rsaListLock);
	arrayList_addAll(*rsaList, manager->rsaList);
	status = celixThreadMutex_unlock(&manager->rsaListLock);

	return status;
}

celix_status_t topologyManager_serviceChanged(void *listener, service_event_pt event) {
	celix_status_t status = CELIX_SUCCESS;
	service_listener_pt listen = listener;
	topology_manager_pt manager = listen->handle;

	service_registration_pt registration = NULL;
	serviceReference_getServiceRegistration(event->reference, &registration);

	properties_pt props = NULL;
	serviceRegistration_getProperties(registration, &props);
	char *export = properties_get(props, (char *) OSGI_RSA_SERVICE_EXPORTED_INTERFACES);
	char *serviceId = properties_get(props, (char *)OSGI_FRAMEWORK_SERVICE_ID);

	if (!export) {
		// Nothing needs to be done: we're not interested...
		return status;
	}

	switch (event->type) {
		case OSGI_FRAMEWORK_SERVICE_EVENT_REGISTERED:
			status = topologyManager_addExportedService(manager, event->reference, serviceId);
			break;
		case OSGI_FRAMEWORK_SERVICE_EVENT_MODIFIED:
			status = topologyManager_removeExportedService(manager, event->reference, serviceId);
			status = topologyManager_addExportedService(manager, event->reference, serviceId);
			break;
		case OSGI_FRAMEWORK_SERVICE_EVENT_UNREGISTERING:
			status = topologyManager_removeExportedService(manager, event->reference, serviceId);
			break;
		case OSGI_FRAMEWORK_SERVICE_EVENT_MODIFIED_ENDMATCH:
			break;
	}

	return status;
}

celix_status_t topologyManager_addImportedService(void *handle, endpoint_description_pt endpoint, char *matchedFilter) {
	celix_status_t status = CELIX_SUCCESS;
	topology_manager_pt manager = handle;

	fw_log(logger, OSGI_FRAMEWORK_LOG_INFO, "TOPOLOGY_MANAGER: Add imported service (%s; %s).", endpoint->service, endpoint->id);

	// Create a local copy of the current list of RSAs, to ensure we do not run into threading issues...
	array_list_pt localRSAs = NULL;
	topologyManager_getRSAs(manager, &localRSAs);

	status = celixThreadMutex_lock(&manager->importedServicesLock);

	hash_map_pt imports = hashMap_create(NULL, NULL, NULL, NULL);
	hashMap_put(manager->importedServices, endpoint, imports);

	int size = arrayList_size(localRSAs);
	for (int iter = 0; iter < size; iter++) {
		remote_service_admin_service_pt rsa = arrayList_get(localRSAs, iter);

		import_registration_pt import = NULL;
		status = rsa->importService(rsa->admin, endpoint, &import);
		if (status == CELIX_SUCCESS) {
			hashMap_put(imports, rsa, import);
		}
	}

	status = celixThreadMutex_unlock(&manager->importedServicesLock);

	return status;
}

celix_status_t topologyManager_removeImportedService(void *handle, endpoint_description_pt endpoint, char *matchedFilter) {
	celix_status_t status = CELIX_SUCCESS;
	topology_manager_pt manager = handle;

	fw_log(logger, OSGI_FRAMEWORK_LOG_INFO, "TOPOLOGY_MANAGER: Remove imported service (%s; %s).", endpoint->service, endpoint->id);

	status = celixThreadMutex_lock(&manager->importedServicesLock);

	hash_map_iterator_pt iter = hashMapIterator_create(manager->importedServices);
	while (hashMapIterator_hasNext(iter)) {
	    hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
	    endpoint_description_pt ep = hashMapEntry_getKey(entry);
	    hash_map_pt imports = hashMapEntry_getValue(entry);

	    if (strcmp(endpoint->id, ep->id) == 0) {
	        hash_map_iterator_pt importsIter = hashMapIterator_create(imports);

            while (hashMapIterator_hasNext(importsIter)) {
                hash_map_entry_pt entry = hashMapIterator_nextEntry(importsIter);

                remote_service_admin_service_pt rsa = hashMapEntry_getKey(entry);
                import_registration_pt import = hashMapEntry_getValue(entry);

                status = rsa->importRegistration_close(rsa->admin, import);
                if (status == CELIX_SUCCESS) {
                    hashMap_remove(imports, rsa);
                }
            }

            hashMapIterator_destroy(importsIter);
	    }
	}
	hashMapIterator_destroy(iter);

	status = celixThreadMutex_unlock(&manager->importedServicesLock);

	return status;
}

celix_status_t topologyManager_addExportedService(topology_manager_pt manager, service_reference_pt reference, char *serviceId) {
	celix_status_t status = CELIX_SUCCESS;

	fw_log(logger, OSGI_FRAMEWORK_LOG_INFO, "TOPOLOGY_MANAGER: Add exported service (%s).", serviceId);

	// Create a local copy of the current list of RSAs, to ensure we do not run into threading issues...
	array_list_pt localRSAs = NULL;
	topologyManager_getRSAs(manager, &localRSAs);

	status = celixThreadMutex_lock(&manager->exportedServicesLock);

	hash_map_pt exports = hashMap_create(NULL, NULL, NULL, NULL);
	hashMap_put(manager->exportedServices, reference, exports);

	int size = arrayList_size(localRSAs);
	for (int iter = 0; iter < size; iter++) {
		remote_service_admin_service_pt rsa = arrayList_get(localRSAs, iter);

		array_list_pt endpoints = NULL;
		status = rsa->exportService(rsa->admin, serviceId, NULL, &endpoints);

		if (status == CELIX_SUCCESS) {
			hashMap_put(exports, rsa, endpoints);
			status = topologyManager_notifyListenersEndpointAdded(manager, rsa, endpoints);
		}
	}

	status = celixThreadMutex_unlock(&manager->exportedServicesLock);

	return status;
}

celix_status_t topologyManager_removeExportedService(topology_manager_pt manager, service_reference_pt reference, char *serviceId) {
	celix_status_t status = CELIX_SUCCESS;

	fw_log(logger, OSGI_FRAMEWORK_LOG_INFO, "TOPOLOGY_MANAGER: Remove exported service (%s).", serviceId);

	status = celixThreadMutex_lock(&manager->exportedServicesLock);

	hash_map_pt exports = hashMap_get(manager->exportedServices, reference);
	if (exports) {
		hash_map_iterator_pt iter = hashMapIterator_create(exports);
		while (hashMapIterator_hasNext(iter)) {
			hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);

			remote_service_admin_service_pt rsa = hashMapEntry_getKey(entry);
			array_list_pt exports = hashMapEntry_getValue(entry);

			for (int exportsIter = 0; exportsIter < arrayList_size(exports); exportsIter++) {
				export_registration_pt export = arrayList_get(exports, exportsIter);
				rsa->exportRegistration_close(export);

				topologyManager_notifyListenersEndpointRemoved(manager, rsa, export);
			}
			arrayList_destroy(exports);
			exports = NULL;
		}
		hashMapIterator_destroy(iter);
	}

	status = celixThreadMutex_unlock(&manager->exportedServicesLock);

	return status;
}

celix_status_t topologyManager_getEndpointDescriptionForExportRegistration(remote_service_admin_service_pt rsa, export_registration_pt export, endpoint_description_pt *endpoint) {
	celix_status_t status = CELIX_SUCCESS;

	export_reference_pt reference = NULL;
	status = rsa->exportRegistration_getExportReference(export, &reference);
	if (status != CELIX_SUCCESS) {
		return status;
	}

	status = rsa->exportReference_getExportedEndpoint(reference, endpoint);

	return status;
}

celix_status_t topologyManager_notifyListenersEndpointAdded(topology_manager_pt manager, remote_service_admin_service_pt rsa, array_list_pt registrations) {
	celix_status_t status = CELIX_SUCCESS;

	array_list_pt endpointListeners = NULL;
	status = bundleContext_getServiceReferences(manager->context, OSGI_ENDPOINT_LISTENER_SERVICE, NULL, &endpointListeners);
	if (status != CELIX_SUCCESS || !endpointListeners) {
		return CELIX_BUNDLE_EXCEPTION;
	}

	int eplSize = arrayList_size(endpointListeners);
	for (int eplIt = 0; eplIt < eplSize; eplIt++) {
		service_reference_pt eplRef = arrayList_get(endpointListeners, eplIt);

		service_registration_pt registration = NULL;
		serviceReference_getServiceRegistration(eplRef, &registration);

		properties_pt props = NULL;
		serviceRegistration_getProperties(registration, &props);
		char *scope = properties_get(props, (char *) OSGI_ENDPOINT_LISTENER_SCOPE);

		endpoint_listener_pt epl = NULL;
		status = bundleContext_getService(manager->context, eplRef, (void **) &epl);
		if (status != CELIX_SUCCESS || !epl) {
			continue;
		}

		filter_pt filter = filter_create(scope);

		int regSize = arrayList_size(registrations);
		for (int regIt = 0; regIt < regSize; regIt++) {
			export_registration_pt export = arrayList_get(registrations, regIt);

			endpoint_description_pt endpoint = NULL;
			status = topologyManager_getEndpointDescriptionForExportRegistration(rsa, export, &endpoint);
			if (status != CELIX_SUCCESS || !endpoint) {
				continue;
			}

			bool matchResult = false;
			filter_match(filter, endpoint->properties, &matchResult);
			if (matchResult) {
				status = epl->endpointAdded(epl->handle, endpoint, scope);
			}
		}

		filter_destroy(filter);
	}

	if (endpointListeners) {
		arrayList_destroy(endpointListeners);
	}

	return status;
}

celix_status_t topologyManager_notifyListenersEndpointRemoved(topology_manager_pt manager, remote_service_admin_service_pt rsa,  export_registration_pt export) {
	celix_status_t status = CELIX_SUCCESS;

	array_list_pt endpointListeners = NULL;
	status = bundleContext_getServiceReferences(manager->context, OSGI_ENDPOINT_LISTENER_SERVICE, NULL, &endpointListeners);
	if (status != CELIX_SUCCESS || !endpointListeners) {
		return CELIX_BUNDLE_EXCEPTION;
	}

	for (int eplIt = 0; eplIt < arrayList_size(endpointListeners); eplIt++) {
		service_reference_pt eplRef = arrayList_get(endpointListeners, eplIt);

		endpoint_listener_pt epl = NULL;
		status = bundleContext_getService(manager->context, eplRef, (void **) &epl);
		if (status != CELIX_SUCCESS || !epl) {
			continue;
		}

		endpoint_description_pt endpoint = NULL;
		status = topologyManager_getEndpointDescriptionForExportRegistration(rsa, export, &endpoint);
		if (status != CELIX_SUCCESS || !endpoint) {
			continue;
		}

		status = epl->endpointRemoved(epl->handle, endpoint, NULL);
	}

	if (endpointListeners) {
		arrayList_destroy(endpointListeners);
	}

	return status;
}

celix_status_t topologyManager_extendFilter(bundle_context_pt context, char *filter, char **updatedFilter) {
	celix_status_t status = CELIX_SUCCESS;

	char* uuid = NULL;
	status = bundleContext_getProperty(context, (char *)OSGI_FRAMEWORK_FRAMEWORK_UUID, &uuid);
	if (!uuid) {
		fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "TOPOLOGY_MANAGER: no framework UUID defined?!");
		return CELIX_BUNDLE_EXCEPTION;
	}

	int len = 10 + strlen(filter) + strlen(OSGI_RSA_ENDPOINT_FRAMEWORK_UUID) + strlen(uuid);
	*updatedFilter = malloc(len);
	if (!*updatedFilter) {
		return CELIX_ENOMEM;
	}

	snprintf(*updatedFilter, len, "(&%s(!(%s=%s)))", filter, OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, uuid);

	return status;
}

celix_status_t topologyManager_listenerAdded(void *handle, array_list_pt listeners) {
	celix_status_t status = CELIX_SUCCESS;
	topology_manager_pt manager = handle;

	for (int i = 0; i < arrayList_size(listeners); i++) {
		listener_hook_info_pt info = arrayList_get(listeners, i);

		bundle_pt bundle = NULL, self = NULL;
		bundleContext_getBundle(info->context, &bundle);
		bundleContext_getBundle(manager->context, &self);
		if (bundle == self) {
			fw_log(logger, OSGI_FRAMEWORK_LOG_DEBUG, "TOPOLOGY_MANAGER: Ignore myself.");
			continue;
		}

		fw_log(logger, OSGI_FRAMEWORK_LOG_INFO, "TOPOLOGY_MANAGER: listener with filter \"%s\" added", info->filter);

		char *filter = NULL;
		topologyManager_extendFilter(manager->context, info->filter, &filter);

		status = celixThreadMutex_lock(&manager->importInterestsLock);

		struct import_interest *interest = hashMap_get(manager->importInterests, filter);
		if (interest) {
			interest->refs++;
		} else {
			interest = malloc(sizeof(*interest));
			interest->filter = filter;
			interest->refs = 1;
			hashMap_put(manager->importInterests, filter, interest);
		}

		status = celixThreadMutex_unlock(&manager->importInterestsLock);
	}

	return status;
}

celix_status_t topologyManager_listenerRemoved(void *handle, array_list_pt listeners) {
	celix_status_t status = CELIX_SUCCESS;
	topology_manager_pt manager = handle;

	for (int i = 0; i < arrayList_size(listeners); i++) {
		listener_hook_info_pt info = arrayList_get(listeners, i);

		bundle_pt bundle = NULL, self = NULL;
		bundleContext_getBundle(info->context, &bundle);
		bundleContext_getBundle(manager->context, &self);
		if (bundle == self) {
			fw_log(logger, OSGI_FRAMEWORK_LOG_DEBUG, "TOPOLOGY_MANAGER: Ignore myself.");
			continue;
		}

		fw_log(logger, OSGI_FRAMEWORK_LOG_INFO, "TOPOLOGY_MANAGER: listener with filter \"%s\" removed.", info->filter);

		char *filter = NULL;
		topologyManager_extendFilter(manager->context, info->filter, &filter);

		status = celixThreadMutex_lock(&manager->importInterestsLock);

		struct import_interest *interest = hashMap_get(manager->importInterests, filter);
		if (interest != NULL && interest->refs-- <= 0) {
			// last reference, remove from scope
			interest = hashMap_remove(manager->importInterests, filter);
		}

		status = celixThreadMutex_unlock(&manager->importInterestsLock);
	}

	return status;
}
