/*
 * Licensed under Apache License v2.
 */
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <service_tracker_customizer.h>
#include <service_tracker.h>

#include "bundle_activator.h"
#include "bundle_context.h"
#include "service_registration.h"
#include "service_reference.h"
#include "celix_errno.h"

#include "disc_mock_service.h"


celix_status_t test(void *handle, array_list_pt *descrList);

celix_status_t discMockService_create(void *handle, disc_mock_service_pt *serv)
{
	*serv = calloc(1, sizeof(struct disc_mock_service));
	if (*serv == NULL)
		return CELIX_ENOMEM;

    (*serv)->handle = handle;
	(*serv)->getEPDescriptors = test;
}

celix_status_t discMockService_destroy(disc_mock_service_pt serv)
{
	free(serv);
}

celix_status_t test(void *handle, array_list_pt *descrList)
{
    struct disc_mock_activator *act = handle;
    *descrList = act->endpointList;

	return 0;
}
