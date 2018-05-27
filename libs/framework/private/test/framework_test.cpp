/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */
/*
 * framework_test.cpp
 *
 *  \date       Sep 28, 2015
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdio.h>

#include "CppUTest/TestHarness.h"
#include "CppUTest/TestHarness_c.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTestExt/MockSupport.h"

extern "C" {
#include "framework.h"
#include "framework_private.h"
}

int main(int argc, char** argv) {
	return CommandLineTestRunner::RUN_ALL_TESTS(argc, argv);
}

TEST_GROUP(framework) {
	properties_pt properties;

	void setup(void) {
		properties = properties_create();
	}

	void teardown() {
		properties_destroy(properties);
	}
};

TEST(framework, create){
	//framework_pt framework = NULL;

	//mock().expectOneCall("bundle_create").ignoreOtherParameters();
	//mock().ignoreOtherCalls();

	//framework_create(&framework, properties);


	//CHECK(framework != NULL);

	mock().checkExpectations();
	mock().clear();
}

/*TEST(framework, startFw){
	framework_pt framework = NULL;

	mock().expectOneCall("bundle_create");
	mock().expectOneCall("framework_logCode")
			.withParameter("code", CELIX_ILLEGAL_ARGUMENT);

	LONGS_EQUAL(CELIX_SUCCESS,framework_create(&framework, properties));

	LONGS_EQUAL(CELIX_SUCCESS,fw_init(framework));

	LONGS_EQUAL(CELIX_SUCCESS,framework_start(framework));

	framework_stop(framework);
	framework_destroy(framework);

}

TEST(framework, installBundle){
	framework_pt framework = NULL;

	mock().expectOneCall("bundle_create");
	mock().expectOneCall("framework_logCode")
			.withParameter("code", CELIX_ILLEGAL_ARGUMENT);

	LONGS_EQUAL(CELIX_SUCCESS, framework_create(&framework, properties));

	LONGS_EQUAL(CELIX_SUCCESS,fw_init(framework));

	LONGS_EQUAL(CELIX_SUCCESS,framework_start(framework));

	// fw_installBundle(); // Needs a fake bundle..

	framework_stop(framework);
	framework_destroy(framework);

}*/





