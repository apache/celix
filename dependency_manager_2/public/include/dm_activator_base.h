#ifndef DM_ACTIVATOR_BASE_H_
#define DM_ACTIVATOR_BASE_H_

#include "bundle_context.h"
#include "celix_errno.h"
#include "dm_dependency_manager.h"

celix_status_t dm_create(bundle_context_pt context, void ** userData);
celix_status_t dm_init(void * userData, bundle_context_pt context, dm_dependency_manager_pt manager);
celix_status_t dm_destroy(void * userData, bundle_context_pt context, dm_dependency_manager_pt manager);

#endif /* DM_ACTIVATOR_BASE_H_ */
