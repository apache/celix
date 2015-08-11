/*
 * Licensed under Apache License v2. See LICENSE for more information.
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

#include "tst_service.h"
#include "calculator_service.h"


struct activator {
	bundle_context_pt context;
	struct tst_service serv;
	service_registration_pt  reg;

	service_tracker_customizer_pt cust;
	service_tracker_pt tracker;
	calculator_service_pt calc;
};

static celix_status_t addCalc(void * handle, service_reference_pt reference, void * service);
static celix_status_t removeCalc(void * handle, service_reference_pt reference, void * service);
static int test(void *handle);

celix_status_t bundleActivator_create(bundle_context_pt context, void **out) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *act = calloc(1, sizeof(*act));
	if (act != NULL) {
		act->context = context;
		act->serv.handle = act;
		act->serv.test = test;

		status = serviceTrackerCustomizer_create(act, NULL, addCalc, NULL, removeCalc, &act->cust);
		status = CELIX_DO_IF(status, serviceTracker_create(context, CALCULATOR2_SERVICE, act->cust, &act->tracker));

	} else {
		status = CELIX_ENOMEM;
	}

	if (status == CELIX_SUCCESS) {
		*out = act;
	} else if (act != NULL) {
		if (act->cust != NULL) {
			free(act->cust);
			act->cust = NULL;
		}
		if (act->tracker != NULL) {
			serviceTracker_destroy(act->tracker);
			act->tracker = NULL;
		}
		free(act);
	}

	return CELIX_SUCCESS;
}

static celix_status_t addCalc(void * handle, service_reference_pt reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator * act = handle;
	act->calc = service;
	return status;
}

static celix_status_t removeCalc(void * handle, service_reference_pt reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator * act = handle;
	if (act->calc == service) {
		act->calc = NULL;
	}
	return status;

}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
    celix_status_t status = CELIX_SUCCESS;
	struct activator * act = userData;

	act->reg = NULL;
	status = bundleContext_registerService(context, (char *)TST_SERVICE_NAME, &act->serv, NULL, &act->reg);

	status = CELIX_DO_IF(status, serviceTracker_open(act->tracker));


	return status;
}


celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
    celix_status_t status = CELIX_SUCCESS;
	struct activator * act = userData;

	status = serviceRegistration_unregister(act->reg);
	status = CELIX_DO_IF(status, serviceTracker_close(act->tracker));

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
	struct activator *act = userData;
	if (act != NULL) {
		if (act->tracker != NULL) {
			serviceTracker_destroy(act->tracker);
			act->tracker = NULL;
		}
		free(act);
	}
	return CELIX_SUCCESS;
}

static int test(void *handle) {
	int status = 0;
	struct activator *act = handle;

	double result = 0.0;

    int rc;
    if (act->calc != NULL) {
         rc = act->calc->sqrt(act->calc->calculator, 4, &result);
        printf("calc result is %d\n", result);
    } else {
        printf("calc not ready\n");
    }

	if (rc != 0 || result != 2.0) {
		status = 1;
	}
	return status;
}
