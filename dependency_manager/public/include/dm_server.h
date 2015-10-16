
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
