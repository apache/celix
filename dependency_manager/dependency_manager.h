/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */
/*
 * dependency_manager.h
 *
 *  Created on: May 12, 2010
 *      Author: dk489
 */

#ifndef DEPENDENCY_MANAGER_H_
#define DEPENDENCY_MANAGER_H_

#include "service_component.h"
#include "bundle_context.h"

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
