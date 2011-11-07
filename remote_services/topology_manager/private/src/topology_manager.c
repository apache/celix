/*
 * topology_manager.c
 *
 *  Created on: Sep 29, 2011
 *      Author: alexander
 */
#include <stdio.h>
#include <stdlib.h>

#include "headers.h"
#include "topology_manager.h"
#include "bundle_context.h"
#include "constants.h"
#include "module.h"
#include "bundle.h"
#include "remote_service_admin.h"
#include "remote_constants.h"

struct topology_manager {
	apr_pool_t *pool;
	BUNDLE_CONTEXT context;

	ARRAY_LIST rsaList;
	HASH_MAP exportedServices;
};

celix_status_t topologyManager_notifyListeners(topology_manager_t manager, remote_service_admin_service_t rsa,  ARRAY_LIST registrations);

celix_status_t topologyManager_create(BUNDLE_CONTEXT context, apr_pool_t *pool, topology_manager_t *manager) {
	celix_status_t status = CELIX_SUCCESS;

	*manager = apr_palloc(pool, sizeof(**manager));
	if (!*manager) {
		status = CELIX_ENOMEM;
	} else {
		(*manager)->pool = pool;
		(*manager)->context = context;
		(*manager)->rsaList = arrayList_create();
		(*manager)->exportedServices = hashMap_create(NULL, NULL, NULL, NULL);
	}

	return status;
}

celix_status_t topologyManager_rsaAdding(void * handle, SERVICE_REFERENCE reference, void **service) {
	celix_status_t status = CELIX_SUCCESS;
	topology_manager_t manager = handle;

	bundleContext_getService(manager->context, reference, service);

	return status;
}

celix_status_t topologyManager_rsaAdded(void * handle, SERVICE_REFERENCE reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;
	topology_manager_t manager = handle;

	printf("TOPOLOGY_MANAGER: Added RSA\n");
	arrayList_add(manager->rsaList, service);

	return status;
}

celix_status_t topologyManager_rsaModified(void * handle, SERVICE_REFERENCE reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;
	topology_manager_t manager = handle;
	return status;
}

celix_status_t topologyManager_rsaRemoved(void * handle, SERVICE_REFERENCE reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;
	topology_manager_t manager = handle;

	printf("TOPOLOGY_MANAGER: Removed RSA\n");
	arrayList_removeElement(manager->rsaList, service);

	return status;
}

celix_status_t topologyManager_serviceChanged(void *listener, SERVICE_EVENT event) {
	celix_status_t status = CELIX_SUCCESS;
	SERVICE_LISTENER listen = listener;
	topology_manager_t manager = listen->handle;
	PROPERTIES props = event->reference->registration->properties;
	char *name = properties_get(props, (char *) OBJECTCLASS);
	char *export = properties_get(props, (char *) SERVICE_EXPORTED_INTERFACES);

	if (event->type == REGISTERED) {
		if (export != NULL) {
			printf("TOPOLOGY_MANAGER: Service registered: %s\n", name);
			topologyManager_exportService(manager, event->reference);
		}
	} else if (event->type == UNREGISTERING) {
		printf("TOPOLOGY_MANAGER: Service unregistering: %s\n", name);
		topologyManager_removeService(manager, event->reference);
	}

	return status;
}

celix_status_t topologyManager_endpointAdded(void *handle, endpoint_description_t endpoint, char *machtedFilter) {
	celix_status_t status = CELIX_SUCCESS;
	topology_manager_t manager = handle;
	printf("TOPOLOGY_MANAGER: Endpoint added\n");

	topologyManager_importService(manager, endpoint);


	return status;
}

celix_status_t topologyManager_endpointRemoved(void *handle, endpoint_description_t endpoint, char *machtedFilter) {
	celix_status_t status = CELIX_SUCCESS;
	return status;
}

celix_status_t topologyManager_exportService(topology_manager_t manager, SERVICE_REFERENCE reference) {
	celix_status_t status = CELIX_SUCCESS;
	HASH_MAP exports = hashMap_create(NULL, NULL, NULL, NULL);

	hashMap_put(manager->exportedServices, reference, exports);

	if (arrayList_size(manager->rsaList) == 0) {
		char *symbolicName = module_getSymbolicName(bundle_getCurrentModule(reference->bundle));
		printf("TOPOLOGY_MANAGER: No RemoteServiceAdmin available, unable to export service from bundle %s.\n", symbolicName);
	} else {
		int size = arrayList_size(manager->rsaList);
		int iter = 0;
		for (iter = 0; iter < size; iter++) {
			remote_service_admin_service_t rsa = arrayList_get(manager->rsaList, iter);

			ARRAY_LIST endpoints = NULL;
			rsa->exportService(rsa->admin, reference, NULL, &endpoints);
			hashMap_put(exports, rsa, endpoints);
			topologyManager_notifyListeners(manager, rsa, endpoints);
		}
	}

	return status;
}

celix_status_t topologyManager_notifyListeners(topology_manager_t manager, remote_service_admin_service_t rsa,  ARRAY_LIST registrations) {
	celix_status_t status = CELIX_SUCCESS;
	ARRAY_LIST endpointListeners = NULL;

	status = bundleContext_getServiceReferences(manager->context, endpoint_listener_service, NULL, &endpointListeners);
	if (status == CELIX_SUCCESS) {
		if (endpointListeners != NULL) {
			int eplIt;
			for (eplIt = 0; eplIt < arrayList_size(endpointListeners); eplIt++) {
				SERVICE_REFERENCE eplRef = arrayList_get(endpointListeners, eplIt);
				endpoint_listener_t epl = NULL;
				status = bundleContext_getService(manager->context, eplRef, (void **) &epl);
				if (status == CELIX_SUCCESS) {
					int regIt;
					for (regIt = 0; regIt < arrayList_size(registrations); regIt++) {
						export_registration_t export = arrayList_get(registrations, regIt);
						export_reference_t reference = NULL;
						endpoint_description_t endpoint = NULL;
						rsa->exportRegistration_getExportReference(export, &reference);
						rsa->exportReference_getExportedEndpoint(reference, &endpoint);
						status = epl->endpointAdded(epl->handle, endpoint, NULL);
					}
				}
			}
		}
	}

	return status;
}

celix_status_t topologyManager_importService(topology_manager_t manager, endpoint_description_t endpoint) {
	celix_status_t status = CELIX_SUCCESS;
	HASH_MAP exports = hashMap_create(NULL, NULL, NULL, NULL);

	//hashMap_put(manager->exportedServices, reference, exports);

	if (arrayList_size(manager->rsaList) == 0) {
		printf("TOPOLOGY_MANAGER: No RemoteServiceAdmin available, unable to import service %s.\n", endpoint->service);
	} else {
		int size = arrayList_size(manager->rsaList);
		int iter = 0;
		for (iter = 0; iter < size; iter++) {
			remote_service_admin_service_t rsa = arrayList_get(manager->rsaList, iter);

			import_registration_t import = NULL;
			rsa->importService(rsa->admin, endpoint, &import);
			//hashMap_put(exports, rsa, endpoints);
		}
	}

	return status;
}

celix_status_t topologyManager_removeService(topology_manager_t manager, SERVICE_REFERENCE reference) {
	celix_status_t status = CELIX_SUCCESS;
	char *name = properties_get(reference->registration->properties, (char *) OBJECTCLASS);

	printf("TOPOLOGY_MANAGER: Remove Service: %s.\n", name);

	return status;
}
