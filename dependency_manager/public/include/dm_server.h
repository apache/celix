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
 * dm_server.h
 *
 *  \date       15 Oct 2015
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#ifndef CELIX_DM_SERVICE_H
#define CELIX_DM_SERVICE_H

#include "bundle_context.h"

#define DM_SERVICE_NAME "dm_server"

typedef struct dm_server * dm_server_pt;

typedef struct dependency_info {
    char *interface;
    bool available;
    bool required;
} * dependency_info_pt;

typedef struct dm_component_info {
    char *id;
    bool active;
    array_list_pt interface_list;       // type char*
    array_list_pt dependency_list;  // type interface_info_pt
} * dm_component_info_pt;

typedef struct dm_info {
    array_list_pt  components;      // type dm_component_info
} * dm_info_pt;

struct dm_service {
    dm_server_pt server;
    celix_status_t (*getInfo)(dm_server_pt server, dm_info_pt info);
};

typedef struct dm_service * dm_service_pt;


#endif //CELIX_DM_SERVICE_H
