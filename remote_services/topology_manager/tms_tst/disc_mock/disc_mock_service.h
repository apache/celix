/*
 * Licensed under Apache License v2. See LICENSE for more information.
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
