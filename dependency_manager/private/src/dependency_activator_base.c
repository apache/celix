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
 *  \date       Aug 8, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>

#include "bundle_activator.h"
#include "dependency_manager.h"
#include "service_component_private.h"
#include "dependency_activator_base.h"

struct dependencyActivatorBase {
	dependency_manager_pt manager;
	bundle_context_pt context;
	void * userData;
};

typedef struct dependencyActivatorBase * dependency_activator_base_pt;

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	apr_pool_t *pool;
	apr_pool_t *npool = NULL;
	celix_status_t status = bundleContext_getMemoryPool(context, &pool);
	apr_pool_create(&npool, pool);
	if (status == CELIX_SUCCESS) {
		*userData = apr_palloc(npool, sizeof(dependency_activator_base_pt));
		((dependency_activator_base_pt)(*userData))->userData = dm_create(context);;
	} else {
		status = CELIX_START_ERROR;
	}

	return status;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;

	dependency_activator_base_pt data = (dependency_activator_base_pt) userData;
	data->manager = dependencyManager_create(context);
	data->context = context;
	dm_init(data->userData, data->context, data->manager);

	return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;

	dependency_activator_base_pt data = (dependency_activator_base_pt) userData;
	dm_destroy(data->userData, data->context, data->manager);
	data->userData = NULL;
	data->context = NULL;
	data->manager = NULL;

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	return status;
}

service_pt dependencyActivatorBase_createService(dependency_manager_pt manager) {
	return serviceComponent_create(manager->context, manager);
}

service_dependency_pt dependencyActivatorBase_createServiceDependency(dependency_manager_pt manager) {
	return serviceDependency_create(manager->context);
}

