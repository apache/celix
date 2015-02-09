#include <stdlib.h>

#include "bundle_activator.h"
#include "dm_activator_base.h"


struct dm_dependency_activator_base {
	dm_dependency_manager_pt manager;
	bundle_context_pt context;
	void* userData;
};

typedef struct dm_dependency_activator_base * dependency_activator_base_pt;

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	celix_status_t status = CELIX_ENOMEM;

	dependency_activator_base_pt dependency_activator = calloc(1, sizeof(struct dm_dependency_activator_base));

	if (dependency_activator) {
		dependency_activator->context = context;
		dm_create(context, &dependency_activator->userData);

		(*userData) = dependency_activator;

		status = CELIX_SUCCESS;
	}

	return status;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
	celix_status_t status;
	dependency_activator_base_pt dependency_activator = (dependency_activator_base_pt) userData;

	status = dependencyManager_create(dependency_activator->context, &dependency_activator->manager);

	if (status == CELIX_SUCCESS) {
		dm_init(dependency_activator->userData, context, dependency_activator->manager);
	}

	return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt __unused context) {
	celix_status_t status = CELIX_SUCCESS;
	dependency_activator_base_pt dependency_activator = (dependency_activator_base_pt) userData;

	dm_destroy(dependency_activator->userData, dependency_activator->context, dependency_activator->manager);

	dependencyManager_destroy(&dependency_activator->manager);

	dependency_activator->userData = NULL;
	dependency_activator->manager = NULL;

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt __unused context) {
	celix_status_t status = CELIX_SUCCESS;
	dependency_activator_base_pt dependency_activator = (dependency_activator_base_pt) userData;

	free(dependency_activator);

	return status;
}


