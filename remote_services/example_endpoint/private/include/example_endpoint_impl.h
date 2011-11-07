/*
 * example_endpoint.h
 *
 *  Created on: Oct 7, 2011
 *      Author: alexander
 */

#ifndef EXAMPLE_ENDPOINT_IMPL_H_
#define EXAMPLE_ENDPOINT_IMPL_H_

#include <apr_general.h>

#include "celix_errno.h"
#include "remote_endpoint_impl.h"

celix_status_t exampleEndpoint_create(apr_pool_t *pool, remote_endpoint_t *endpoint);

celix_status_t exampleEndpoint_setService(remote_endpoint_t endpoint, void *service);

celix_status_t exampleEndpoint_handleRequest(remote_endpoint_t ep, char *request, char *data, char **reply);

celix_status_t exampleEndpoint_add(remote_endpoint_t ep, char *data, char **reply);
celix_status_t exampleEndpoint_sub(remote_endpoint_t ep, char *data, char **reply);
celix_status_t exampleEndpoint_sqrt(remote_endpoint_t ep, char *data, char **reply);

#endif /* EXAMPLE_ENDPOINT_IMPL_H_ */
