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
 * discovery.h
 *
 *  \date       Sep 29, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef DISCOVERY_H_
#define DISCOVERY_H_

#define DISCOVERY_SHM_MEMSIZE 32768
#define DISCOVERY_SHM_FILENAME "/dev/null"
#define DISCOVERY_SHM_FTOK_ID 50
#define DISCOVERY_SEM_FILENAME "/dev/null"
#define DISCOVERY_SEM_FTOK_ID 52

#include "endpoint_listener.h"


struct ipc_shmData
{
	int semId;
	int numListeners;
	char data[DISCOVERY_SHM_MEMSIZE - (2* (sizeof(int) + sizeof(key_t)))];
};

typedef struct discovery *discovery_pt;
typedef struct ipc_shmData *ipc_shmData_pt;

celix_status_t discovery_create(apr_pool_t *pool, bundle_context_pt context, discovery_pt *discovery);
celix_status_t discovery_stop(discovery_pt discovery);

celix_status_t discovery_endpointAdded(void *handle, endpoint_description_pt endpoint, char *machtedFilter);
celix_status_t discovery_endpointRemoved(void *handle, endpoint_description_pt endpoint, char *machtedFilter);

celix_status_t discovery_endpointListenerAdding(void * handle, service_reference_pt reference, void **service);
celix_status_t discovery_endpointListenerAdded(void * handle, service_reference_pt reference, void * service);
celix_status_t discovery_endpointListenerModified(void * handle, service_reference_pt reference, void * service);
celix_status_t discovery_endpointListenerRemoved(void * handle, service_reference_pt reference, void * service);

celix_status_t discovery_updateEndpointListener(discovery_pt discovery, service_reference_pt reference, endpoint_listener_pt service);


#endif /* DISCOVERY_H_ */
