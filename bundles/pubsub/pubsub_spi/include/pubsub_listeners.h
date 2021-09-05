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

#ifndef PUBSUB_LISTENERS_H_
#define PUBSUB_LISTENERS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "celix_properties.h"

#define PUBSUB_DISCOVERED_ENDPOINT_LISTENER_SERVICE "pubsub_discovered_endpoint_listener"

//Informs the topology manager that pub/sub endpoints are discovered in the network
struct pubsub_discovered_endpoint_listener {
    void *handle;

    celix_status_t (*addDiscoveredEndpoint)(void *handle, const celix_properties_t *properties);
    celix_status_t (*removeDiscoveredEndpoint)(void *handle, const celix_properties_t *properties);
};
typedef struct pubsub_discovered_endpoint_listener pubsub_discovered_endpoint_listener_t;



#define PUBSUB_ANNOUNCE_ENDPOINT_LISTENER_SERVICE "pubsub_announce_endpoint_listener"

//Informs the pubsub discoveries to announce/revoke endpoint
struct pubsub_announce_endpoint_listener {
    void *handle;

    celix_status_t (*announceEndpoint)(void *handle, const celix_properties_t *properties);
    celix_status_t (*revokeEndpoint)(void *handle, const celix_properties_t *properties);
};

typedef struct pubsub_announce_endpoint_listener pubsub_announce_endpoint_listener_t;

#ifdef __cplusplus
}
#endif
#endif /* PUBSUB_LISTENERS_H_ */
