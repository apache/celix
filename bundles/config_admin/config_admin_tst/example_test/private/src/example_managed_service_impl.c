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
/**
 * example_managed_service_impl.c
 *
 *  \date       Aug 12, 2013
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdlib.h>
#include <stdio.h>

#include "example_managed_service_impl.h"

/* ------------------------ Constructor -------------------------------------*/

celix_status_t managedServiceImpl_create(bundle_context_pt context, managed_service_pt *instance) {

	celix_status_t status = CELIX_SUCCESS;

	struct test_managed_service *managedService = calloc(1, sizeof(*managedService));
	if(!managedService){
		printf("[ ERROR ]: ManagedServiceImpl - Not initialized (ENOMEM) \n");
		return CELIX_ENOMEM;
	}

	managedService->context = context;
	managedService->registration = NULL;
	managedService->properties = NULL;

	printf("[ ManagedServiceImpl ]: ManagedServiceImpl - Initialized \n");
	*instance = (managed_service_pt)managedService;
	return status;
}


celix_status_t managedServiceImpl_destroy(managed_service_pt *instance) {
    free(*instance);

    return CELIX_SUCCESS;
}

/* -------------------- Implementation --------------------------------------*/

celix_status_t managedServiceImpl_updated(managed_service_pt managedService, properties_pt properties){
	struct test_managed_service *msp = (struct test_managed_service *) managedService;

	if (properties == NULL){
		printf("[ managedServiceImpl ]: updated - Received NULL properties \n");
		msp->store_props(msp->handle, "", "");
	}else{
		printf("[ managedServiceImpl ]: updated - Received New Properties \n");
		char *value = (char*)properties_get(properties, "type");
		char *value2 = (char*)properties_get(properties, "second_type");
		msp->store_props(msp->handle, value, value2);
		// it would be nicer if we get the property values here and store them in the activator structure.
	}

	return CELIX_SUCCESS;
}

























