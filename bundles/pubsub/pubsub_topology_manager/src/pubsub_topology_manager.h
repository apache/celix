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

#ifndef PUBSUB_TOPOLOGY_MANAGER_H_
#define PUBSUB_TOPOLOGY_MANAGER_H_

#include "service_reference.h"
#include "bundle_context.h"
#include "celix_log_helper.h"
#include "celix_shell_command.h"
#include "celix_bundle_context.h"

#include "pubsub_endpoint.h"
#include "pubsub/publisher.h"
#include "pubsub/subscriber.h"

#define PUBSUB_TOPOLOGY_MANAGER_VERBOSE_KEY         "PUBSUB_TOPOLOGY_MANAGER_VERBOSE"
#define PUBSUB_TOPOLOGY_MANAGER_HANDLING_THREAD_SLEEPTIME_SECONDS_KEY   "PUBSUB_TOPOLOGY_MANAGER_HANDLING_THREAD_SLEEPTIME_SECONDS"
#define PUBSUB_TOPOLOGY_MANAGER_HANDLING_THREAD_SLEEPTIME_MS_KEY        "PUBSUB_TOPOLOGY_MANAGER_HANDLING_THREAD_SLEEPTIME_MS"
#define PUBSUB_TOPOLOGY_MANAGER_DEFAULT_VERBOSE     false


typedef struct pubsub_topology_manager {
    celix_bundle_context_t *context;

    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map; //key = svcId, value = pubsub_admin_t*
    } pubsubadmins;

    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map; //key = uuid , value = pstm_discovered_endpoint_entry_t
    } discoveredEndpoints;

    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map; //key = scope/topic key, value = pstm_topic_receiver_or_sender_entry_t*
    } topicReceivers;

    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map; //key = scope/topic key, value = pstm_topic_receiver_or_sender_entry_t*
    } topicSenders;

    struct {
        celix_thread_mutex_t mutex;
        celix_array_list_t *list; //<pubsub_announce_endpoint_listener_t*>
    } announceEndpointListeners;

    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map; //key = svcId, value = pubsub_admin_metrics_service_t*
    } psaMetrics;

    struct {
        celix_thread_t thread;
        celix_thread_mutex_t mutex; //protect running and condition
        celix_thread_cond_t cond;
        bool running;
    } psaHandling;

    celix_log_helper_t *loghelper;

    unsigned handlingThreadSleepTime;
    bool verbose;
} pubsub_topology_manager_t;

typedef struct pstm_discovered_endpoint_entry {
    const char *uuid;
    long selectedPsaSvcId; // -1L, indicates no selected psa
    int usageCount; //note that discovered endpoints can be found multiple times by different pubsub discovery components
    celix_properties_t *endpoint;
} pstm_discovered_endpoint_entry_t;

typedef struct pstm_topic_receiver_or_sender_entry {
    char *scopeAndTopicKey; //key of the combined value of the scope and topic
    celix_properties_t *endpoint;
    char *topic;
    char *scope;
    int usageCount; //nr of provided subscriber services for the topic receiver (matching scope & topic) or nr of publisher requested for matching scope & topic.
    long bndId;
    celix_properties_t *topicProperties; //found in META-INF/(pub|sub)/(topic).properties

    //for sender entry
    celix_filter_t *publisherFilter;

    //for receiver entry
    celix_properties_t *subscriberProperties;

    struct {
        bool needsMatch; //true if a psa needs to be selected or if a new psa has to be considered.
        long selectedPsaSvcId;
        long selectedSerializerSvcId;
        long selectedProtocolSvcId;
    } matching;
} pstm_topic_receiver_or_sender_entry_t;

celix_status_t pubsub_topologyManager_create(celix_bundle_context_t *context, celix_log_helper_t *logHelper, pubsub_topology_manager_t **manager);
celix_status_t pubsub_topologyManager_destroy(pubsub_topology_manager_t *manager);

void pubsub_topologyManager_psaAdded(void *handle, void *svc, const celix_properties_t *props);
void pubsub_topologyManager_psaRemoved(void *handle, void *svc, const celix_properties_t *props);

void pubsub_topologyManager_pubsubAnnounceEndpointListenerAdded(void* handle, void *svc, const celix_properties_t *props);
void pubsub_topologyManager_pubsubAnnounceEndpointListenerRemoved(void * handle, void *svc, const celix_properties_t *props);

void pubsub_topologyManager_subscriberAdded(void * handle, void *svc, const celix_properties_t *props, const celix_bundle_t *bnd);
void pubsub_topologyManager_subscriberRemoved(void * handle, void *svc, const celix_properties_t *props, const celix_bundle_t *bnd);

void pubsub_topologyManager_publisherTrackerAdded(void *handle, const celix_service_tracker_info_t *info);
void pubsub_topologyManager_publisherTrackerRemoved(void *handle, const celix_service_tracker_info_t *info);

celix_status_t pubsub_topologyManager_addDiscoveredEndpoint(void *handle, const celix_properties_t *properties);
celix_status_t pubsub_topologyManager_removeDiscoveredEndpoint(void *handle, const celix_properties_t *properties);

bool pubsub_topologyManager_shellCommand(void *handle, const char * commandLine, FILE *outStream, FILE *errorStream);

void pubsub_topologyManager_addMetricsService(void * handle, void *svc, const celix_properties_t *props);
void pubsub_topologyManager_removeMetricsService(void * handle, void *svc, const celix_properties_t *props);

#endif /* PUBSUB_TOPOLOGY_MANAGER_H_ */
