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
 * updated_thread_pool.c
 *
 *  \date       Aug 12, 2013
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* celix.config_admin.UpdatedThreadPool */
#include "thpool.h"
#include "updated_thread_pool.h"



struct updated_thread_pool{

	bundle_context_pt 	context;

	int maxTreads;

	//	apr_thread_mutex_t *mutex;
	threadpool threadPool;	//protected by mutex

};

typedef struct data_callback *data_callback_t;

struct data_callback{

	managed_service_service_pt managedServiceService;
	properties_pt properties;

};


static void *updateThreadPool_updatedCallback(void *data);
static celix_status_t updatedThreadPool_wrapDataCallback(managed_service_service_pt service, properties_pt properties, data_callback_t *data);


/* ========== CONSTRUCTOR ========== */

/* ---------- public ---------- */

celix_status_t updatedThreadPool_create(bundle_context_pt context, int maxThreads, updated_thread_pool_pt *updatedThreadPool){

	*updatedThreadPool = calloc(1, sizeof(**updatedThreadPool));
	if (!*updatedThreadPool){
		printf("[ ERROR ]: UpdatedThreadPool - Not initialized (ENOMEM) \n");
		return CELIX_ENOMEM;
	}

	(*updatedThreadPool)->threadPool=thpool_init(maxThreads);
//	if ( apr_thread_pool_create(&(*updatedThreadPool)->threadPool, INIT_THREADS, maxTreads, pool) != APR_SUCCESS ){
	if ((*updatedThreadPool)->threadPool == NULL) {
		printf("[ ERROR ]: UpdatedThreadPool - Instance not created \n");
		return CELIX_ENOMEM;
	}

	(*updatedThreadPool)->context = context;

	printf("[ SUCCESS ]: UpdatedThreadPool - initialized \n");
	return CELIX_SUCCESS;

}

celix_status_t updatedThreadPool_destroy(updated_thread_pool_pt pool) {
	thpool_destroy(pool->threadPool);
	free(pool);
	return CELIX_SUCCESS;
}
/* ========== IMPLEMENTATION ========== */

/* ---------- public ---------- */

celix_status_t updatedThreadPool_push(updated_thread_pool_pt updatedThreadPool, managed_service_service_pt service, properties_pt properties){

	data_callback_t data = NULL;

	if ( updatedThreadPool_wrapDataCallback(service, properties, &data) != CELIX_SUCCESS ){
		return CELIX_ILLEGAL_ARGUMENT;
	}

	if (thpool_add_work(updatedThreadPool->threadPool, updateThreadPool_updatedCallback, data) != 0) {
		printf("[ ERROR ]: UpdatedThreadPool - add_work \n ");
		return CELIX_ILLEGAL_STATE;
	}

	return CELIX_SUCCESS;
}

/* ---------- private ---------- */

void *updateThreadPool_updatedCallback(void *data) {

	data_callback_t params = data;

	managed_service_service_pt managedServiceService = params->managedServiceService;
	properties_pt properties = params->properties;

	(*managedServiceService->updated)(managedServiceService->managedService, properties);

	free(data);

	return NULL;

}

celix_status_t updatedThreadPool_wrapDataCallback(managed_service_service_pt service, properties_pt properties, data_callback_t *data){

	*data = calloc(1, sizeof(**data));

	if (!*data){
		printf("[ ERROR ]: UpdatedThreadPool - WrapDataCallback (Data not initialized) \n");
		return CELIX_ENOMEM;
	}

	(*data)->managedServiceService = service;
	(*data)->properties = properties;

	return CELIX_SUCCESS;
}

