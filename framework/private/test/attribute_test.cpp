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
 * attribute_test.cpp
 *
 *  \date       Feb 11, 2013
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "CppUTest/TestHarness.h"
#include "CppUTest/TestHarness_c.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTestExt/MockSupport.h"

extern "C" {
#include "attribute_private.h"
#include "celix_log.h"

framework_logger_pt logger = (framework_logger_pt) 0x42;
}

int main(int argc, char** argv) {
	return RUN_ALL_TESTS(argc, argv);
}

static char* my_strdup(const char* s){
	if(s==NULL){
		return NULL;
	}

	size_t len = strlen(s);

	char *d = (char*) calloc (len + 1,sizeof(char));

	if (d == NULL){
		return NULL;
	}

	strncpy (d,s,len);
	return d;
}

TEST_GROUP(attribute) {
	void setup(void) {
	}

	void teardown() {
		mock().checkExpectations();
		mock().clear();
	}
};

TEST(attribute, create) {
	char * key = (char*) my_strdup("key");
	char * value = (char*) my_strdup("value");

	attribute_pt attribute = NULL;
	celix_status_t status = attribute_create(key, value, &attribute);
	STRCMP_EQUAL(key, attribute->key);
	STRCMP_EQUAL(value, attribute->value);
	LONGS_EQUAL(CELIX_SUCCESS, status);

	mock().expectOneCall("framework_logCode")
			.withParameter("code", CELIX_ILLEGAL_ARGUMENT);

	status = attribute_create(NULL, NULL, NULL);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);

	attribute_destroy(attribute);
}

TEST(attribute, getKey) {
	char key[] = "key";
	char value[] = "value";

	attribute_pt attribute = (attribute_pt) malloc(sizeof(*attribute));
	attribute->key = key;
	attribute->value = value;

	char *actual = NULL;
	attribute_getKey(attribute, &actual);
	STRCMP_EQUAL(key, actual);

	free(attribute);
}

TEST(attribute, getValue) {
	char key[] = "key";
	char value[] = "value";

	attribute_pt attribute = (attribute_pt) malloc(sizeof(*attribute));
	attribute->key = key;
	attribute->value = value;

	char *actual = NULL;
	attribute_getValue(attribute, &actual);
	STRCMP_EQUAL(value, actual);

	free(attribute);
}
