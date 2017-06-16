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
 * properties_test.cpp
 *
 *  \date       Feb 11, 2013
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
#include "properties.h"
}

int main(int argc, char** argv) {
	return RUN_ALL_TESTS(argc, argv);
}

TEST_GROUP(properties) {
	properties_pt properties;

	void setup(void) {
	}

	void teardown() {
		mock().checkExpectations();
		mock().clear();
	}
};

TEST(properties, create) {
	properties = properties_create();
	CHECK(properties);

	properties_destroy(properties);
}

TEST(properties, load) {
	char propertiesFile[] = "resources-test/properties.txt";
	properties = properties_load(propertiesFile);
	LONGS_EQUAL(4, hashMap_size(properties));

	const char keyA[] = "a";
	const char *valueA = properties_get(properties, keyA);
	STRCMP_EQUAL("b", valueA);

	const char keyNiceA[] = "nice_a";
	const char *valueNiceA = properties_get(properties, keyNiceA);
	STRCMP_EQUAL("nice_b", valueNiceA);

	const char keyB[] = "b";
	const char *valueB = properties_get(properties, keyB);
	STRCMP_EQUAL("c \t d", valueB);

	properties_destroy(properties);
}

TEST(properties, store) {
	char propertiesFile[] = "resources-test/properties_out.txt";
	properties = properties_create();
	char keyA[] = "x";
	char keyB[] = "y";
	char valueA[] = "1";
	char valueB[] = "2";
	properties_set(properties, keyA, valueA);
	properties_set(properties, keyB, valueB);
	properties_store(properties, propertiesFile, NULL);

	properties_destroy(properties);
}

TEST(properties, copy) {
	properties_pt copy;
	char propertiesFile[] = "resources-test/properties.txt";
	properties = properties_load(propertiesFile);
	LONGS_EQUAL(4, hashMap_size(properties));

	properties_copy(properties, &copy);

	char keyA[] = "a";
	const char *valueA = properties_get(copy, keyA);
	STRCMP_EQUAL("b", valueA);
	const char keyB[] = "b";
	STRCMP_EQUAL("c \t d", properties_get(copy, keyB));

	properties_destroy(properties);
	properties_destroy(copy);
}

TEST(properties, getSet) {
	properties = properties_create();
	char keyA[] = "x";
	char keyB[] = "y";
	char keyC[] = "z";
	char valueA[] = "1";
	char valueB[] = "2";
	char valueC[] = "3";
	properties_set(properties, keyA, valueA);
	properties_set(properties, keyB, valueB);

	STRCMP_EQUAL(valueA, properties_get(properties, keyA));
	STRCMP_EQUAL(valueB, properties_get(properties, keyB));
	STRCMP_EQUAL(valueC, properties_getWithDefault(properties, keyC, valueC));

	properties_destroy(properties);
}

