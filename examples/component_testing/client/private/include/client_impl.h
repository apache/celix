/*
 * client_impl.h
 *
 *  Created on: Feb 2, 2012
 *      Author: alexander
 */

#ifndef CLIENT_IMPL_H_
#define CLIENT_IMPL_H_

#include <celix_errno.h>
#include <apr_general.h>
#include "bundle_context.h"

#include "server.h"

typedef struct client *client_t;

struct client {
	apr_pool_t *pool;
	server_service_t server;
	BUNDLE_CONTEXT context;
};

celix_status_t client_create(apr_pool_t *pool, BUNDLE_CONTEXT context, client_t *client);
celix_status_t client_doo(client_t client, int a, int b, int *reply);

celix_status_t client_addingService(void *clientP, SERVICE_REFERENCE reference, void **service);
celix_status_t client_addedService(void *clientP, SERVICE_REFERENCE reference, void *service);
celix_status_t client_modifiedService(void *clientP, SERVICE_REFERENCE reference, void *service);
celix_status_t client_removedService(void *clientP, SERVICE_REFERENCE reference, void *service);

#endif /* CLIENT_IMPL_H_ */
