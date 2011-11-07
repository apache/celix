/*
 * example_impl.c
 *
 *  Created on: Oct 5, 2011
 *      Author: alexander
 */
#include <math.h>

#include "headers.h"
#include "example_impl.h"

celix_status_t example_create(apr_pool_t *pool, example_t *example) {
	celix_status_t status = CELIX_SUCCESS;

	*example = apr_palloc(pool, sizeof(**example));
	if (!*example) {
		status = CELIX_ENOMEM;
	} else {
		(*example)->pool = pool;
	}

	return status;
}

celix_status_t example_add(example_t example, double a, double b, double *result) {
	celix_status_t status = CELIX_SUCCESS;

	*result = a + b;

	return status;
}

celix_status_t example_sub(example_t example, double a, double b, double *result) {
	celix_status_t status = CELIX_SUCCESS;

	*result = a - b;

	return status;
}

celix_status_t example_sqrt(example_t example, double a, double *result) {
	celix_status_t status = CELIX_SUCCESS;

	if (a > 0) {
		*result = sqrt(a);
	} else {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	return status;
}
