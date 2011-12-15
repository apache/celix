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
 * dependency_activator.c
 *
 *  Created on: Aug 23, 2010
 *      Author: alexanderb
 */
#include <stdlib.h>
#include <stdio.h>
#include "celixbool.h"

#include "dependency_activator_base.h"
#include "service_component_private.h"
#include "log_writer.h"
#include "log_service.h"
#include "bundle_context.h"

void * dm_create(BUNDLE_CONTEXT context) {
    apr_pool_t *pool;

    bundleContext_getMemoryPool(context, &pool);

    log_writer_t writer = NULL;
    logWriter_create(pool, &writer);
	return writer;
}

void dm_init(void * userData, BUNDLE_CONTEXT context, DEPENDENCY_MANAGER manager) {
	log_writer_t data = (log_writer_t) userData;

	SERVICE service = dependencyActivatorBase_createService(manager);
	serviceComponent_setImplementation(service, data);

	data->dep = dependencyActivatorBase_createServiceDependency(manager);
	serviceDependency_setRequired(data->dep, false);
	serviceDependency_setAutoConfigure(data->dep, (void**) &(data->logReader));
	serviceDependency_setService(data->dep, (char *) LOG_READER_SERVICE_NAME, NULL);
	serviceComponent_addServiceDependency(service, data->dep);

	data->service = service;
	dependencyManager_add(manager, service);
}

void dm_destroy(void * userData, BUNDLE_CONTEXT context, DEPENDENCY_MANAGER manager) {
    log_writer_t data = (log_writer_t) userData;
//    serviceComponent_removeServiceDependency(data->service, data->dep);
	dependencyManager_remove(manager, data->service);
	//free(data);
}

