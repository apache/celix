#include <stdlib.h>
#include <pthread.h>

#include <constants.h>
#include <bundle_context.h>
#include <service_tracker.h>

#include "example.h"
#include "managed_service.h"

struct activator {
	bundle_context_pt context;
	example_pt example;
	managed_service_service_pt managed;
	service_registration_pt managedServiceRegistry;
};

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = calloc(1, sizeof(struct activator));
	if (activator != NULL) {
		(*userData) = activator;
		activator->example = NULL;
		example_create(&activator->example);
		activator->managed = NULL;
		activator->managedServiceRegistry = NULL;

	} else {
		status = CELIX_ENOMEM;
	}
	return status;
}

celix_status_t bundleActivator_start(void *userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;

	example_start(activator->example);
    
    activator->managed = calloc(1, sizeof(*activator->managed));
    properties_pt props = properties_create();

	if (activator->managed != NULL && props != NULL) {
        properties_set(props, "service.pid", "org.example.config.admin"); 
        activator->managed->managedService = (void *)activator->example;
        activator->managed->updated = (void *)example_updated;
		bundleContext_registerService(context, (char *)  MANAGED_SERVICE_SERVICE_NAME, activator->managed, props, &activator->managedServiceRegistry);
	}

	return status;
}

celix_status_t bundleActivator_stop(void *userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;

	if (activator->managed != NULL) {
		serviceRegistration_unregister(activator->managedServiceRegistry);
	}
	example_stop(activator->example);

	return status;
}

celix_status_t bundleActivator_destroy(void *userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;

	example_destroy(activator->example);
	if (activator->managed != NULL) {
		free(activator->managed);
	}

	return status;
}
