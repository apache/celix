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
 * celix_errorcodes_test.cpp
 *
 *  \date       Oct 16, 2015
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
#include "celix_errno.h"
}

int main(int argc, char** argv) {
	return CommandLineTestRunner::RunAllTests(argc, argv);
}

TEST_GROUP(celix_errorcodes) {
	void setup(void){
	}

	void teardown(void){
	}
};

TEST(celix_errorcodes, test_entire_file){
	char ret_str[100];

	celix_strerror(CELIX_BUNDLE_EXCEPTION, ret_str, 100);
	STRCMP_EQUAL("Bundle exception", ret_str);

	celix_strerror(CELIX_INVALID_BUNDLE_CONTEXT, ret_str, 100);
	STRCMP_EQUAL("Invalid bundle context", ret_str);

	celix_strerror(CELIX_ILLEGAL_ARGUMENT, ret_str, 100);
	STRCMP_EQUAL("Illegal argument", ret_str);

	celix_strerror(CELIX_INVALID_SYNTAX, ret_str, 100);
	STRCMP_EQUAL("Invalid syntax", ret_str);

	celix_strerror(CELIX_FRAMEWORK_SHUTDOWN, ret_str, 100);
	STRCMP_EQUAL("Framework shutdown", ret_str);

	celix_strerror(CELIX_ILLEGAL_STATE, ret_str, 100);
	STRCMP_EQUAL("Illegal state", ret_str);

	celix_strerror(CELIX_FRAMEWORK_EXCEPTION, ret_str, 100);
	STRCMP_EQUAL("Framework exception", ret_str);

	celix_strerror(CELIX_FILE_IO_EXCEPTION, ret_str, 100);
	STRCMP_EQUAL("File I/O exception", ret_str);

	celix_strerror(CELIX_SERVICE_EXCEPTION, ret_str, 100);
	STRCMP_EQUAL("Service exception", ret_str);

	celix_strerror(CELIX_START_ERROR, ret_str, 100);
	STRCMP_EQUAL("Unknown code", ret_str);

	//test non celix error code
	STRCMP_EQUAL("Cannot allocate memory",
				celix_strerror(ENOMEM, ret_str, 100));
}
