/*
 * test_sercice_skeleton.c
 *
 *  Created on: Aug 5, 2011
 *      Author: alexander
 */

#include "hessian_2.0_in.h"

void testServiceSkeleton_sayHello(hessian_in_t in);

void testServiceSkeleton_handleData(hessian_in_t in) {
	char *method = NULL;
	hessian_readCall(in, &method);

	switch (method) {
		case "sayHello":
			testServiceSkeleton_sayHello(in);
			break;
		default:
			break;
	}
}

void testServiceSkeleton_sayHello(hessian_in_t in) {
	char *message = NULL;
	int read;
	hessian_readString(in, &message, &read);

	testService_sayHello(NULL, message);
}
