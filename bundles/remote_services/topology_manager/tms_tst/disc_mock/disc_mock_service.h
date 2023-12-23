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

#ifndef DISC_MOCK_SERVICE_H
#define DISC_MOCK_SERVICE_H

#include "bundle_context.h"
#include "endpoint_listener.h"
#include "service_registration.h"

#define DISC_MOCK_SERVICE_NAME "disc_mock_service"

typedef struct disc_mock_service {
    void *handle;// disc_mock_activator_t*
    celix_status_t (*getEPDescriptors)(void *handle, celix_array_list_t** descrList);
} disc_mock_service_t;


struct disc_mock_activator {
	celix_bundle_context_t *context;
	disc_mock_service_t *serv;
	service_registration_t * reg;

//	service_tracker_customizer_t *cust;
//	service_tracker_t *tracker;
    endpoint_listener_t *endpointListener;
    service_registration_t *endpointListenerService;

    celix_array_list_t* endpointList;
};


celix_status_t discMockService_create(void *handle, disc_mock_service_t **serv);
celix_status_t discMockService_destroy(disc_mock_service_t *serv);

#endif //CELIX_TST_SERVICE_H
