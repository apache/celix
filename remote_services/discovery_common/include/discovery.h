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
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef DISCOVERY_H_
#define DISCOVERY_H_

#include "discovery_type.h"
#include "bundle_context.h"
#include "service_reference.h"

#include "endpoint_description.h"
#include "endpoint_listener.h"

#include "endpoint_discovery_server.h"
#include "endpoint_discovery_poller.h"

#include "log_helper.h"

#define DISCOVERY_SERVER_INTERFACE	"DISCOVERY_CFG_SERVER_INTERFACE"
#define DISCOVERY_SERVER_IP 		"DISCOVERY_CFG_SERVER_IP"
#define DISCOVERY_SERVER_PORT 		"DISCOVERY_CFG_SERVER_PORT"
#define DISCOVERY_SERVER_PATH 		"DISCOVERY_CFG_SERVER_PATH"
#define DISCOVERY_POLL_ENDPOINTS 	"DISCOVERY_CFG_POLL_ENDPOINTS"
#define DISCOVERY_SERVER_MAX_EP	"DISCOVERY_CFG_SERVER_MAX_EP"

struct discovery {
    bundle_context_t* context;

    celix_thread_mutex_t listenerReferencesMutex;
    celix_thread_mutex_t discoveredServicesMutex;

    hash_map_t* listenerReferences; //key=serviceReference, value=nop
    hash_map_t* discoveredServices; //key=endpointId (string), value=endpoint_description_pt

    endpoint_discovery_poller_t* poller;
    endpoint_discovery_server_t* server;

    log_helper_t* loghelper;

    discovery_impl_t* pImpl;
};


/* those one could be put into a general discovery.h - file */
celix_status_t discovery_create(bundle_context_pt context, discovery_pt *discovery);
celix_status_t discovery_destroy(discovery_pt discovery);

celix_status_t discovery_start(discovery_pt discovery);
celix_status_t discovery_stop(discovery_pt discovery);

celix_status_t discovery_endpointAdded(void *handle, endpoint_description_pt endpoint, char *machtedFilter);
celix_status_t discovery_endpointRemoved(void *handle, endpoint_description_pt endpoint, char *machtedFilter);

celix_status_t discovery_endpointListenerAdding(void * handle, service_reference_pt reference, void **service);
celix_status_t discovery_endpointListenerAdded(void * handle, service_reference_pt reference, void * service);
celix_status_t discovery_endpointListenerModified(void * handle, service_reference_pt reference, void * service);
celix_status_t discovery_endpointListenerRemoved(void * handle, service_reference_pt reference, void * service);

celix_status_t discovery_informEndpointListeners(discovery_pt discovery, endpoint_description_pt endpoint, bool endpointAdded);
celix_status_t discovery_updateEndpointListener(discovery_pt discovery, service_reference_pt reference, endpoint_listener_pt service);

celix_status_t discovery_addDiscoveredEndpoint(discovery_pt discovery, endpoint_description_pt endpoint);
celix_status_t discovery_removeDiscoveredEndpoint(discovery_pt discovery, endpoint_description_pt endpoint);

#endif /* DISCOVERY_H_ */
