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

#ifndef CELIX_DM_INFO_H_
#define CELIX_DM_INFO_H_



#include <stdbool.h>
#include "celix_array_list.h"
#include "celix_properties.h"

#ifdef __cplusplus
extern "C" {
#endif


struct celix_dm_interface_info_struct {
    char* name;
    celix_properties_t *properties;
};
typedef struct celix_dm_interface_info_struct *dm_interface_info_pt;  //deprecated
typedef struct celix_dm_interface_info_struct dm_interface_info_t;  //deprecated
typedef struct celix_dm_interface_info_struct celix_dm_interface_info_t;

struct celix_dm_service_dependency_info_struct {
    char *serviceName;
    char *filter;
    char *versionRange;
    bool available;
    bool required;
    size_t count;
};
typedef struct celix_dm_service_dependency_info_struct *dm_service_dependency_info_pt;  //deprecated
typedef struct celix_dm_service_dependency_info_struct dm_service_dependency_info_t;  //deprecated
typedef struct celix_dm_service_dependency_info_struct celix_dm_service_dependency_info_t;

struct celix_dm_component_info_struct {
    long bundleId;
    char* bundleSymbolicName;
    char* id;
    char* name;
    bool active;
    char* state;
    size_t nrOfTimesStarted;
    size_t nrOfTimesResumed;
    celix_array_list_t *interfaces;   // type dm_interface_info_t*
    celix_array_list_t *dependency_list;  // type dm_service_dependency_info_t*
};
typedef struct celix_dm_component_info_struct *dm_component_info_pt; //deprecated
typedef struct celix_dm_component_info_struct dm_component_info_t; //deprecated
typedef struct celix_dm_component_info_struct celix_dm_component_info_t;

struct celix_dm_dependency_manager_info_struct {
    long bndId;
    char* bndSymbolicName;
    celix_array_list_t *components;      // type dm_component_info_t*
};
typedef struct celix_dm_dependency_manager_info_struct *dm_dependency_manager_info_pt; //deprecated
typedef struct celix_dm_dependency_manager_info_struct dm_dependency_manager_info_t; //deprecated
typedef struct celix_dm_dependency_manager_info_struct celix_dependency_manager_info_t;


#ifdef __cplusplus
}
#endif

#endif //CELIX_DM_INFO_H_
