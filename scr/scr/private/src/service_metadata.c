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
 * service_metadata.c
 *
 *  \date       Nov 3, 2015
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
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
