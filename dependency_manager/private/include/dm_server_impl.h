#include "dm_server.h"
#include "dm_component.h"
#include "dm_component_impl.h"
#include "dm_dependency_manager_impl.h"

celix_status_t dmServiceCreate(dm_server_pt * dmServ, bundle_context_pt context, struct dm_dependency_manager *manager);
celix_status_t dmServiceAddComponent(dm_server_pt dmServ, dm_component_pt component);
celix_status_t dmServiceDestroy(dm_server_pt dmServ);
celix_status_t dmService_getInfo(dm_server_pt dmServ, dm_info_pt info);

