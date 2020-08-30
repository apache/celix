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

#ifndef PUBSUB_DISCOVERY_IMPL_H_
#define PUBSUB_DISCOVERY_IMPL_H_

#include <celix_log_helper.h>
#include "bundle_context.h"
#include "service_reference.h"

#include "pubsub_endpoint.h"
#include "etcd.h"

#define FREE_MEM(ptr) if(ptr) {free(ptr); ptr = NULL;}

#define PUBSUB_ETCD_DISCOVERY_VERBOSE_KEY "PUBSUB_ETCD_DISCOVERY_VERBOSE"
#define PUBSUB_ETCD_DISCOVERY_DEFAULT_VERBOSE false


#define PUBSUB_DISCOVERY_SERVER_IP_KEY          "PUBSUB_DISCOVERY_ETCD_SERVER_IP"
#define PUBSUB_DISCOVERY_SERVER_PORT_KEY        "PUBSUB_DISCOVERY_ETCD_SERVER_PORT"
#define PUBSUB_DISCOVERY_SERVER_PATH_KEY        "PUBSUB_DISCOVERY_ETCD_ROOT_PATH"
#define PUBSUB_DISCOVERY_ETCD_TTL_KEY           "PUBSUB_DISCOVERY_ETCD_TTL"


#define PUBSUB_DISCOVERY_SERVER_IP_DEFAULT      "127.0.0.1"
#define PUBSUB_DISCOVERY_SERVER_PORT_DEFAULT    2379
#define PUBSUB_DISCOVERY_SERVER_PATH_DEFAULT    "pubsub/"
#define PUBSUB_DISCOVERY_ETCD_TTL_DEFAULT       30

typedef struct pubsub_discovery {
    celix_bundle_context_t *context;
    celix_log_helper_t *logHelper;

    celix_thread_mutex_t discoveredEndpointsMutex; //when locked with EndpointsListenersMutex -> first lock this
    hash_map_pt discoveredEndpoints; //<key = uuid,celix_properties_t /*endpoint*/>>

    celix_thread_mutex_t announcedEndpointsMutex;
    hash_map_pt announcedEndpoints; //<key = char* (etcd key),pubsub_announce_entry_t /*endpoint*/>>

    celix_thread_mutex_t discoveredEndpointsListenersMutex;
    hash_map_pt discoveredEndpointsListeners; //key=svcId, value=pubsub_discovered_endpoint_listener_t

    celix_thread_mutex_t runningMutex;
    bool running;
    celix_thread_cond_t  waitCond;
    celix_thread_t watchThread;
    celix_thread_t refreshTTLThread;

    //configurable by config/env.
    const char *pubsubPath;
    bool verbose;
    etcdlib_t *etcdlib;
    int ttlForEntries;
    int sleepInsecBetweenTTLRefresh;
    const char *fwUUID;
} pubsub_discovery_t;

typedef struct pubsub_announce_entry {
    char *key; //etcd key
    bool isSet; //whether the value is already set (in case of unavailable etcd server this can linger)
    int refreshCount;
    int setCount;
    int errorCount;
    celix_properties_t *properties; //the endpoint properties
    struct timespec createTime; //from MONOTONIC clock
} pubsub_announce_entry_t;


pubsub_discovery_t* pubsub_discovery_create(celix_bundle_context_t *context, celix_log_helper_t *logHelper);
celix_status_t pubsub_discovery_destroy(pubsub_discovery_t *node_discovery);
celix_status_t pubsub_discovery_start(pubsub_discovery_t *node_discovery);
celix_status_t pubsub_discovery_stop(pubsub_discovery_t *node_discovery);

void pubsub_discovery_discoveredEndpointsListenerAdded(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *bnd);
void pubsub_discovery_discoveredEndpointsListenerRemoved(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *bnd);

celix_status_t pubsub_discovery_announceEndpoint(void *handle, const celix_properties_t *endpoint);
celix_status_t pubsub_discovery_revokeEndpoint(void *handle, const celix_properties_t *endpoint);

bool pubsub_discovery_executeCommand(void *handle, const char * commandLine, FILE *os, FILE *errorStream);


#endif /* PUBSUB_DISCOVERY_IMPL_H_ */
