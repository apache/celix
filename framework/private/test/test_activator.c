/*
 * test_activator.c
 *
 *  Created on: Aug 10, 2011
 *      Author: alexander
 */

#include "bundle_activator.h"

celix_status_t bundleActivator_create(BUNDLE_CONTEXT context, void **userData) {
	*userData = NULL;
	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_start(void * userData, BUNDLE_CONTEXT context) {
	sleep(1);
	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_stop(void * userData, BUNDLE_CONTEXT context) {
	sleep(1);
	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_destroy(void * userData, BUNDLE_CONTEXT context) {
	return CELIX_SUCCESS;
}

