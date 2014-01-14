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
#include <apr_file_io.h>
#include "bundle_cache_private.h"
#include "celix_log.h"

framework_logger_pt logger;
}

int main(int argc, char** argv) {
	return RUN_ALL_TESTS(argc, argv);
}

TEST_GROUP(bundle_cache) {
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

TEST(bundle_cache, create) {
	properties_pt configuration = (properties_pt) 0x10;

	mock().expectOneCall("properties_get")
		.withParameter("properties", configuration)
		.withParameter("key", "org.osgi.framework.storage")
		.andReturnValue((char *) NULL);

	bundle_cache_pt cache = NULL;
	celix_status_t status = bundleCache_create(configuration, pool, logger, &cache);
	LONGS_EQUAL(CELIX_SUCCESS, status);
}

TEST(bundle_cache, deleteTree) {
	bundle_cache_pt cache = (bundle_cache_pt) apr_palloc(pool, sizeof(*cache));
	char cacheDir[] = "bundle_cache_test_directory";
	char cacheFile[] = "bundle_cache_test_directory/temp";
	cache->cacheDir = cacheDir;
	cache->mp = pool;

	apr_dir_make(cacheDir, APR_UREAD|APR_UWRITE|APR_UEXECUTE, pool);
	apr_file_t *file;
	apr_file_mktemp(&file, cacheFile, APR_UREAD|APR_UWRITE, pool);

	celix_status_t status = bundleCache_delete(cache);

	LONGS_EQUAL(CELIX_SUCCESS, status);

}

TEST(bundle_cache, getArchive) {
	bundle_cache_pt cache = (bundle_cache_pt) apr_palloc(pool, sizeof(*cache));
	char cacheDir[] = "bundle_cache_test_directory";
	cache->cacheDir = cacheDir;
	cache->mp = pool;

	char bundle0[] = "bundle_cache_test_directory/bundle0";
	char bundle1[] = "bundle_cache_test_directory/bundle1";
	apr_dir_make(cacheDir, APR_UREAD|APR_UWRITE|APR_UEXECUTE, pool);
	apr_dir_make(bundle0, APR_UREAD|APR_UWRITE|APR_UEXECUTE, pool);
	apr_dir_make(bundle1, APR_UREAD|APR_UWRITE|APR_UEXECUTE, pool);

	bundle_archive_pt archive = (bundle_archive_pt) 0x10;
	mock().expectOneCall("bundleArchive_recreate")
		.withParameter("archiveRoot", bundle1)
		.withParameter("mp", pool)
		.andOutputParameter("bundle_archive", archive)
		.andReturnValue(CELIX_SUCCESS);

	array_list_pt archives = NULL;
	celix_status_t status = bundleCache_getArchives(cache, pool, &archives);

	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK(archives);
	LONGS_EQUAL(1, arrayList_size(archives));
	POINTERS_EQUAL(archive, arrayList_get(archives, 0));

	apr_dir_remove(bundle0, pool);
	apr_dir_remove(bundle1, pool);
	apr_dir_remove(cacheDir, pool);
}

TEST(bundle_cache, createArchive) {
	bundle_cache_pt cache = (bundle_cache_pt) apr_palloc(pool, sizeof(*cache));
	char cacheDir[] = "bundle_cache_test_directory";
	cache->cacheDir = cacheDir;

	char archiveRoot[] = "bundle_cache_test_directory/bundle1";
	int id = 1;
	char location[] = "test.zip";
	bundle_archive_pt archive = (bundle_archive_pt) 0x10;
	mock().expectOneCall("bundleArchive_create")
        .withParameter("logger", (void *) NULL)
		.withParameter("archiveRoot", archiveRoot)
		.withParameter("id", id)
		.withParameter("location", location)
		.withParameter("inputFile", (char *) NULL)
		.withParameter("mp", pool)
		.andOutputParameter("bundle_archive", archive)
		.andReturnValue(CELIX_SUCCESS);

	bundle_archive_pt actual;
	bundleCache_createArchive(cache, pool, 1l, location, NULL, &actual);
	POINTERS_EQUAL(archive, actual);
}
