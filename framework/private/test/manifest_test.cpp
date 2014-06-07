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
 * manifest_test.cpp
 *
 *  \date       Feb 11, 2013
 *  \author     <a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdio.h>

#include <apr_general.h>

#include "CppUTest/TestHarness.h"
#include "CppUTest/TestHarness_c.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTestExt/MockSupport.h"

extern "C" {
#include "manifest.h"
#include "hash_map.h"
#include "celix_log.h"

framework_logger_pt logger;
}

int main(int argc, char** argv) {
	return RUN_ALL_TESTS(argc, argv);
}

TEST_GROUP(manifest) {
	apr_pool_t *pool;

	void setup(void) {
		apr_initialize();
		apr_pool_create(&pool, NULL);

		logger = (framework_logger_pt) apr_palloc(pool, sizeof(*logger));
        logger->logFunction = frameworkLogger_log;
	}

	void teardown() {
		apr_pool_destroy(pool);
		mock().checkExpectations();
		mock().clear();
	}
};

TEST(manifest, createFromFile) {
    char manifestFile[] = "../../celix/framework/private/resources-test/manifest.txt";
    manifest_pt manifest = NULL;
//    properties_pt properties = properties_create();
    properties_pt properties = (properties_pt) 0x40;
    void *ov = (void *) 0x00;

    mock()
        .expectOneCall("properties_create")
        .andReturnValue(properties);
    mock()
        .expectOneCall("properties_set")
        .withParameter("properties", properties)
        .withParameter("key", "Bundle-SymbolicName")
        .withParameter("value", "client")
        .andReturnValue(ov);
    mock()
        .expectOneCall("properties_set")
        .withParameter("properties", properties)
        .withParameter("key", "Bundle-Version")
        .withParameter("value", "1.0.0")
        .andReturnValue(ov);
    mock()
        .expectOneCall("properties_set")
        .withParameter("properties", properties)
        .withParameter("key", "library")
        .withParameter("value", "client")
        .andReturnValue(ov);
    mock()
        .expectOneCall("properties_set")
        .withParameter("properties", properties)
        .withParameter("key", "Import-Service")
        .withParameter("value", "server")
        .andReturnValue(ov);
    mock()
        .expectOneCall("properties_destroy")
        .withParameter("properties", properties);

    manifest_createFromFile(manifestFile, &manifest);
}

TEST(manifest, createFromFileWithSections) {
    char manifestFile[] = "../../celix/framework/private/resources-test/manifest_sections.txt";
    manifest_pt manifest = NULL;
//    properties_pt properties = properties_create();
    properties_pt properties = (properties_pt) 0x40;
    properties_pt properties2 = (properties_pt) 0x41;
    properties_pt properties3 = (properties_pt) 0x42;
    void *ov = (void *) 0x00;

    mock()
        .expectOneCall("properties_create")
        .andReturnValue(properties);
    mock()
        .expectOneCall("properties_set")
        .withParameter("properties", properties)
        .withParameter("key", "Bundle-SymbolicName")
        .withParameter("value", "client")
        .andReturnValue(ov);
    mock()
        .expectOneCall("properties_set")
        .withParameter("properties", properties)
        .withParameter("key", "Bundle-Version")
        .withParameter("value", "1.0.0")
        .andReturnValue(ov);
    mock()
        .expectOneCall("properties_set")
        .withParameter("properties", properties)
        .withParameter("key", "library")
        .withParameter("value", "client")
        .andReturnValue(ov);
    mock()
        .expectOneCall("properties_set")
        .withParameter("properties", properties)
        .withParameter("key", "Import-Service")
        .withParameter("value", "server")
        .andReturnValue(ov);
    mock()
        .expectOneCall("properties_create")
        .andReturnValue(properties2);
    mock()
        .expectOneCall("properties_set")
        .withParameter("properties", properties2)
        .withParameter("key", "a")
        .withParameter("value", "1")
        .andReturnValue(ov);
    mock()
        .expectOneCall("properties_create")
        .andReturnValue(properties3);
    mock()
        .expectOneCall("properties_set")
        .withParameter("properties", properties3)
        .withParameter("key", "b")
        .withParameter("value", "1")
        .andReturnValue(ov);
    mock()
        .expectOneCall("properties_get")
        .withParameter("properties", properties)
        .withParameter("key", "Bundle-SymbolicName")
        .andReturnValue("bsn");
    mock()
        .expectOneCall("properties_destroy")
        .withParameter("properties", properties);

    manifest_createFromFile(manifestFile, &manifest);

    properties_pt main = manifest_getMainAttributes(manifest);
    POINTERS_EQUAL(properties, main);

    char *name = manifest_getValue(manifest, "Bundle-SymbolicName");
    STRCMP_EQUAL("bsn", name);

    hash_map_pt map = NULL;
    manifest_getEntries(manifest, &map);
    LONGS_EQUAL(2, hashMap_size(map));

    properties_pt actual = (properties_pt) hashMap_get(map, (void *) "a");
    POINTERS_EQUAL(properties2, actual);

    actual = (properties_pt) hashMap_get(map, (void *) "b");
    POINTERS_EQUAL(properties3, actual);
}

