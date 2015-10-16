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