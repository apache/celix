/*
 * client_test.c
 *
 *  Created on: Feb 2, 2012
 *      Author: alexander
 */

#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTestExt/MockSupport.h"

extern "C" {
	#include <stdio.h>
	#include <stdlib.h>
	#include <celix_errno.h>

	#include "CppUTestExt/MockSupport_c.h"

	#include "server.h"
	#include "client_impl.h"

	celix_status_t server_doo(server_t server, int a, int b, int *reply);

	celix_status_t server_doo(server_t server, int a, int b, int *reply) {
		mock_c()->actualCall("server_doo")
				->withIntParameters("a", a)
				->withIntParameters("b", b)
				->withPointerParameters("reply", reply);

		return mock_c()->returnValue().value.intValue;
	}
}

MockFunctionCall& ServerDoExpectAndReturn(int a, int b, int *reply, celix_status_t ret) {
	return mock()
			.expectOneCall("server_doo")
			.withParameter("a", a)
			.withParameter("b", b)
			.withParameter("reply", reply)
			.andReturnValue(ret);
}


int main(int argc, char** argv) {
	return RUN_ALL_TESTS(argc, argv);
}

//START: testGroup
TEST_GROUP(client) {

	server_service_t server;
	client_t client;

    void setup() {
    	server = (server_service_t) malloc(sizeof(*server));
    	server->server = NULL;
    	server->server_doo = server_doo;

    	apr_pool_t *pool;
    	apr_initialize();
    	apr_pool_create(&pool, NULL);
    	client_create(pool, NULL, &client);
    }

    void teardown()
    {
    	mock().checkExpectations();
    	mock().clear();
    	free(server);
    	server = NULL;
    	apr_terminate();
    }
};
//END: testGroup

TEST(client, doo) {
	int i = 3;
	ServerDoExpectAndReturn(1, 2, &i, CELIX_SUCCESS);

	client_addedService(client, NULL, server);
	client_doo(client, 1, 2, &i);

	CHECK_EQUAL_C_INT(3, i);
}
