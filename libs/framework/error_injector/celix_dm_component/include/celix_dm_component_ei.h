/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef CELIX_CELIX_DM_COMPONENT_EI_H
#define CELIX_CELIX_DM_COMPONENT_EI_H
#ifdef __cplusplus
extern "C" {
#endif
#include "celix_error_injector.h"
#include "celix_dm_component.h"
#include "celix_dm_service_dependency.h"

CELIX_EI_DECLARE(celix_dmComponent_create, celix_dm_component_t*);
CELIX_EI_DECLARE(celix_dmServiceDependency_create, celix_dm_service_dependency_t*);
CELIX_EI_DECLARE(celix_dmComponent_addServiceDependency, celix_status_t);
CELIX_EI_DECLARE(celix_dmServiceDependency_setService, celix_status_t);
CELIX_EI_DECLARE(celix_dmComponent_addInterface, celix_status_t);
CELIX_EI_DECLARE(celix_dependencyManager_addAsync, celix_status_t);

#ifdef __cplusplus
}
#endif

#endif //CELIX_CELIX_DM_COMPONENT_EI_H
