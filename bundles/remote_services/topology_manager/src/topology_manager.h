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
 * topology_manager.h
 *
 *  \date       Sep 29, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef TOPOLOGY_MANAGER_H_
#define TOPOLOGY_MANAGER_H_

#include "endpoint_listener.h"
#include "service_reference.h"
#include "bundle_context.h"
#include "celix_log_helper.h"
#include "scope.h"


typedef struct topology_manager topology_manager_t;
typedef struct topology_manager *topology_manager_pt;

celix_status_t topologyManager_create(celix_bundle_context_t *context, celix_log_helper_t *logHelper, topology_manager_pt *manager, void **scope);
celix_status_t topologyManager_destroy(topology_manager_pt manager);
celix_status_t topologyManager_closeImports(topology_manager_pt manager);

celix_status_t topologyManager_rsaAdding(void *handle, service_reference_pt reference, void **service);
celix_status_t topologyManager_rsaAdded(void *handle, service_reference_pt reference, void *service);
celix_status_t topologyManager_rsaModified(void *handle, service_reference_pt reference, void *service);
celix_status_t topologyManager_rsaRemoved(void *handle, service_reference_pt reference, void *service);

celix_status_t topologyManager_endpointListenerAdding(void* handle, service_reference_pt reference, void** service);
celix_status_t topologyManager_endpointListenerAdded(void* handle, service_reference_pt reference, void* service);
celix_status_t topologyManager_endpointListenerModified(void * handle, service_reference_pt reference, void* service);
celix_status_t topologyManager_endpointListenerRemoved(void * handle, service_reference_pt reference, void* service);

celix_status_t topologyManager_serviceChanged(void *listener, celix_service_event_t *event);

celix_status_t topologyManager_addImportedService(void *handle, endpoint_description_t *endpoint, char *matchedFilter);
celix_status_t topologyManager_removeImportedService(void *handle, endpoint_description_t *endpoint, char *matchedFilter);

celix_status_t topologyManager_addExportedService(void * handle, service_reference_pt reference, void * service);
celix_status_t topologyManager_removeExportedService(void * handle, service_reference_pt reference, void * service);

celix_status_t topologyManager_listenerAdded(void *handle, celix_array_list_t* listeners);
celix_status_t topologyManager_listenerRemoved(void *handle, celix_array_list_t* listeners);

#endif /* TOPOLOGY_MANAGER_H_ */
