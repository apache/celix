/*
 * dm_event.c
 *
 *  Created on: 18 Dec 2014
 *      Author: abroekhuis
 */

#include <stdlib.h>

#include "dm_event.h"

celix_status_t event_create(dm_event_type_e event_type, bundle_pt bundle, bundle_context_pt context, service_reference_pt reference, void *service, dm_event_pt *event) {
	celix_status_t status = CELIX_SUCCESS;

	*event = calloc(1, sizeof(**event));
	if (!*event) {
		status = CELIX_ENOMEM;
	}

	if (status == CELIX_SUCCESS) {
		(*event)->bundle = bundle;
		(*event)->event_type = event_type;
		(*event)->context = context;
		(*event)->reference = reference;
		(*event)->service = service;
	}

	return status;
}

celix_status_t event_destroy(dm_event_pt *event) {
	celix_status_t status = CELIX_SUCCESS;

	if (!*event) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		free(*event);
		*event = NULL;
	}

	return status;
}
