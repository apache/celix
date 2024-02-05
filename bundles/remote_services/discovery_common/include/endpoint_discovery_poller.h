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
 * endpoint_discovery_poller.h
 *
 * \date       3 Jul 2014
 * \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 * \copyright  Apache License, Version 2.0
 */

#ifndef ENDPOINT_DISCOVERY_POLLER_H_
#define ENDPOINT_DISCOVERY_POLLER_H_

#include "celix_errno.h"
#include "discovery_type.h"
#include "celix_log_helper.h"
#include "celix_threads.h"
#include "hash_map.h"
#include "celix_array_list.h"

typedef struct endpoint_discovery_poller endpoint_discovery_poller_t;

struct endpoint_discovery_poller {
    discovery_t *discovery;
    hash_map_pt entries;
    celix_log_helper_t **loghelper;

    celix_thread_mutex_t pollerLock;
    celix_thread_t pollerThread;

    unsigned int poll_interval;
    unsigned int poll_timeout;

    volatile bool running;
};

celix_status_t endpointDiscoveryPoller_create(discovery_t *discovery, celix_bundle_context_t *context, const char* defaultPollEndpoints, endpoint_discovery_poller_t **poller);
celix_status_t endpointDiscoveryPoller_destroy(endpoint_discovery_poller_t *poller);

celix_status_t endpointDiscoveryPoller_addDiscoveryEndpoint(endpoint_discovery_poller_t *poller, char *url);
celix_status_t endpointDiscoveryPoller_removeDiscoveryEndpoint(endpoint_discovery_poller_t *poller, char *url);

celix_status_t endpointDiscoveryPoller_getDiscoveryEndpoints(endpoint_discovery_poller_t *poller, celix_array_list_t* urls);

#endif /* ENDPOINT_DISCOVERY_POLLER_H_ */
