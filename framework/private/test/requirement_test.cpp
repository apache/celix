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
 *  \author     <a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
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
}

int main(int argc, char** argv) {
	return RUN_ALL_TESTS(argc, argv);
}

TEST_GROUP(requirement) {
	apr_pool_t *pool;

	void setup(void) {
		apr_initialize();
		apr_pool_create(&pool, NULL);
	}

	void teardown() {
		apr_pool_destroy(pool);
		mock().checkExpectations();
		mock().clear();
	}
};

TEST(requirement, create) {
	hash_map_pt directives = hashMap_create(NULL, NULL, NULL, NULL);
	hash_map_pt attributes = hashMap_create(NULL, NULL, NULL, NULL);

	attribute_pt serviceAttribute = (attribute_pt) 0x01;
	hashMap_put(attributes, (void*) "service", serviceAttribute);
	attribute_pt versionAttribute = (attribute_pt) 0x02;
	hashMap_put(attributes, (void*) "version", versionAttribute);

	version_range_pt infiniteRange = (version_range_pt) 0x10;
	version_range_pt parsedRange = (version_range_pt) 0x11;

	mock().expectOneCall("attribute_getValue")
        .withParameter("attribute", serviceAttribute)
        .andOutputParameter("value", (char *) "target")
        .andReturnValue(CELIX_SUCCESS);

	mock().expectOneCall("versionRange_createInfiniteVersionRange")
	    .withParameter("pool", pool)
	    .andOutputParameter("range", infiniteRange)
        .andReturnValue(CELIX_SUCCESS);

	mock().expectOneCall("attribute_getValue")
        .withParameter("attribute", versionAttribute)
        .andOutputParameter("value", (char *) "1.0.0")
        .andReturnValue(CELIX_SUCCESS);

	mock().expectOneCall("versionRange_parse")
        .withParameter("pool", pool)
        .withParameter("rangeStr", (char *) "1.0.0")
        .andOutputParameter("range", parsedRange)
        .andReturnValue(CELIX_SUCCESS);

	requirement_pt requirement = NULL;
	requirement_create(pool, directives, attributes, &requirement);
}

TEST(requirement, getVersionRange) {
	requirement_pt requirement = (requirement_pt) apr_palloc(pool, sizeof(*requirement));
	version_range_pt versionRange = (version_range_pt) 0x10;
	requirement->versionRange = versionRange;

	version_range_pt actual = NULL;
	requirement_getVersionRange(requirement, &actual);
	POINTERS_EQUAL(versionRange, actual);
}

TEST(requirement, getTargetName) {
	requirement_pt requirement = (requirement_pt) apr_palloc(pool, sizeof(*requirement));
	char targetName[] = "target";
	requirement->targetName = targetName;

	char *actual = NULL;
	requirement_getTargetName(requirement, &actual);
	STRCMP_EQUAL(targetName, actual);
}

TEST(requirement, isSatisfied) {
	requirement_pt requirement = (requirement_pt) apr_palloc(pool, sizeof(*requirement));
	version_range_pt versionRange = (version_range_pt) 0x10;
	requirement->versionRange = versionRange;

	capability_pt capability = (capability_pt) 0x20;
	version_pt version = (version_pt) 0x30;

	mock().expectOneCall("capability_getVersion")
			.withParameter("capability", capability)
			.andOutputParameter("version", version);
	mock().expectOneCall("versionRange_isInRange")
		.withParameter("versionRange", versionRange)
		.withParameter("version", version)
		.andOutputParameter("inRange", true);

	bool inRange = false;
	requirement_isSatisfied(requirement, capability, &inRange);
	CHECK(inRange);
}
