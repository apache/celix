/*
 * example_impl.h
 *
 *  Created on: Oct 5, 2011
 *      Author: alexander
 */

#ifndef EXAMPLE_IMPL_H_
#define EXAMPLE_IMPL_H_

#include <apr_general.h>

#include "celix_errno.h"

#include "example_service.h"

struct example {
	apr_pool_t *pool;
};

celix_status_t example_create(apr_pool_t *pool, example_t *example);
celix_status_t example_add(example_t example, double a, double b, double *result);
celix_status_t example_sub(example_t example, double a, double b, double *result);
celix_status_t example_sqrt(example_t example, double a, double *result);

#endif /* EXAMPLE_IMPL_H_ */
