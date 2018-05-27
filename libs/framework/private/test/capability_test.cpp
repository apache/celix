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
 * capability_test.cpp
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
#include "capability_private.h"
#include "utils.h"
#include "attribute.h"
#include "celix_log.h"

framework_logger_pt logger = (framework_logger_pt) 0x42;
}

int main(int argc, char** argv) {
	return RUN_ALL_TESTS(argc, argv);
}

TEST_GROUP(capability) {
	void setup(void) {
	}

	void teardown() {
		mock().checkExpectations();
		mock().clear();
	}
};

TEST(capability, create) {
	capability_pt capability;
	module_pt module = (module_pt) 0x10;
	hash_map_pt directives;
	hash_map_pt attributes;

	attribute_pt serviceAttribute = (attribute_pt) 0x01;
    attribute_pt versionAttribute = (attribute_pt) 0x02;

	version_pt emptyVersion = (version_pt) 0x10;
	version_pt fromstrVersion = (version_pt) 0x11;

	char *value1 = (char *) "target";
	char *value2 = (char *) "1.0.0";

	//create with empty version
	directives =  hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
	attributes = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
    hashMap_put(attributes, (void*) "service", serviceAttribute);


	mock().expectOneCall("attribute_getValue")
        .withParameter("attribute", serviceAttribute)
        .withOutputParameterReturning("value", &value1, sizeof(value1))
        .andReturnValue(CELIX_SUCCESS);
	mock().expectOneCall("version_createEmptyVersion")
        .withOutputParameterReturning("version", &emptyVersion, sizeof(emptyVersion))
        .andReturnValue(CELIX_SUCCESS);

	capability = NULL;
	capability_create(module, directives, attributes, &capability);

	mock().expectOneCall("attribute_destroy")
			.withParameter("attribute", serviceAttribute);
	mock().expectOneCall("version_destroy")
			.withParameter("version", emptyVersion);

	capability_destroy(capability);

	//create with version from version string
	directives =  hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
	attributes = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
    hashMap_put(attributes, (void*) "service", serviceAttribute);
    hashMap_put(attributes, (void*) "version", versionAttribute);

	mock().expectOneCall("attribute_getValue")
			.withParameter("attribute", serviceAttribute)
        	.withOutputParameterReturning("value", &value1, sizeof(value1))
        	.andReturnValue(CELIX_SUCCESS);
	mock().expectOneCall("attribute_getValue")
    		.withParameter("attribute", versionAttribute)
    		.withOutputParameterReturning("value", &value2, sizeof(value2))
    		.andReturnValue(CELIX_SUCCESS);
	mock().expectOneCall("version_createVersionFromString")
			.withParameter("versionStr", (char *) "1.0.0")
			.withOutputParameterReturning("version", &fromstrVersion, sizeof(fromstrVersion))
			.andReturnValue(CELIX_SUCCESS);

	capability = NULL;
	capability_create(module, directives, attributes, &capability);

	mock().expectOneCall("attribute_destroy")
			.withParameter("attribute", serviceAttribute);
	mock().expectOneCall("attribute_destroy")
			.withParameter("attribute", versionAttribute);
	mock().expectOneCall("version_destroy")
			.withParameter("version", fromstrVersion);

	capability_destroy(capability);
}

TEST(capability, getServiceName) {
	capability_pt capability = (capability_pt) malloc(sizeof(*capability));
	char serviceName[] = "service";
	capability->serviceName = serviceName;

	const char *actual = NULL;
	capability_getServiceName(capability, &actual);
	STRCMP_EQUAL(serviceName, actual);

	free(capability);
}

TEST(capability, getVersion) {
	capability_pt capability = (capability_pt) malloc(sizeof(*capability));
	version_pt version = (version_pt) 0x10;
	capability->version = version;

	version_pt actual = NULL;
	capability_getVersion(capability, &actual);
	POINTERS_EQUAL(version, actual);

	free(capability);
}

TEST(capability, getModule) {
	capability_pt capability = (capability_pt) malloc(sizeof(*capability));
	module_pt module = (module_pt) 0x10;
	capability->module = module;

	module_pt actual = NULL;
	capability_getModule(capability, &actual);
	POINTERS_EQUAL(module, actual);

	free(capability);
}
