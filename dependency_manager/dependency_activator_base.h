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

void * dm_create(BUNDLE_CONTEXT context);
void dm_init(void * userData, BUNDLE_CONTEXT context, DEPENDENCY_MANAGER manager);
void dm_destroy(void * userData, BUNDLE_CONTEXT context, DEPENDENCY_MANAGER manager);

SERVICE dependencyActivatorBase_createService(void);
SERVICE_DEPENDENCY dependencyActivatorBase_createServiceDependency(DEPENDENCY_MANAGER manager);

#endif /* DEPENDENCY_ACTIVATOR_BASE_H_ */
