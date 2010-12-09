/*
 * test_service.h
 *
 *  Created on: Sep 24, 2010
 *      Author: alexanderb
 */

#ifndef TEST_SERVICE_H_
#define TEST_SERVICE_H_

typedef struct test_service_handle * TEST_SERVICE_HANDLE;

struct test_service {
	TEST_SERVICE_HANDLE handle;
	char * doo(char * message, int value);
};

#endif /* TEST_SERVICE_H_ */
