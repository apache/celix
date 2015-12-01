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
 * bundle_archive_test.cpp
 *
 *  \date       Feb 11, 2013
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>

#include "CppUTest/TestHarness.h"
#include "CppUTest/TestHarness_c.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTestExt/MockSupport.h"

extern "C" {
#include "bundle_archive.h"
framework_logger_pt logger = (framework_logger_pt) 0x42;
}

int main(int argc, char** argv) {
	return RUN_ALL_TESTS(argc, argv);
}

//----------------------TESTGROUP DEFINES----------------------

TEST_GROUP(bundle_archive) {
	bundle_archive_pt bundle_archive;
	char * bundle_path; 			//uses the default build shell bundle
	char * alternate_bundle_path;//alternative bundle, if shell bundle not found
	char cache_path[512];				//a local cache folder

	void setup(void) {
		char cwd[512];
		bundle_archive = NULL;
		bundle_path = (char*) "../shell/shell.zip"; //uses the default build shell bundle
		alternate_bundle_path = (char*) "../log_service/log_service.zip"; //alternative bundle, if shell bundle not found
		snprintf(cache_path, sizeof(cache_path), "%s/.cache", getcwd(cwd, sizeof(cwd)));							//a local cache folder
		struct stat file_stat;

		if (stat(bundle_path, &file_stat) < 0) {
			if (stat(alternate_bundle_path, &file_stat) < 0) {
				FAIL("failed to find needed test bundle");
			} else {
				bundle_path = alternate_bundle_path;
			}
		}
	}

	void teardown() {
		mock().checkExpectations();
		mock().clear();
	}
};

//----------------------TEST DEFINES----------------------
//WARNING: if one test fails, it does not clean up properly,
//causing most if not all of the subsequent tests to also fail

TEST(bundle_archive, create) {
	mock().expectOneCall("framework_logCode").withParameter("code", CELIX_ILLEGAL_ARGUMENT);

	bundle_archive = (bundle_archive_pt) 0x42;
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, bundleArchive_create(cache_path, 5, bundle_path, NULL, &bundle_archive));

	bundle_archive = NULL;
	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_create(cache_path, 5, bundle_path, NULL, &bundle_archive));
	CHECK(bundle_archive != NULL);

	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_closeAndDelete(bundle_archive));
	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_destroy(bundle_archive));
}

TEST(bundle_archive, createSystemBundleArchive) {
	mock().expectOneCall("framework_logCode").withParameter("code", CELIX_ILLEGAL_ARGUMENT);

	bundle_archive = (bundle_archive_pt) 0x42;
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, bundleArchive_createSystemBundleArchive(&bundle_archive ));

	bundle_archive = NULL;
	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_createSystemBundleArchive(&bundle_archive));
	CHECK(bundle_archive != NULL);

	//LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_closeAndDelete(bundle_archive));
	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_destroy(bundle_archive));
}

TEST(bundle_archive, recreate) {
	char * get_root;
	char * get_location;
	bundle_archive_pt recr_archive = NULL;

	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_create(cache_path, 5, bundle_path, NULL, &bundle_archive));
	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_recreate(cache_path, &recr_archive));

	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_getArchiveRoot(recr_archive, &get_root));
	STRCMP_EQUAL(cache_path, get_root);
	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_getLocation(recr_archive, &get_location));
	STRCMP_EQUAL(bundle_path, get_location);

	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_closeAndDelete(bundle_archive));
	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_destroy(bundle_archive));
	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_destroy(recr_archive));
}

TEST(bundle_archive, getId) {
	long get_id;
	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_create(cache_path, -42, bundle_path, NULL, &bundle_archive));
	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_getId(bundle_archive, &get_id));
	LONGS_EQUAL(-42, get_id);

	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_closeAndDelete(bundle_archive));
	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_destroy(bundle_archive));
	bundle_archive = NULL;

	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_create(cache_path, 666, bundle_path, NULL, &bundle_archive));
	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_getId(bundle_archive, &get_id));
	LONGS_EQUAL(666, get_id);

	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_closeAndDelete(bundle_archive));
	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_destroy(bundle_archive));
}

TEST(bundle_archive, getLocation) {
	char * get_loc;

	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_create(cache_path, 1, bundle_path, NULL, &bundle_archive));
	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_getLocation(bundle_archive, &get_loc));
	STRCMP_EQUAL(bundle_path, get_loc);

	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_closeAndDelete(bundle_archive));
	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_destroy(bundle_archive));
}

TEST(bundle_archive, getArchiveRoot) {
	char * get_root;

	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_create(cache_path, 1, bundle_path, NULL, &bundle_archive));
	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_getArchiveRoot(bundle_archive, &get_root));
	STRCMP_EQUAL(cache_path, get_root);

	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_closeAndDelete(bundle_archive));
	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_destroy(bundle_archive));
}

TEST(bundle_archive, getCurrentRevisionNumber) {
	long get_rev_num;

	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_create(cache_path, 1, bundle_path, NULL, &bundle_archive));
	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_getCurrentRevisionNumber(bundle_archive, &get_rev_num));
	LONGS_EQUAL(0, get_rev_num);

	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_revise(bundle_archive, bundle_path, NULL));
	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_getCurrentRevisionNumber(bundle_archive,
					&get_rev_num));
	LONGS_EQUAL(1, get_rev_num);

	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_revise(bundle_archive, bundle_path, NULL));
	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_getCurrentRevisionNumber(bundle_archive, &get_rev_num));
	LONGS_EQUAL(2, get_rev_num);

	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_closeAndDelete(bundle_archive));
	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_destroy(bundle_archive));
}

