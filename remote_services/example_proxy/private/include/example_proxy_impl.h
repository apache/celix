/*
 * example_proxy_impl.h
 *
 *  Created on: Oct 13, 2011
 *      Author: alexander
 */

#ifndef EXAMPLE_PROXY_IMPL_H_
#define EXAMPLE_PROXY_IMPL_H_

#include <apr_general.h>

#include "celix_errno.h"

#include "example_service.h"
#include "remote_proxy.h"

#include "endpoint_listener.h"

struct example {
	apr_pool_t *pool;
	endpoint_description_t endpoint;
};

celix_status_t exampleProxy_create(apr_pool_t *pool, example_t *example);
celix_status_t exampleProxy_add(example_t example, double a, double b, double *result);
celix_status_t exampleProxy_sub(example_t example, double a, double b, double *result);
celix_status_t exampleProxy_sqrt(example_t example, double a, double *result);

celix_status_t exampleProxy_setEndpointDescription(void *proxy, endpoint_description_t endpoint);

#endif /* EXAMPLE_PROXY_IMPL_H_ */
