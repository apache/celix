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
 * updated_thread_pool.c
 *
 *  \date       Aug 12, 2013
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */


#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* celix.config_admin.UpdatedThreadPool */
#include "updated_thread_pool.h"

/* APR */
#include <apr_thread_mutex.h>
#include <apr_thread_pool.h>


struct updated_thread_pool{

	apr_pool_t 		*pool;
	bundle_context_pt 	context;

	apr_int64_t maxTreads;

	//	apr_thread_mutex_t *mutex;
	apr_thread_pool_t *threadPool;	//protected by mutex

};

typedef struct data_callback *data_callback_t;

struct data_callback{

	apr_pool_t 		*pool;	//independent pool

	managed_service_service_pt managedServiceService;
	properties_pt properties;

};


static void *APR_THREAD_FUNC updateThreadPool_updatedCallback(apr_thread_t *thread, void *data);
static celix_status_t updatedThreadPool_wrapDataCallback(managed_service_service_pt service, properties_pt properties, data_callback_t *data);


/* ========== CONSTRUCTOR ========== */

/* ---------- public ---------- */

celix_status_t updatedThreadPool_create(apr_pool_t *pool, bundle_context_pt context, apr_int64_t maxTreads, updated_thread_pool_t *updatedThreadPool){

	*updatedThreadPool = apr_palloc(pool, sizeof(**updatedThreadPool));
	if (!*updatedThreadPool){
		printf("[ ERROR ]: UpdatedThreadPool - Not initialized (ENOMEM) \n");
		return CELIX_ENOMEM;
	}

	if ( apr_thread_pool_create(&(*updatedThreadPool)->threadPool, INIT_THREADS, maxTreads, pool) != APR_SUCCESS ){
		printf("[ ERROR ]: UpdatedThreadPool - Instance not created \n");
		return CELIX_ENOMEM;
	}

	(*updatedThreadPool)->pool = pool;
	(*updatedThreadPool)->context = context;

	(*updatedThreadPool)->maxTreads = maxTreads;

	printf("[ SUCCESS ]: UpdatedThreadPool - initialized \n");
	return CELIX_SUCCESS;

}

/* ========== IMPLEMENTATION ========== */

/* ---------- public ---------- */

celix_status_t updatedThreadPool_push(updated_thread_pool_t updatedThreadPool, managed_service_service_pt service, properties_pt properties){

	data_callback_t data = NULL;

	// TODO apr_thread_mutex_lock(instance->mutex)?

	if ( apr_thread_pool_busy_count(updatedThreadPool->threadPool) < updatedThreadPool->maxTreads ) {

		if ( updatedThreadPool_wrapDataCallback(service, properties, &data) != CELIX_SUCCESS ){
			return CELIX_ILLEGAL_ARGUMENT;
		}


		if ( apr_thread_pool_push(updatedThreadPool->threadPool, updateThreadPool_updatedCallback, data,
				APR_THREAD_TASK_PRIORITY_NORMAL, NULL) != APR_SUCCESS ){

			printf("[ ERROR ]: UpdatedThreadPool - Push (apr_thread_pool_push) \n ");
			return CELIX_ILLEGAL_STATE;

		}

	} else {

		printf("[ ERROR ]: UpdatedThreadPool - Push (Full!) \n ");
		return CELIX_ILLEGAL_STATE;

	}

	printf("[ SUCCESS ]:  UpdatedThreadPool - Push \n");
	return CELIX_SUCCESS;
}

/* ---------- private ---------- */

void *APR_THREAD_FUNC updateThreadPool_updatedCallback(apr_thread_t *thread, void *data) {


	printf("[ DEBUG ]: UpdatedThreadPool - Callback \n");

	data_callback_t params = data;

	managed_service_service_pt managedServiceService = params->managedServiceService;
	properties_pt properties = params->properties;

	apr_pool_destroy(params->pool);

	(*managedServiceService->updated)(managedServiceService->managedService, properties);

	return NULL;

}

celix_status_t updatedThreadPool_wrapDataCallback(managed_service_service_pt service, properties_pt properties, data_callback_t *data){

	apr_pool_t *pool;

	if ( apr_pool_create(&pool,NULL) != APR_SUCCESS ){
		printf("[ ERROR ]: UpdatedThreadPool - WrapDataCallback (Data not created) \n");
		return CELIX_ILLEGAL_ARGUMENT;
	}

	*data = apr_palloc(pool, sizeof(**data));
	if (!*data){
		printf("[ ERROR ]: UpdatedThreadPool - WrapDataCallback (Data not initialized) \n");
		return CELIX_ENOMEM;
	}

	(*data)->pool = pool;
	(*data)->managedServiceService = service;
	(*data)->properties = properties;

	return CELIX_SUCCESS;
}

