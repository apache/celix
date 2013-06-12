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
 *  \date       May 12, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef DEPENDENCY_MANAGER_H_
#define DEPENDENCY_MANAGER_H_

#include "service_component.h"
#include "bundle_context.h"

struct dependencyManager {
	bundle_context_pt context;
	array_list_pt services;
};

typedef struct dependencyManager * dependency_manager_pt;

dependency_manager_pt dependencyManager_create(bundle_context_pt context);
void dependencyManager_add(dependency_manager_pt manager, service_pt service);
void dependencyManager_remove(dependency_manager_pt manager, service_pt service);

void dm_startService(service_pt service);
void dm_stopService(service_pt service);

#endif /* DEPENDENCY_MANAGER_H_ */
