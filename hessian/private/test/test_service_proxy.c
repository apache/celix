/*
 * test_encoder.c
 *
 *  Created on: Aug 5, 2011
 *      Author: alexander
 */

#include "hessian_2.0_out.h"

void testServiceProxy_sayHello(char *message) {
	hessian_out_t out = malloc(sizeof(*out));

	hessian_writeString(out, message);

	// tcp send out->buffer;
}
