/*
 * dependency_activator_base.c
 *
 *  Created on: Aug 8, 2010
 *      Author: alexanderb
 */
#include <stdlib.h>

#include "bundle_activator.h"
#include "dependency_manager.h"
#include "service_component_private.h"
#include "dependency_activator_base.h"

DEPENDENCY_MANAGER m_manager;
BUNDLE_CONTEXT m_context;

struct dependencyActivatorBase {
	DEPENDENCY_MANAGER manager;
	BUNDLE_CONTEXT context;
	void * userData;
};

typedef struct dependencyActivatorBase * DEPENDENCY_ACTIVATOR_BASE;

void * bundleActivator_create() {
	DEPENDENCY_ACTIVATOR_BASE data = (DEPENDENCY_ACTIVATOR_BASE) malloc(sizeof(*data));
	void * userData = dm_create();
	data->userData = userData;

	return data;
}

void bundleActivator_start(void * userData, BUNDLE_CONTEXT context) {
	DEPENDENCY_ACTIVATOR_BASE data = (DEPENDENCY_ACTIVATOR_BASE) userData;
	data->manager = dependencyManager_create(context);
	data->context = context;
	dm_init(data->userData, data->context, data->manager);
}

void bundleActivator_stop(void * userData, BUNDLE_CONTEXT context) {
	DEPENDENCY_ACTIVATOR_BASE data = (DEPENDENCY_ACTIVATOR_BASE) userData;
	dm_destroy(data->userData, data->context, data->manager);
	m_manager = NULL;
	m_context = NULL;
}

void bundleActivator_destroy(void * userData) {

}

SERVICE dependencyActivatorBase_createService(DEPENDENCY_MANAGER manager) {
	return serviceComponent_create(manager->context, manager);
}

SERVICE_DEPENDENCY dependencyActivatorBase_createServiceDependency(DEPENDENCY_MANAGER manager) {
	return serviceDependency_create(manager->context);
}

