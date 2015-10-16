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
 * dm_component_impl.c
 *
 *  \date       9 Oct 2014
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <stdlib.h>

#include "array_list.h"

#include "dm_server.h"
#include "dm_component.h"
#include "dm_component_impl.h"
#include "dm_dependency_manager_impl.h"

struct dm_server {
    struct dm_dependency_manager *manager;
};

celix_status_t dmServiceCreate(dm_server_pt * dmServ, bundle_context_pt context, struct dm_dependency_manager *manager) {
    *dmServ = calloc(sizeof(struct dm_server), 1);
    (*dmServ)->manager = manager;
    return CELIX_SUCCESS;
}

celix_status_t dmServiceDestroy(dm_server_pt dmServ) {
    free(dmServ);
    return CELIX_SUCCESS;
}

celix_status_t dmService_getInfo(dm_server_pt dmServ, dm_info_pt info) {
    int compCnt;
    arrayList_create(&(info->components));
    array_list_pt  compList = dmServ->manager->components;

    for (compCnt = 0; compCnt < arrayList_size(compList); compCnt++) {
        int i;
        struct dm_component *component = arrayList_get(compList, compCnt);

        // Create a component info
        dm_component_info_pt compInfo = calloc(sizeof(*compInfo),1);
        arrayList_create(&(compInfo->interface_list));
        arrayList_create(&(compInfo->dependency_list));

        //Fill in the fields of the component
        char *outstr;
        asprintf(&outstr, "%p",component->implementation);
        compInfo->id = outstr;
        compInfo->active = component->active;

        array_list_pt interfaces = component->dm_interface;
        array_list_pt dependencies = component->dependencies;

        for(i = 0; i < arrayList_size(interfaces); i++) {
            dm_interface * interface = arrayList_get(interfaces, i);
            arrayList_add(compInfo->interface_list, strdup(interface->serviceName));
        }

        for(i = 0; i < arrayList_size(dependencies); i++) {
            dm_service_dependency_pt  dependency = arrayList_get(dependencies, i);

            dependency_info_pt depInfo = calloc(sizeof(*depInfo), 1);
            depInfo->available = dependency->available;
            depInfo->required = dependency->required;
            if(dependency->tracked_filter) {
                depInfo->interface  = strdup(dependency->tracked_filter);
            } else {
                depInfo->interface = strdup(dependency->tracked_service_name);
            }
            arrayList_add(compInfo->dependency_list, depInfo);
        }

        arrayList_add(info->components, compInfo);
    }

    return CELIX_SUCCESS;
}