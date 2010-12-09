/*
 * dependency_manager.c
 *
 *  Created on: May 12, 2010
 *      Author: dk489
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dependency_manager.h"
#include "dependency_activator_base.h"
#include "bundle_context.h"
#include "service_component_private.h"

DEPENDENCY_MANAGER dependencyManager_create(BUNDLE_CONTEXT context) {
	DEPENDENCY_MANAGER manager = (DEPENDENCY_MANAGER) malloc(sizeof(*manager));
	manager->context = context;
	manager->services = arrayList_create();
	return manager;
}

void dependencyManager_add(DEPENDENCY_MANAGER manager, SERVICE service) {
	arrayList_add(manager->services, service);
	serviceComponent_start(service);
}

void dependencyManager_remove(DEPENDENCY_MANAGER manager, SERVICE service) {
	serviceComponent_stop(service);
	arrayList_removeElement(manager->services, service);
}

SERVICE dependencyManager_createService(DEPENDENCY_MANAGER manager) {
	return serviceComponent_create(manager->context, manager);
}

SERVICE_DEPENDENCY dependencyManager_createServiceDependency(DEPENDENCY_MANAGER manager) {
	return serviceDependency_create(manager->context);
}
