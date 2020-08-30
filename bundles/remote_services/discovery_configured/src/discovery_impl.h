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
 * discovery_impl.h
 *
 *  \date       Sep 29, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef DISCOVERY_IMPL_H_
#define DISCOVERY_IMPL_H_

#include "bundle_context.h"
#include "service_reference.h"

#include "endpoint_description.h"
#include "endpoint_listener.h"

#include "endpoint_discovery_poller.h"
#include "endpoint_discovery_server.h"

#include "celix_log_helper.h"

#define DEFAULT_SERVER_IP 	"127.0.0.1"
#define DEFAULT_SERVER_PORT "9999"
#define DEFAULT_SERVER_PATH "/org.apache.celix.discovery.configured"
#define DEFAULT_POLL_ENDPOINTS "http://localhost:9999/org.apache.celix.discovery.configured"


//struct discovery_impl {
//	celix_bundle_context_t *context;
//
//	celix_thread_mutex_t listenerReferencesMutex;
//	celix_thread_mutex_t discoveredServicesMutex;
//
//	hash_map_pt listenerReferences; //key=serviceReference, value=nop
//	hash_map_pt discoveredServices; //key=endpointId (string), value=endpoint_description_t *
//
//	endpoint_discovery_poller_t *poller;
//	endpoint_discovery_server_t *server;
//
//	log_helper_t *loghelper;
//};

#endif /* DISCOVERY_IMPL_H_ */
