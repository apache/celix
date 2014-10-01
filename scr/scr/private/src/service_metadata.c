/*
 * service.c
 *
 *  Created on: Jun 28, 2012
 *      Author: alexander
 */
#include <stdlib.h>

#include "array_list.h"

#include "service_metadata.h"

struct service {
	bool serviceFactory;

	ARRAY_LIST provides;

};

celix_status_t service_create(apr_pool_t *pool, service_t *service) {
	*service = malloc(sizeof(**service));
	(*service)->serviceFactory = false;
	arrayList_create(pool, &(*service)->provides);

	return CELIX_SUCCESS;
}

celix_status_t serviceMetadata_setServiceFactory(service_t currentService, bool serviceFactory) {
	currentService->serviceFactory = serviceFactory;
	return CELIX_SUCCESS;
}

celix_status_t serviceMetadata_isServiceFactory(service_t currentService, bool *serviceFactory) {
	*serviceFactory = currentService->serviceFactory;
	return CELIX_SUCCESS;
}

celix_status_t serviceMetadata_addProvide(service_t service, char *provide) {
	arrayList_add(service->provides, provide);
	return CELIX_SUCCESS;
}

celix_status_t serviceMetadata_getProvides(service_t service, char **provides[], int *size) {
	*size = arrayList_size(service->provides);
	char **providesA = malloc(*size * sizeof(*providesA));
	int i;
	for (i = 0; i < *size; i++) {
		providesA[i] = arrayList_get(service->provides, i);
	}

	*provides = providesA;
	return CELIX_SUCCESS;
}
