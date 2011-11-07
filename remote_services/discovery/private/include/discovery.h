/*
 * topology_manager.h
 *
 *  Created on: Sep 29, 2011
 *      Author: alexander
 */

#ifndef DISCOVERY_H_
#define DISCOVERY_H_

#include "endpoint_listener.h"

typedef struct discovery *discovery_t;

celix_status_t discovery_create(apr_pool_t *pool, BUNDLE_CONTEXT context, discovery_t *discovery);
celix_status_t discovery_stop(discovery_t discovery);

celix_status_t discovery_endpointAdded(void *handle, endpoint_description_t endpoint, char *machtedFilter);
celix_status_t discovery_endpointRemoved(void *handle, endpoint_description_t endpoint, char *machtedFilter);

celix_status_t discovery_endpointListenerAdding(void * handle, SERVICE_REFERENCE reference, void **service);
celix_status_t discovery_endpointListenerAdded(void * handle, SERVICE_REFERENCE reference, void * service);
celix_status_t discovery_endpointListenerModified(void * handle, SERVICE_REFERENCE reference, void * service);
celix_status_t discovery_endpointListenerRemoved(void * handle, SERVICE_REFERENCE reference, void * service);

celix_status_t discovery_updateEndpointListener(discovery_t discovery, SERVICE_REFERENCE reference, endpoint_listener_t service);


#endif /* DISCOVERY_H_ */
