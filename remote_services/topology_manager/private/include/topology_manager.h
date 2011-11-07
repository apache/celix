/*
 * topology_manager.h
 *
 *  Created on: Sep 29, 2011
 *      Author: alexander
 */

#ifndef TOPOLOGY_MANAGER_H_
#define TOPOLOGY_MANAGER_H_

#include "endpoint_listener.h"

typedef struct topology_manager *topology_manager_t;

celix_status_t topologyManager_create(BUNDLE_CONTEXT context, apr_pool_t *pool, topology_manager_t *manager);

celix_status_t topologyManager_rsaAdding(void *handle, SERVICE_REFERENCE reference, void **service);
celix_status_t topologyManager_rsaAdded(void *handle, SERVICE_REFERENCE reference, void *service);
celix_status_t topologyManager_rsaModified(void *handle, SERVICE_REFERENCE reference, void *service);
celix_status_t topologyManager_rsaRemoved(void *handle, SERVICE_REFERENCE reference, void *service);

celix_status_t topologyManager_serviceChanged(void *listener, SERVICE_EVENT event);

celix_status_t topologyManager_endpointAdded(void *handle, endpoint_description_t endpoint, char *machtedFilter);
celix_status_t topologyManager_endpointRemoved(void *handle, endpoint_description_t endpoint, char *machtedFilter);

celix_status_t topologyManager_importService(topology_manager_t manager, endpoint_description_t endpoint);
celix_status_t topologyManager_exportService(topology_manager_t manager, SERVICE_REFERENCE reference);
celix_status_t topologyManager_removeService(topology_manager_t manager, SERVICE_REFERENCE reference);

celix_status_t topologyManager_listenerAdded(void *handle, ARRAY_LIST listeners);
celix_status_t topologyManager_listenerRemoved(void *handle, ARRAY_LIST listeners);

#endif /* TOPOLOGY_MANAGER_H_ */
