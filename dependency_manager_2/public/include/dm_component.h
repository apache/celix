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
 * dm_component.h
 *
 *  \date       8 Oct 2014
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#ifndef COMPONENT_H_
#define COMPONENT_H_

#include <bundle_context.h>
#include <celix_errno.h>

typedef struct dm_component *dm_component_pt;

#include "dm_dependency_manager.h"
#include "dm_service_dependency.h"

typedef celix_status_t (*init_fpt)(void *userData);
typedef celix_status_t (*start_fpt)(void *userData);
typedef celix_status_t (*stop_fpt)(void *userData);
typedef celix_status_t (*destroy_fpt)(void *userData);

celix_status_t component_create(bundle_context_pt context, dm_dependency_manager_pt manager, dm_component_pt *component);
celix_status_t component_destroy(dm_component_pt *component);

celix_status_t component_setInterface(dm_component_pt component, char *serviceName, void *service, properties_pt properties);
celix_status_t component_setImplementation(dm_component_pt component, void *implementation);

celix_status_t component_addServiceDependency(dm_component_pt component, ...);
celix_status_t component_removeServiceDependency(dm_component_pt component, dm_service_dependency_pt dependency);

celix_status_t component_setCallbacks(dm_component_pt component, init_fpt init, start_fpt start, stop_fpt, destroy_fpt);

#endif /* COMPONENT_H_ */
