/*
 * example_impl.c
 *
 *  Created on: Oct 5, 2011
 *      Author: alexander
 */
#include <math.h>

#include <stdio.h>

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
	printf("CALCULATOR: Add: %f + %f = %f\n", a, b, *result);

	return status;
}

celix_status_t example_sub(example_t example, double a, double b, double *result) {
	celix_status_t status = CELIX_SUCCESS;

	*result = a - b;
	printf("CALCULATOR: Sub: %f + %f = %f\n", a, b, *result);

	return status;
}

celix_status_t example_sqrt(example_t example, double a, double *result) {
	celix_status_t status = CELIX_SUCCESS;

	if (a > 0) {
		*result = sqrt(a);
		printf("CALCULATOR: Sqrt: %f = %f\n", a, *result);
	} else {
		printf("CALCULATOR: Sqrt: %f = ERR\n", a);
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	return status;
}
