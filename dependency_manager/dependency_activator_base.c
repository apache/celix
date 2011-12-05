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
 * dependency_activator_base.c
 *
 *  Created on: Aug 8, 2010
 *      Author: alexanderb
 */
#include <stdlib.h>

#include "bundle_activator.h"
#include "dependency_manager.h"
#include "service_component_private.h"
#include "dependency_activator_base.h"

struct dependencyActivatorBase {
	DEPENDENCY_MANAGER manager;
	BUNDLE_CONTEXT context;
	void * userData;
};

typedef struct dependencyActivatorBase * DEPENDENCY_ACTIVATOR_BASE;

celix_status_t bundleActivator_create(BUNDLE_CONTEXT context, void **userData) {
	apr_pool_t *pool;
	apr_pool_t *npool = NULL;
	celix_status_t status = bundleContext_getMemoryPool(context, &pool);
	apr_pool_create(&npool, pool);
	if (status == CELIX_SUCCESS) {
		*userData = apr_palloc(npool, sizeof(DEPENDENCY_ACTIVATOR_BASE));
		((DEPENDENCY_ACTIVATOR_BASE)(*userData))->userData = dm_create(context);;
	} else {
		status = CELIX_START_ERROR;
	}

	return status;
}

celix_status_t bundleActivator_start(void * userData, BUNDLE_CONTEXT context) {
	celix_status_t status = CELIX_SUCCESS;

	DEPENDENCY_ACTIVATOR_BASE data = (DEPENDENCY_ACTIVATOR_BASE) userData;
	data->manager = dependencyManager_create(context);
	data->context = context;
	dm_init(data->userData, data->context, data->manager);

	return status;
}

celix_status_t bundleActivator_stop(void * userData, BUNDLE_CONTEXT context) {
	celix_status_t status = CELIX_SUCCESS;

	DEPENDENCY_ACTIVATOR_BASE data = (DEPENDENCY_ACTIVATOR_BASE) userData;
	dm_destroy(data->userData, data->context, data->manager);
	data->userData = NULL;
	data->context = NULL;
	data->manager = NULL;

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, BUNDLE_CONTEXT context) {
	celix_status_t status = CELIX_SUCCESS;
	return status;
}

SERVICE dependencyActivatorBase_createService(DEPENDENCY_MANAGER manager) {
	return serviceComponent_create(manager->context, manager);
}

SERVICE_DEPENDENCY dependencyActivatorBase_createServiceDependency(DEPENDENCY_MANAGER manager) {
	return serviceDependency_create(manager->context);
}

