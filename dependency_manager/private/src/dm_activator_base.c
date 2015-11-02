/**
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

/*
 * dm_activator_base.c
 *
 *  \date       26 Jul 2014
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */


#include <stdlib.h>

#include "bundle_activator.h"
#include "dm_activator_base.h"
#include "dm_info.h"

struct dm_dependency_activator_base {
	dm_dependency_manager_pt manager;
	bundle_context_pt context;
	service_registration_pt reg;
	dm_info_service_pt info;
	void* userData;
};

typedef struct dm_dependency_activator_base * dependency_activator_base_pt;

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	celix_status_t status = CELIX_ENOMEM;

	dependency_activator_base_pt dependency_activator = calloc(1, sizeof(struct dm_dependency_activator_base));
	dm_info_service_pt serv = calloc(1, sizeof(*serv));

	if (dependency_activator != NULL && serv != NULL) {
		dependency_activator->context = context;
		dm_create(context, &dependency_activator->userData);
		dependency_activator->info = serv;

        status = dependencyManager_create(dependency_activator->context, &dependency_activator->manager);
	} else {
        status = CELIX_ENOMEM;

	}

    if (status == CELIX_SUCCESS) {
        *userData = dependency_activator;
    } else {
        if (dependency_activator != NULL) {
            dependencyManager_destroy(dependency_activator->manager);
        }
        free(dependency_activator);
        free(serv);
    }

	return status;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
	celix_status_t status;
	dependency_activator_base_pt dependency_activator = (dependency_activator_base_pt) userData;


    status = dm_init(dependency_activator->userData, context, dependency_activator->manager);

    if (status == CELIX_SUCCESS) {
        //Create the service
        dependency_activator->info->handle = dependency_activator->manager;
        dependency_activator->info->getInfo = (void *) dependencyManager_getInfo;
        dependency_activator->info->destroyInfo = (void *) dependencyManager_destroyInfo;

        status = bundleContext_registerService(context, DM_INFO_SERVICE_NAME, dependency_activator->info, NULL,
                                               &(dependency_activator->reg));
    }

	return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context __attribute__((unused))) {
	celix_status_t status = CELIX_SUCCESS;
    dependency_activator_base_pt dependency_activator = (dependency_activator_base_pt) userData;

    // Remove the service
    status = serviceRegistration_unregister(dependency_activator->reg);
    depedencyManager_removeAllComponents(dependency_activator->manager);

    return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context __attribute__((unused))) {
	celix_status_t status = CELIX_SUCCESS;
	dependency_activator_base_pt dependency_activator = (dependency_activator_base_pt) userData;

    status = dm_destroy(dependency_activator->userData, dependency_activator->context,
                        dependency_activator->manager);

    if (status == CELIX_SUCCESS) {
        dependencyManager_destroy(dependency_activator->manager);
    }

	dependency_activator->userData = NULL;
	dependency_activator->manager = NULL;

	if (dependency_activator != NULL) {
		free(dependency_activator->info);
	}
	free(dependency_activator);

	return status;
}


