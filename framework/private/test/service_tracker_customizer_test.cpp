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
 * service_tracker_customizer_test.cpp
 *
 *  \date       Feb 7, 2013
 *  \author     <a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <stdlib.h>
//#include <stdio.h>

#include "CppUTest/TestHarness.h"
#include "CppUTest/TestHarness_c.h"
#include "CppUTest/CommandLineTestRunner.h"
//#include "CppUTestExt/MockSupport.h"

extern "C"
{
#include "service_tracker_customizer_private.h"
#include "service_reference.h"
#include "celix_log.h"

framework_logger_pt logger;
}

int main(int argc, char** argv) {
	return RUN_ALL_TESTS(argc, argv);
}

TEST_GROUP(service_tracker_customizer) {
	apr_pool_t *pool;

	void setup(void) {
		apr_initialize();
		apr_pool_create(&pool, NULL);

		logger = (framework_logger_pt) apr_palloc(pool, sizeof(*logger));
        logger->logFunction = frameworkLogger_log;
	}

	void teardown() {
		apr_pool_destroy(pool);
//		mock().checkExpectations();
//		mock().clear();
	}
};

extern "C" {
	celix_status_t serviceTrackerCustomizerTest_addingService(void * handle, service_reference_pt reference, void **service) {
		return CELIX_SUCCESS;
	}
	celix_status_t serviceTrackerCustomizerTest_addedService(void * handle, service_reference_pt reference, void * service) {
		return CELIX_SUCCESS;
	}
	celix_status_t serviceTrackerCustomizerTest_modifiedService(void * handle, service_reference_pt reference, void * service) {
		return CELIX_SUCCESS;
	}
	celix_status_t serviceTrackerCustomizerTest_removedService(void * handle, service_reference_pt reference, void * service) {
		return CELIX_SUCCESS;
	}
}

TEST(service_tracker_customizer, create) {
	void *handle = (void *) 0x10;
	service_tracker_customizer_pt customizer = NULL;
	celix_status_t status = serviceTrackerCustomizer_create(pool,
			handle,
			serviceTrackerCustomizerTest_addingService,
			serviceTrackerCustomizerTest_addedService,
			serviceTrackerCustomizerTest_modifiedService,
			serviceTrackerCustomizerTest_removedService, &customizer);

	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(handle, customizer->handle);
	POINTERS_EQUAL(serviceTrackerCustomizerTest_addingService, customizer->addingService);
	POINTERS_EQUAL(serviceTrackerCustomizerTest_addedService, customizer->addedService);
	POINTERS_EQUAL(serviceTrackerCustomizerTest_modifiedService, customizer->modifiedService);
	POINTERS_EQUAL(serviceTrackerCustomizerTest_removedService, customizer->removedService);
}

TEST(service_tracker_customizer, createIllegalArgument) {
	void *handle = (void *) 0x10;
	service_tracker_customizer_pt customizer = NULL;
	celix_status_t status = serviceTrackerCustomizer_create(pool,
			NULL,
			serviceTrackerCustomizerTest_addingService,
			serviceTrackerCustomizerTest_addedService,
			serviceTrackerCustomizerTest_modifiedService,
			serviceTrackerCustomizerTest_removedService, &customizer);

	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST(service_tracker_customizer, getHandle) {
	service_tracker_customizer_pt customizer = (service_tracker_customizer_pt) apr_palloc(pool, sizeof(*customizer));
	void *handle = (void *) 0x10;
	customizer->handle = handle;

	void *getHandle = NULL;
	celix_status_t status = serviceTrackerCustomizer_getHandle(customizer, &getHandle);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(handle, getHandle);
}

TEST(service_tracker_customizer, getAddingFunction) {
	service_tracker_customizer_pt customizer = (service_tracker_customizer_pt) apr_palloc(pool, sizeof(*customizer));
	adding_callback_pt adding = (adding_callback_pt) 0x11;
	customizer->addingService = adding;

	adding_callback_pt getAdding = NULL;
	celix_status_t status = serviceTrackerCustomizer_getAddingFunction(customizer, &getAdding);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(adding, getAdding);
}

TEST(service_tracker_customizer, getAddedFunction) {
	service_tracker_customizer_pt customizer = (service_tracker_customizer_pt) apr_palloc(pool, sizeof(*customizer));
	added_callback_pt added = (added_callback_pt) 0x11;
	customizer->addedService = added;

	added_callback_pt getAdded = NULL;
	celix_status_t status = serviceTrackerCustomizer_getAddedFunction(customizer, &getAdded);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(added, getAdded);
}

TEST(service_tracker_customizer, getModifiedFunction) {
	service_tracker_customizer_pt customizer = (service_tracker_customizer_pt) apr_palloc(pool, sizeof(*customizer));
	modified_callback_pt modified = (modified_callback_pt) 0x11;
	customizer->modifiedService = modified;

	modified_callback_pt getModified = NULL;
	celix_status_t status = serviceTrackerCustomizer_getModifiedFunction(customizer, &getModified);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(modified, getModified);
}

TEST(service_tracker_customizer, getRemovedFunction) {
	service_tracker_customizer_pt customizer = (service_tracker_customizer_pt) apr_palloc(pool, sizeof(*customizer));
	removed_callback_pt removed = (removed_callback_pt) 0x11;
	customizer->removedService = removed;

	removed_callback_pt getRemoved = NULL;
	celix_status_t status = serviceTrackerCustomizer_getRemovedFunction(customizer, &getRemoved);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(removed, getRemoved);
}