TEST(bundle_archive, getCurrentRevision) {
	long get_rev_num;
	bundle_revision_pt get_rev;

	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_create(cache_path, 1, bundle_path, NULL, &bundle_archive));
	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_getCurrentRevision(bundle_archive, &get_rev));
	bundleRevision_getNumber(get_rev, &get_rev_num);
	LONGS_EQUAL(0, get_rev_num);

	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_revise(bundle_archive, bundle_path, NULL));
	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_getCurrentRevision(bundle_archive, &get_rev));
	bundleRevision_getNumber(get_rev, &get_rev_num);
	LONGS_EQUAL(1, get_rev_num);

	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_closeAndDelete(bundle_archive));
	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_destroy(bundle_archive));
}

TEST(bundle_archive, getRevision) {
	long get_rev_num;
	bundle_revision_pt get_rev;

	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_create(cache_path, 1, bundle_path, NULL, &bundle_archive));
	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_revise(bundle_archive, bundle_path, NULL));
	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_revise(bundle_archive, bundle_path, NULL));

	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_getRevision(bundle_archive, 0, &get_rev));
	bundleRevision_getNumber(get_rev, &get_rev_num);
	LONGS_EQUAL(0, get_rev_num);

	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_getRevision(bundle_archive, 2, &get_rev));
	bundleRevision_getNumber(get_rev, &get_rev_num);
	LONGS_EQUAL(2, get_rev_num);

	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_getRevision(bundle_archive, 1, &get_rev));
	bundleRevision_getNumber(get_rev, &get_rev_num);
	LONGS_EQUAL(1, get_rev_num);

	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_closeAndDelete(bundle_archive));
	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_destroy(bundle_archive));
}

TEST(bundle_archive, set_getPersistentState) {
	bundle_state_e get;

	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_create(cache_path, 1, bundle_path, NULL, &bundle_archive));

	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_setPersistentState(bundle_archive, OSGI_FRAMEWORK_BUNDLE_UNKNOWN));
	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_getPersistentState(bundle_archive, &get));
	LONGS_EQUAL(OSGI_FRAMEWORK_BUNDLE_INSTALLED, get);

	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_setPersistentState(bundle_archive, OSGI_FRAMEWORK_BUNDLE_ACTIVE));
	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_getPersistentState(bundle_archive, &get));
	LONGS_EQUAL(OSGI_FRAMEWORK_BUNDLE_ACTIVE, get);

	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_setPersistentState(bundle_archive, OSGI_FRAMEWORK_BUNDLE_STARTING));
	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_getPersistentState(bundle_archive, &get));
	LONGS_EQUAL(OSGI_FRAMEWORK_BUNDLE_STARTING, get);

	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_setPersistentState(bundle_archive, OSGI_FRAMEWORK_BUNDLE_UNINSTALLED));
	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_getPersistentState(bundle_archive, &get));
	LONGS_EQUAL(OSGI_FRAMEWORK_BUNDLE_UNINSTALLED, get);

	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_closeAndDelete(bundle_archive));
	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_destroy(bundle_archive));
}

TEST(bundle_archive, set_getRefreshCount) {
	long get_refresh_num;

	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_create(cache_path, 1, bundle_path, NULL, &bundle_archive));
	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_getRefreshCount(bundle_archive, &get_refresh_num));
	LONGS_EQUAL(0, get_refresh_num);

	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_setRefreshCount(bundle_archive));
	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_getRefreshCount(bundle_archive, &get_refresh_num));
	LONGS_EQUAL(0, get_refresh_num);

	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_closeAndDelete(bundle_archive));
	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_destroy(bundle_archive));
}

TEST(bundle_archive, get_setLastModified) {
	time_t set_time;
	time_t get_time;

	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_create(cache_path, 1, bundle_path, NULL, &bundle_archive));

	time(&set_time);
	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_setLastModified(bundle_archive, set_time));
	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_getLastModified(bundle_archive, &get_time));
	CHECK(set_time == get_time);

	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_closeAndDelete(bundle_archive));
	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_destroy(bundle_archive));
}

//CANT seem to find a way to test this static function
/*TEST(bundle_archive, readLastModified) {
	mock().expectOneCall("framework_logCode");

	time_t set_time;
	time_t get_time;
	bundle_archive_pt recr_archive = NULL;

	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_create(cache_path, 1, bundle_path, NULL, &bundle_archive));

	time(&set_time);
	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_setLastModified(bundle_archive, set_time));
	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_recreate(cache_path, &recr_archive));
	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_getLastModified(recr_archive, &get_time));
	LONGS_EQUAL(set_time, get_time);

	LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_closeAndDelete(bundle_archive));
	//LONGS_EQUAL(CELIX_SUCCESS, bundleArchive_destroy(bundle_archive));
}*/

TEST(bundle_archive, rollbackRevise) {
	bool get;
	bundleArchive_rollbackRevise(NULL, &get);

	LONGS_EQUAL(true, get);

}

