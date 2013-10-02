/*
 * server_impl.c
 *
 *  Created on: Feb 2, 2012
 *      Author: alexander
 */

#include "server_impl.h"

celix_status_t server_create(apr_pool_t *pool, server_t *server) {
	celix_status_t status = CELIX_SUCCESS;

	*server = apr_palloc(pool, sizeof(**server));
	if (!*server) {
		status = CELIX_ENOMEM;
	} else {
		(*server)->oldReply = -1;
	}

	return status;
}

celix_status_t server_doo(server_t server, int a, int b, int *reply) {
	*reply = a + b;

	return CELIX_SUCCESS;
}
