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


#ifndef PUBSUB_TOPOLOGY_MANAGER_H_
#define PUBSUB_TOPOLOGY_MANAGER_H_

#include "service_reference.h"
#include "bundle_context.h"
#include "log_helper.h"
#include "command.h"
#include "celix_bundle_context.h"

#include "pubsub_common.h"
#include "pubsub_endpoint.h"
#include "pubsub/publisher.h"
#include "pubsub/subscriber.h"

#define PUBSUB_TOPOLOGY_MANAGER_VERBOSE_KEY 		"PUBSUB_TOPOLOGY_MANAGER_VERBOSE"
#define PUBSUB_TOPOLOGY_MANAGER_DEFAULT_VERBOSE		false


struct pubsub_topology_manager {
	bundle_context_pt context;

	celix_thread_mutex_t psaListLock;
	array_list_pt psaList;

	celix_thread_mutex_t discoveryListLock;
	hash_map_pt discoveryList; //<svcId,NULL>

	celix_thread_mutex_t publicationsLock;
	hash_map_pt publications; //<topic(string),list<pubsub_ep>>

	celix_thread_mutex_t subscriptionsLock;
	hash_map_pt subscriptions; //<topic(string),list<pubsub_ep>>

	command_service_t shellCmdService;
	service_registration_pt  shellCmdReg;


	log_helper_pt loghelper;

	bool verbose;
};

typedef struct pubsub_topology_manager pubsub_topology_manager_t;

celix_status_t pubsub_topologyManager_create(bundle_context_pt context, log_helper_pt logHelper, pubsub_topology_manager_t **manager);
celix_status_t pubsub_topologyManager_destroy(pubsub_topology_manager_t *manager);
celix_status_t pubsub_topologyManager_closeImports(pubsub_topology_manager_t *manager);

void pubsub_topologyManager_psaAdded(void *handle, void *svc, const celix_properties_t *props);
void pubsub_topologyManager_psaRemoved(void *handle, void *svc, const celix_properties_t *props);

void pubsub_topologyManager_pubsubDiscoveryAdded(void* handle, void *svc, const celix_properties_t *props);
void pubsub_topologyManager_pubsubDiscoveryRemoved(void * handle, void *svc, const celix_properties_t *props);

void pubsub_topologyManager_subscriberAdded(void * handle, void *svc, const celix_properties_t *props, const celix_bundle_t *bnd);
void pubsub_topologyManager_subscriberRemoved(void * handle, void *svc, const celix_properties_t *props, const celix_bundle_t *bnd);

void pubsub_topologyManager_publisherTrackerAdded(void *handle, const celix_service_tracker_info_t *info);
void pubsub_topologyManager_publisherTrackerRemoved(void *handle, const celix_service_tracker_info_t *info);

celix_status_t pubsub_topologyManager_addDiscoveredEndpoint(void *handle, const celix_properties_t *properties);
celix_status_t pubsub_topologyManager_removeDiscoveredEndpoint(void *handle, const celix_properties_t *properties);

#endif /* PUBSUB_TOPOLOGY_MANAGER_H_ */
