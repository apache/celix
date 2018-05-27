/*
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

#ifndef DISC_MOCK_SERVICE_H
#define DISC_MOCK_SERVICE_H

#include "bundle_context.h"
#include "endpoint_listener.h"
#include "service_registration.h"

#define DISC_MOCK_SERVICE_NAME "disc_mock_service"

struct disc_mock_service {
    void *handle;// disc_mock_activator_pt
    celix_status_t (*getEPDescriptors)(void *handle, array_list_pt *descrList);
};

typedef struct disc_mock_service *disc_mock_service_pt;

struct disc_mock_activator {
	bundle_context_pt context;
	disc_mock_service_pt serv;
	service_registration_pt  reg;

//	service_tracker_customizer_pt cust;
//	service_tracker_pt tracker;
    endpoint_listener_pt endpointListener;
    service_registration_pt endpointListenerService;

    array_list_pt endpointList;
};





celix_status_t discMockService_create(void *handle, disc_mock_service_pt *serv);
celix_status_t discMockService_destroy(disc_mock_service_pt serv);

#endif //CELIX_TST_SERVICE_H
