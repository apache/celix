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
 * bundle_cache_test.cpp
 *
 *  \date       Feb 11, 2013
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <celix_properties.h>

#include "CppUTest/TestHarness.h"
#include "CppUTest/TestHarness_c.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTestExt/MockSupport.h"

extern "C" {
#include "bundle_cache_private.h"
#include "celix_log.h"

framework_logger_pt logger = (framework_logger_pt) 0x42;
}

int main(int argc, char** argv) {
	return RUN_ALL_TESTS(argc, argv);
}

TEST_GROUP(bundle_cache) {
	void setup(void) {
	}

	void teardown() {
		mock().checkExpectations();
		mock().clear();
	}
};

TEST(bundle_cache, create) {
	properties_pt configuration = celix_properties_create();

	bundle_cache_pt cache = NULL;
	LONGS_EQUAL(CELIX_SUCCESS, bundleCache_create("test-uuid", configuration, &cache));

	LONGS_EQUAL(CELIX_SUCCESS, bundleCache_destroy(&cache));
}

TEST(bundle_cache, deleteTree) {
	bundle_cache_pt cache = (bundle_cache_pt) malloc(sizeof(*cache));
	char cacheDir[] = "bundle_cache_test_directory";
	char cacheDir2[] = "bundle_cache_test_directory/testdir";
	char cacheFile[] = "bundle_cache_test_directory/tempXXXXXX";
	cache->cacheDir = cacheDir;

	int rv = 0;
	rv += mkdir(cacheDir, S_IRWXU);
	rv += mkdir(cacheDir2, S_IRWXU);

	/* Check previous mkdir calls were fine*/
	LONGS_EQUAL(rv,0);

	//mkstemp opens the file for safety, but bundlecache_delete needs to reopen the file
	umask(0033);
	int fd = mkstemp(cacheFile);
	if(fd>=0){
		close(fd);
	}

	LONGS_EQUAL(CELIX_SUCCESS, bundleCache_delete(cache));

	free(cache);
}

TEST(bundle_cache, getArchive) {
	bundle_cache_pt cache = (bundle_cache_pt) malloc(sizeof(*cache));
	char cacheDir[] = "bundle_cache_test_directory";
	cache->cacheDir = cacheDir;

	char bundle0[] = "bundle_cache_test_directory/bundle0";
	char bundle1[] = "bundle_cache_test_directory/bundle1";
	int rv = 0;
	rv += mkdir(cacheDir, S_IRWXU);
	rv += mkdir(bundle0, S_IRWXU);
	rv += mkdir(bundle1, S_IRWXU);

	/* Check previous mkdir calls were fine*/
	LONGS_EQUAL(rv,0);

	bundle_archive_pt archive = (bundle_archive_pt) 0x10;
	mock().expectOneCall("bundleArchive_recreate")
		.withParameter("archiveRoot", bundle1)
		.withOutputParameterReturning("bundle_archive", &archive, sizeof(archive))
		.andReturnValue(CELIX_SUCCESS);

	array_list_pt archives = NULL;
	LONGS_EQUAL(CELIX_SUCCESS, bundleCache_getArchives(cache, &archives));

	CHECK(archives);
	LONGS_EQUAL(1, arrayList_size(archives));
	POINTERS_EQUAL(archive, arrayList_get(archives, 0));

	rmdir(bundle0);
	rmdir(bundle1);
	rmdir(cacheDir);
	arrayList_destroy(archives);

	LONGS_EQUAL(CELIX_SUCCESS, bundleCache_getArchives(cache, &archives));

	arrayList_destroy(archives);
	rmdir(cacheDir);
	free(cache);
}

TEST(bundle_cache, createArchive) {
	bundle_cache_pt cache = (bundle_cache_pt) malloc(sizeof(*cache));
	char cacheDir[] = "bundle_cache_test_directory";
	cache->cacheDir = cacheDir;

	char archiveRoot[] = "bundle_cache_test_directory/bundle1";
	int id = 1;
	char location[] = "test.zip";
	bundle_archive_pt archive = (bundle_archive_pt) 0x10;
	mock().expectOneCall("bundleArchive_create")
		.withParameter("archiveRoot", archiveRoot)
		.withParameter("id", id)
		.withParameter("location", location)
		.withParameter("inputFile", (char *) NULL)
		.withOutputParameterReturning("bundle_archive", &archive, sizeof(archive))
		.andReturnValue(CELIX_SUCCESS);

	bundle_archive_pt actual;
	bundleCache_createArchive(cache, 1l, location, NULL, &actual);
	POINTERS_EQUAL(archive, actual);

	free(cache);
}
