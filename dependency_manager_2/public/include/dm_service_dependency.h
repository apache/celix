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
 * dm_service_dependency.h
 *
 *  \date       8 Oct 2014
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#ifndef DM_SERVICE_DEPENDENCY_H_
#define DM_SERVICE_DEPENDENCY_H_

#include "celix_errno.h"

typedef struct dm_service_dependency *dm_service_dependency_pt;

#include "dm_component.h"

typedef celix_status_t (*service_add_fpt)(void *handle, service_reference_pt reference, void *service);
typedef celix_status_t (*service_change_fpt)(void *handle, service_reference_pt reference, void *service);
typedef celix_status_t (*service_remove_fpt)(void *handle, service_reference_pt reference, void *service);
typedef celix_status_t (*service_swap_fpt)(void *handle, service_reference_pt oldReference, void *oldService, service_reference_pt newReference, void *newService);

celix_status_t serviceDependency_create(dm_service_dependency_pt *dependency_ptr);
celix_status_t serviceDependency_destroy(dm_service_dependency_pt *dependency_ptr);

celix_status_t serviceDependency_setRequired(dm_service_dependency_pt dependency, bool required);
celix_status_t serviceDependency_setService(dm_service_dependency_pt dependency, char *serviceName, char *filter);
celix_status_t serviceDependency_setCallbacks(dm_service_dependency_pt dependency, service_add_fpt add, service_change_fpt change, service_remove_fpt remove, service_swap_fpt swap);
celix_status_t serviceDependency_setAutoConfigure(dm_service_dependency_pt dependency, celix_thread_mutex_t *service_lock, void **field);

#endif /* DM_SERVICE_DEPENDENCY_H_ */
