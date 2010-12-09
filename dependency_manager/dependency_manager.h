/*
 * dependency_manager.h
 *
 *  Created on: May 12, 2010
 *      Author: dk489
 */

#ifndef DEPENDENCY_MANAGER_H_
#define DEPENDENCY_MANAGER_H_

#include "headers.h"
#include "service_component.h"

struct dependencyManager {
	BUNDLE_CONTEXT context;
	ARRAY_LIST services;
};

typedef struct dependencyManager * DEPENDENCY_MANAGER;

DEPENDENCY_MANAGER dependencyManager_create(BUNDLE_CONTEXT context);
void dependencyManager_add(DEPENDENCY_MANAGER manager, SERVICE service);
void dependencyManager_remove(DEPENDENCY_MANAGER manager, SERVICE service);

void dm_startService(SERVICE service);
void dm_stopService(SERVICE service);

#endif /* DEPENDENCY_MANAGER_H_ */
