/*
 * dependency_activator_base.h
 *
 *  Created on: May 12, 2010
 *      Author: dk489
 */

#ifndef DEPENDENCY_ACTIVATOR_BASE_H_
#define DEPENDENCY_ACTIVATOR_BASE_H_

#include "headers.h"
#include "dependency_manager.h"
#include "service_dependency.h"

void * dm_create();
void dm_init(void * userData, BUNDLE_CONTEXT context, DEPENDENCY_MANAGER manager);
void dm_destroy(void * userData, BUNDLE_CONTEXT context, DEPENDENCY_MANAGER manager);

SERVICE dependencyActivatorBase_createService();
SERVICE_DEPENDENCY dependencyActivatorBase_createServiceDependency(DEPENDENCY_MANAGER manager);

#endif /* DEPENDENCY_ACTIVATOR_BASE_H_ */
