/*
 * server_impl.h
 *
 *  Created on: Feb 2, 2012
 *      Author: alexander
 */

#ifndef SERVER_IMPL_H_
#define SERVER_IMPL_H_

#include <apr_general.h>

#include <celix_errno.h>

#include "server.h"

struct server {
	int oldReply;
};

celix_status_t server_create(apr_pool_t *pool, server_t *server);
celix_status_t server_doo(server_t server, int a, int b, int *reply);

#endif /* SERVER_IMPL_H_ */
