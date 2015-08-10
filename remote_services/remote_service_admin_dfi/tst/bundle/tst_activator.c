/*
 * Licensed under Apache License v2. See LICENSE for more information.
 */
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "bundle_activator.h"
#include "bundle_context.h"
#include "service_registration.h"
#include "service_reference.h"
#include "celix_errno.h"

#include "tst_service.h"
#include "calculator_service.h"


struct activator {
	bundle_context_pt context;
	struct tst_service serv;
	service_registration_pt  reg;
};

static void test(void *handle);

celix_status_t bundleActivator_create(bundle_context_pt context, void **out) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *act = calloc(1, sizeof(*act));
	if (act != NULL) {
		act->context = context;
		act->serv.handle = act;
		act->serv.test = test;
	} else {
		status = CELIX_ENOMEM;
	}

	if (status == CELIX_SUCCESS) {
		*out = act;
	}

	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
    celix_status_t status = CELIX_SUCCESS;
	struct activator * act = userData;

	act->reg = NULL;
	status = bundleContext_registerService(context, (char *)TST_SERVICE_NAME, &act->serv, NULL, &act->reg);

	return status;
}


celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
    celix_status_t status = CELIX_SUCCESS;
	struct activator * act = userData;

	status = serviceRegistration_unregister(act->reg);

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
	free(userData);
	return CELIX_SUCCESS;
}

static void test(void *handle) {
	struct activator *act = handle;
	bundle_context_pt context = act->context;
	//TODO improve. don't use asserts.

	int rc = 0;
	service_reference_pt ref = NULL;
	calculator_service_pt calc = NULL;

	rc = bundleContext_getServiceReference(context, (char *)CALCULATOR2_SERVICE, &ref);
	assert(rc == 0);

	rc = bundleContext_getService(context, ref, (void **)&calc);
	assert(rc == 0);

	double result = 0.0;
	rc = calc->sqrt(calc->calculator, 4, &result);
	assert(rc == 0);
	assert(result == 2);

	bundleContext_ungetService(context, ref, NULL);
	bundleContext_ungetServiceReference(context, ref);
}
