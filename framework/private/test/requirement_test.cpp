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
 * requirement_test.cpp
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
#include "requirement_private.h"
#include "attribute.h"
#include "version_range.h"
#include "celix_log.h"
#include "utils.h"

framework_logger_pt logger = (framework_logger_pt) 0x42;
}

int main(int argc, char** argv) {
	return RUN_ALL_TESTS(argc, argv);
}

TEST_GROUP(requirement) {
	void setup(void) {
	}

	void teardown() {
		mock().checkExpectations();
		mock().clear();
	}
};

TEST(requirement, create) {
	requirement_pt requirement = NULL;
	hash_map_pt directives;
	hash_map_pt attributes;

	attribute_pt serviceAttribute = (attribute_pt) 0x01;
	attribute_pt versionAttribute = (attribute_pt) 0x02;

	version_range_pt infiniteRange = (version_range_pt) 0x10;
	version_range_pt parsedRange = (version_range_pt) 0x11;

	char *value1 = (char *) "target";
	char *value2 = (char *) "1.0.0";

	//create with infinite version range
	directives = hashMap_create(NULL, NULL, NULL, NULL);
	attributes = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
	hashMap_put(attributes, (void*) "service", serviceAttribute);

	mock().expectOneCall("attribute_getValue")
        .withParameter("attribute", serviceAttribute)
        .withOutputParameterReturning("value", &value1, sizeof(value1))
        .andReturnValue(CELIX_SUCCESS);
	mock().expectOneCall("versionRange_createInfiniteVersionRange")
	    .withOutputParameterReturning("range", &infiniteRange, sizeof(infiniteRange))
        .andReturnValue(CELIX_SUCCESS);

	requirement_create(directives, attributes, &requirement);

	//clean up
	mock().expectOneCall("attribute_destroy")
			.withParameter("attribute", serviceAttribute);

	mock().expectOneCall("versionRange_destroy")
			.withParameter("range",	infiniteRange);

	requirement_destroy(requirement);
	requirement = NULL;

	//create with version range
	directives = hashMap_create(NULL, NULL, NULL, NULL);
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
	mock().expectOneCall("versionRange_parse")
        .withParameter("rangeStr", (char *) "1.0.0")
        .withOutputParameterReturning("range", &parsedRange, sizeof(parsedRange))
        .andReturnValue(CELIX_SUCCESS);


	requirement_create(directives, attributes, &requirement);

	//clean up
	mock().expectOneCall("attribute_destroy")
			.withParameter("attribute", versionAttribute);

	mock().expectOneCall("attribute_destroy")
			.withParameter("attribute", serviceAttribute);

	mock().expectOneCall("versionRange_destroy")
			.withParameter("range", parsedRange);

	requirement_destroy(requirement);
}

TEST(requirement, getVersionRange) {
	requirement_pt requirement = (requirement_pt) malloc(sizeof(*requirement));
	version_range_pt versionRange = (version_range_pt) 0x10;
	requirement->versionRange = versionRange;

	version_range_pt actual = NULL;
	requirement_getVersionRange(requirement, &actual);
	POINTERS_EQUAL(versionRange, actual);

	free(requirement);
}

TEST(requirement, getTargetName) {
	requirement_pt requirement = (requirement_pt) malloc(sizeof(*requirement));
	char targetName[] = "target";
	requirement->targetName = targetName;

	const char *actual = NULL;
	requirement_getTargetName(requirement, &actual);
	STRCMP_EQUAL(targetName, actual);

	free(requirement);
}

TEST(requirement, isSatisfied) {
	requirement_pt requirement = (requirement_pt) malloc(sizeof(*requirement));
	version_range_pt versionRange = (version_range_pt) 0x10;
	requirement->versionRange = versionRange;

	capability_pt capability = (capability_pt) 0x20;
	version_pt version = (version_pt) 0x30;

	mock().expectOneCall("capability_getVersion")
			.withParameter("capability", capability)
			.withOutputParameterReturning("version", &version, sizeof(version));
	bool inRange1 = true;
	mock().expectOneCall("versionRange_isInRange")
		.withParameter("versionRange", versionRange)
		.withParameter("version", version)
		.withOutputParameterReturning("inRange", &inRange1, sizeof(inRange1));

	bool inRange2 = false;
	requirement_isSatisfied(requirement, capability, &inRange2);
	CHECK(inRange2);

	free(requirement);
}
