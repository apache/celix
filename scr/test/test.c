/*
 * scr.c
 *
 *  Created on: Jun 25, 2012
 *      Author: alexander
 */

#include <stdio.h>

#include <bundle_activator.h>

struct service {
	char *a;
};

typedef struct service *service_t;

celix_status_t bundleActivator_create(BUNDLE_CONTEXT context, void **userData) {
	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_start(void * userData, BUNDLE_CONTEXT context) {
	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_stop(void * userData, BUNDLE_CONTEXT context) {
	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_destroy(void * userData, BUNDLE_CONTEXT context) {
	return CELIX_SUCCESS;
}

void activate() {
	printf("Activate!\n");
}
