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
 * dm_dependency_manager.h
 *
 *  \date       22 Feb 2014
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#ifndef DM_DEPENDENCY_MANAGER_H_
#define DM_DEPENDENCY_MANAGER_H_

#include "bundle_context.h"
#include "celix_errno.h"
#include "array_list.h"
#include "dm_info.h"
#include "dm_component.h"

typedef struct dm_dependency_manager *dm_dependency_manager_pt;

celix_status_t dependencyManager_create(bundle_context_pt context, dm_dependency_manager_pt *manager);
void dependencyManager_destroy(dm_dependency_manager_pt manager);

celix_status_t dependencyManager_add(dm_dependency_manager_pt manager, dm_component_pt component);

celix_status_t depedencyManager_removeAllComponents(dm_dependency_manager_pt manager);

/**
 * returns a dm_ of dm_dependency_manager_info. Caller has ownership.
 */
celix_status_t dependencyManager_getInfo(dm_dependency_manager_pt manager, dm_dependency_manager_info_pt *info);
void dependencyManager_destroyInfo(dm_dependency_manager_pt manager, dm_dependency_manager_info_pt info);

#endif /* DM_DEPENDENCY_MANAGER_H_ */
