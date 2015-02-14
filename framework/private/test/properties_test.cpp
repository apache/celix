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

	void setup(void) {
	}

	void teardown() {
		mock().checkExpectations();
		mock().clear();
	}
};

TEST(properties, create) {
	properties_pt properties = properties_create();
	CHECK(properties);
}

TEST(properties, load) {
	char propertiesFile[] = "../../celix/framework/private/resources-test/properties.txt";
	properties_pt properties = properties_load(propertiesFile);
	LONGS_EQUAL(2, hashMap_size(properties));

	char keyA[] = "a";
	char *valueA = properties_get(properties, keyA);
	STRCMP_EQUAL("b", valueA);
	char keyB[] = "b";
	STRCMP_EQUAL("c", properties_get(properties, keyB));
}

TEST(properties, store) {
	char propertiesFile[] = "properties_out.txt";
	properties_pt properties = properties_create();
	char keyA[] = "x";
	char keyB[] = "y";
	char valueA[] = "1";
	char valueB[] = "2";
	properties_set(properties, keyA, valueA);
	properties_set(properties, keyB, valueB);
	properties_store(properties, propertiesFile, NULL);
}

TEST(properties, getSet) {
	properties_pt properties = properties_create();
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
}

