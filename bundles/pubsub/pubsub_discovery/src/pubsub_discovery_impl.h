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

#ifndef PUBSUB_DISCOVERY_IMPL_H_
#define PUBSUB_DISCOVERY_IMPL_H_

#include "bundle_context.h"
#include "service_reference.h"

#include "etcd_watcher.h"
#include "etcd_writer.h"
#include "pubsub_endpoint.h"

#define FREE_MEM(ptr) if(ptr) {free(ptr); ptr = NULL;}

#define PUBSUB_ETCD_DISCOVERY_VERBOSE_KEY "PUBSUB_ETCD_DISCOVERY_VERBOSE"
#define PUBSUB_ETCD_DISCOVERY_DEFAULT_VERBOSE false

struct watcher_info {
    etcd_watcher_pt watcher;
    int nr_references;
};

struct pubsub_discovery {
	bundle_context_pt context;

	celix_thread_mutex_t discoveredPubsMutex;
	hash_map_pt discoveredPubs; //<topic,List<pubsub_endpoint_pt>>

	celix_thread_mutex_t listenerReferencesMutex;
	hash_map_pt listenerReferences; //key=serviceReference, value=nop

	celix_thread_mutex_t watchersMutex;
	hash_map_pt watchers; //key = topicname, value = struct watcher_info

	etcd_writer_pt writer;

	bool verbose;
};


celix_status_t pubsub_discovery_create(bundle_context_pt context, pubsub_discovery_pt* node_discovery);
celix_status_t pubsub_discovery_destroy(pubsub_discovery_pt node_discovery);
celix_status_t pubsub_discovery_start(pubsub_discovery_pt node_discovery);
celix_status_t pubsub_discovery_stop(pubsub_discovery_pt node_discovery);

celix_status_t pubsub_discovery_addNode(pubsub_discovery_pt node_discovery, pubsub_endpoint_pt pubEP);
celix_status_t pubsub_discovery_removeNode(pubsub_discovery_pt node_discovery, pubsub_endpoint_pt pubEP);

celix_status_t pubsub_discovery_tmPublisherAnnounceAdded(void * handle, service_reference_pt reference, void * service);
celix_status_t pubsub_discovery_tmPublisherAnnounceModified(void * handle, service_reference_pt reference, void * service);
celix_status_t pubsub_discovery_tmPublisherAnnounceRemoved(void * handle, service_reference_pt reference, void * service);

celix_status_t pubsub_discovery_announcePublisher(void *handle, pubsub_endpoint_pt pubEP);
celix_status_t pubsub_discovery_removePublisher(void *handle, pubsub_endpoint_pt pubEP);
celix_status_t pubsub_discovery_interestedInTopic(void *handle, const char* scope, const char* topic);
celix_status_t pubsub_discovery_uninterestedInTopic(void *handle, const char* scope, const char* topic);

celix_status_t pubsub_discovery_informPublishersListeners(pubsub_discovery_pt discovery, pubsub_endpoint_pt endpoint, bool endpointAdded);

#endif /* PUBSUB_DISCOVERY_IMPL_H_ */
