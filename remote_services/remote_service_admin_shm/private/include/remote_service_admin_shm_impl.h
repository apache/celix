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
 * remote_service_admin_shm_impl.h
 *
 *  \date       Sep 30, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef REMOTE_SERVICE_ADMIN_SHM_IMPL_H_
#define REMOTE_SERVICE_ADMIN_SHM_IMPL_H_

#include "remote_service_admin_impl.h"

#define RSA_SHM_MEMSIZE 327680
#define RSA_SHM_PATH_PROPERTYNAME "shmPath"
#define RSA_SEM_PATH_PROPERTYNAME "semPath"
#define RSA_SHM_FTOK_ID_PROPERTYNAME "shmFtokId"
#define RSA_SEM_FTOK_ID_PROPERTYNAME "semFtokId"
#define RSA_SHM_DEFAULTPATH "/dev/null"
#define RSA_SEM_DEFAULTPATH "/dev/null"
#define RSA_SHM_DEFAULT_FTOK_ID "52"
#define RSA_SEM_DEFAULT_FTOK_ID "54"

struct recv_shm_thread
{
	remote_service_admin_pt admin;
	endpoint_description_pt endpointDescription;
};

struct ipc_segment
{
	int semId;
	int shmId;
	void *shmBaseAdress;
};

struct remote_service_admin {
	apr_pool_t *pool;
	bundle_context_pt context;

	hash_map_pt exportedServices;
	hash_map_pt importedServices;
	hash_map_pt exportedIpcSegment;
	hash_map_pt importedIpcSegment;

	hash_map_pt pollThread;
	hash_map_pt pollThreadRunning;

	struct mg_context *ctx;
};

typedef struct recv_shm_thread *recv_shm_thread_pt;
typedef struct ipc_segment *ipc_segment_pt;

celix_status_t remoteServiceAdmin_stop(remote_service_admin_pt admin);


#endif /* REMOTE_SERVICE_ADMIN_SHM_IMPL_H_ */
