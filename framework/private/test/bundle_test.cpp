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
 * bundle_test.cpp
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
#include "bundle_private.h"
}

int main(int argc, char** argv) {
	return RUN_ALL_TESTS(argc, argv);
}

TEST_GROUP(bundle) {
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

TEST(bundle, create) {
	bundle_archive_pt archive = (bundle_archive_pt) 0x10;
	mock().expectOneCall("bundleArchive_createSystemBundleArchive")
			.withParameter("pool", pool)
			.andOutputParameter("bundle_archive", archive)
			.andReturnValue(CELIX_SUCCESS);

	module_pt module = (module_pt) 0x20;
	mock().expectOneCall("module_createFrameworkModule")
			.ignoreOtherParameters()
			.andReturnValue(module);

	mock().expectOneCall("resolver_addModule")
			.withParameter("module", module);
	mock().expectOneCall("resolver_addModule")
			.withParameter("module", module);

	bundle_pt actual = NULL;
	celix_status_t status = bundle_create(&actual, pool);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(NULL, actual->context);
	POINTERS_EQUAL(NULL, actual->activator);
	LONGS_EQUAL(OSGI_FRAMEWORK_BUNDLE_INSTALLED, actual->state);
	POINTERS_EQUAL(NULL, actual->handle);
	POINTERS_EQUAL(archive, actual->archive);
	CHECK(actual->modules);
	POINTERS_EQUAL(NULL, actual->manifest);
	POINTERS_EQUAL(pool, actual->memoryPool);
	CHECK(actual->lock)
	LONGS_EQUAL(0, actual->lockCount);
	POINTERS_EQUAL(NULL, actual->lockThread);
	POINTERS_EQUAL(NULL, actual->framework);
}

TEST(bundle, createFromArchive) {
	framework_pt framework = (framework_pt) 0x10;
	bundle_archive_pt archive = (bundle_archive_pt) 0x20;
	bundle_revision_pt revision = (bundle_revision_pt) 0x21;
	manifest_pt manifest = (manifest_pt) 0x30;


	mock().expectOneCall("bundleArchive_getCurrentRevision")
        .withParameter("archive", archive)
        .andOutputParameter("revision", revision)
        .andReturnValue(CELIX_SUCCESS);

	mock().expectOneCall("bundleRevision_getManifest")
		.withParameter("revision", revision)
		.andOutputParameter("manifest", manifest)
		.andReturnValue(CELIX_SUCCESS);

	// CPPUTest has no build in support for longs, which breaks this test.
	mock().expectOneCall("bundleArchive_getId")
		.withParameter("archive", archive)
		.andOutputParameter("id", 1)
		.andReturnValue(CELIX_SUCCESS);

	module_pt module = (module_pt) 0x40;
	mock().expectOneCall("module_create")
		.withParameter("headerMap", manifest)
//		.withParameter("moduleId", "1.0")
//		.withParameter("bundle", (void *) 0x40)
		.ignoreOtherParameters()
		.andReturnValue(module);

	version_pt version = (version_pt) 0x50;
	mock().expectOneCall("module_getVersion")
		.withParameter("module", module)
		.andReturnValue(version);

	char symbolicName[] = "name";
	mock().expectOneCall("module_getSymbolicName")
		.withParameter("module", module)
		.andOutputParameter("symbolicName", symbolicName)
		.andReturnValue(CELIX_SUCCESS);

	array_list_pt bundles = NULL;
	arrayList_create(&bundles);
	mock().expectOneCall("framework_getBundles")
		.withParameter("framework", framework)
		.andReturnValue(bundles);

	mock().expectOneCall("resolver_addModule")
		.withParameter("module", module);

	mock().expectOneCall("resolver_addModule")
		.withParameter("module", module);

	bundle_pt actual = NULL;
	celix_status_t status = bundle_createFromArchive(&actual, framework, archive, pool);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(NULL, actual->context);
	POINTERS_EQUAL(NULL, actual->activator);
	LONGS_EQUAL(OSGI_FRAMEWORK_BUNDLE_INSTALLED, actual->state);
	POINTERS_EQUAL(NULL, actual->handle);
	POINTERS_EQUAL(archive, actual->archive);
	CHECK(actual->modules);
	POINTERS_EQUAL(NULL, actual->manifest);
	POINTERS_EQUAL(pool, actual->memoryPool);
	CHECK(actual->lock)
	LONGS_EQUAL(0, actual->lockCount);
	POINTERS_EQUAL(NULL, actual->lockThread);
	POINTERS_EQUAL(framework, actual->framework);
}

TEST(bundle, getArchive) {
	bundle_pt bundle = (bundle_pt) apr_palloc(pool, sizeof(*bundle));
	bundle_archive_pt archive = (bundle_archive_pt) 0x10;
	bundle->archive = archive;

	bundle_archive_pt actual = NULL;
	celix_status_t status = bundle_getArchive(bundle, &actual);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(archive, actual);

	actual = NULL;
	status = bundle_getArchive(NULL, &actual);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST(bundle, getCurrentModule) {
	bundle_pt bundle = (bundle_pt) apr_palloc(pool, sizeof(*bundle));
	module_pt module1 = (module_pt) 0x11;
	module_pt module2 = (module_pt) 0x12;
	module_pt module3 = (module_pt) 0x13;
	module_pt module4 = (module_pt) 0x14;
	arrayList_create(&bundle->modules);
	arrayList_add(bundle->modules, module1);
	arrayList_add(bundle->modules, module2);
	arrayList_add(bundle->modules, module3);
	arrayList_add(bundle->modules, module4);

	module_pt actual = NULL;
	celix_status_t status = bundle_getCurrentModule(bundle, &actual);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(module4, actual);

	actual = NULL;
	status = bundle_getCurrentModule(NULL, &actual);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST(bundle, getModules) {
	bundle_pt bundle = (bundle_pt) apr_palloc(pool, sizeof(*bundle));
	array_list_pt modules = (array_list_pt) 0x10;
	bundle->modules = modules;

	array_list_pt actual = bundle_getModules(bundle);
	POINTERS_EQUAL(modules, actual);
}

TEST(bundle, getHandle) {
	bundle_pt bundle = (bundle_pt) apr_palloc(pool, sizeof(*bundle));
	void *expected = (void *) 0x10;
	bundle->handle = expected;

	void *actual = bundle_getHandle(bundle);
	POINTERS_EQUAL(expected, actual);
}

TEST(bundle, setHandle) {
	bundle_pt bundle = (bundle_pt) apr_palloc(pool, sizeof(*bundle));
	void *expected = (void *) 0x10;

	bundle_setHandle(bundle, expected);
	POINTERS_EQUAL(expected, bundle->handle);
}

TEST(bundle, getActivator) {
	bundle_pt bundle = (bundle_pt) apr_palloc(pool, sizeof(*bundle));
	activator_pt expected = (activator_pt) 0x10;
	bundle->activator = expected;

	activator_pt actual = bundle_getActivator(bundle);
	POINTERS_EQUAL(expected, actual);
}

TEST(bundle, setActivator) {
	bundle_pt bundle = (bundle_pt) apr_palloc(pool, sizeof(*bundle));
	activator_pt expected = (activator_pt) 0x10;

	celix_status_t status = bundle_setActivator(bundle, expected);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(expected, bundle->activator);
}

TEST(bundle, getContext) {
	bundle_pt bundle = (bundle_pt) apr_palloc(pool, sizeof(*bundle));
	bundle_context_pt expected = (bundle_context_pt) 0x10;
	bundle->context = expected;

	bundle_context_pt actual = NULL;
	celix_status_t status = bundle_getContext(bundle, &actual);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(expected, actual);
}

TEST(bundle, setContext) {
	bundle_pt bundle = (bundle_pt) apr_palloc(pool, sizeof(*bundle));
	bundle_context_pt expected = (bundle_context_pt) 0x10;

	celix_status_t status = bundle_setContext(bundle, expected);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(expected, bundle->context);
}

TEST(bundle, getEntry) {
	bundle_pt bundle = (bundle_pt) apr_palloc(pool, sizeof(*bundle));
	framework_pt framework = (framework_pt) 0x10;
	bundle->framework = framework;

	char name[] = "name";
	char *expected = (char *) 0x20;
	mock().expectOneCall("framework_getBundleEntry")
		.withParameter("framework", framework)
		.withParameter("bundle", bundle)
		.withParameter("name", name)
		.withParameter("pool", pool)
		.andOutputParameter("entry", expected)
		.andReturnValue(CELIX_SUCCESS);

	char *actual = NULL;
	celix_status_t status = bundle_getEntry(bundle, name, pool, &actual);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(expected, actual);
}

TEST(bundle, getState) {
	bundle_pt bundle = (bundle_pt) apr_palloc(pool, sizeof(*bundle));
	bundle->state = OSGI_FRAMEWORK_BUNDLE_ACTIVE;

	bundle_state_e actual = OSGI_FRAMEWORK_BUNDLE_UNKNOWN;
	celix_status_t status = bundle_getState(bundle, &actual);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(OSGI_FRAMEWORK_BUNDLE_ACTIVE, actual);
}

TEST(bundle, setState) {
	bundle_pt bundle = (bundle_pt) apr_palloc(pool, sizeof(*bundle));
	bundle->state = OSGI_FRAMEWORK_BUNDLE_UNKNOWN;

	celix_status_t status = bundle_setState(bundle, OSGI_FRAMEWORK_BUNDLE_INSTALLED);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(OSGI_FRAMEWORK_BUNDLE_INSTALLED, bundle->state);
}

TEST(bundle, start) {
	bundle_pt bundle = (bundle_pt) apr_palloc(pool, sizeof(*bundle));
	framework_pt framework = (framework_pt) 0x10;
	bundle_archive_pt archive = (bundle_archive_pt) 0x20;
	bundle->framework = framework;
	bundle->archive = archive;

	int options = 42;

	mock().expectOneCall("bundleArchive_getId")
        .withParameter("archive", archive)
        .andOutputParameter("id", 1)
        .andReturnValue(CELIX_SUCCESS);

	mock().expectOneCall("fw_startBundle")
		.withParameter("framework", framework)
		.withParameter("bundle", bundle)
		.withParameter("options", options)
		.andReturnValue(CELIX_SUCCESS);

	celix_status_t status = bundle_startWithOptions(bundle, options);
	LONGS_EQUAL(CELIX_SUCCESS, status);
}

TEST(bundle, update) {
	bundle_pt bundle = (bundle_pt) apr_palloc(pool, sizeof(*bundle));
	framework_pt framework = (framework_pt) 0x10;
	bundle_archive_pt archive = (bundle_archive_pt) 0x20;
	bundle->framework = framework;
	bundle->archive = archive;

	mock().expectOneCall("bundleArchive_getId")
        .withParameter("archive", archive)
        .andOutputParameter("id", 1)
        .andReturnValue(CELIX_SUCCESS);

	char input[] = "input";
	mock().expectOneCall("framework_updateBundle")
		.withParameter("framework", framework)
		.withParameter("bundle", bundle)
		.withParameter("inputFile", input)
		.andReturnValue(CELIX_SUCCESS);

	celix_status_t status = bundle_update(bundle, input);
	LONGS_EQUAL(CELIX_SUCCESS, status);
}

TEST(bundle, stop) {
	bundle_pt bundle = (bundle_pt) apr_palloc(pool, sizeof(*bundle));
	framework_pt framework = (framework_pt) 0x10;
	bundle_archive_pt archive = (bundle_archive_pt) 0x20;
	bundle->framework = framework;
	bundle->archive = archive;

	mock().expectOneCall("bundleArchive_getId")
	        .withParameter("archive", archive)
	        .andOutputParameter("id", 1)
	        .andReturnValue(CELIX_SUCCESS);

	int options = 1;
	mock().expectOneCall("fw_stopBundle")
		.withParameter("framework", framework)
		.withParameter("bundle", bundle)
		.withParameter("record", 1)
		.andReturnValue(CELIX_SUCCESS);

	celix_status_t status = bundle_stopWithOptions(bundle, options);
	LONGS_EQUAL(CELIX_SUCCESS, status);
}

TEST(bundle, uninstall) {
	bundle_pt bundle = (bundle_pt) apr_palloc(pool, sizeof(*bundle));
	framework_pt framework = (framework_pt) 0x10;
	bundle_archive_pt archive = (bundle_archive_pt) 0x20;
	bundle->framework = framework;
	bundle->archive = archive;

	mock().expectOneCall("bundleArchive_getId")
        .withParameter("archive", archive)
        .andOutputParameter("id", 1)
        .andReturnValue(CELIX_SUCCESS);

	mock().expectOneCall("fw_uninstallBundle")
		.withParameter("framework", framework)
		.withParameter("bundle", bundle)
		.andReturnValue(CELIX_SUCCESS);

	celix_status_t status = bundle_uninstall(bundle);
	LONGS_EQUAL(CELIX_SUCCESS, status);
}

TEST(bundle, setPersistentStateInactive) {
	bundle_pt bundle = (bundle_pt) apr_palloc(pool, sizeof(*bundle));
	bundle_archive_pt archive = (bundle_archive_pt) 0x10;
	bundle->archive = archive;


	mock().expectOneCall("bundleArchive_getId")
		.withParameter("archive", archive)
		.andOutputParameter("id", 0)
		.andReturnValue(CELIX_SUCCESS);

	mock().expectOneCall("bundleArchive_setPersistentState")
		.withParameter("archive", archive)
		.withParameter("state", OSGI_FRAMEWORK_BUNDLE_INSTALLED)
		.andReturnValue(CELIX_SUCCESS);

	celix_status_t status = bundle_setPersistentStateInactive(bundle);
	LONGS_EQUAL(CELIX_SUCCESS, status);
}

TEST(bundle, setPersistentStateUninstalled) {
	bundle_pt bundle = (bundle_pt) apr_palloc(pool, sizeof(*bundle));
	bundle_archive_pt archive = (bundle_archive_pt) 0x10;
	bundle->archive = archive;


	mock().expectOneCall("bundleArchive_getId")
		.withParameter("archive", archive)
		.andOutputParameter("id", 0)
		.andReturnValue(CELIX_SUCCESS);

	mock().expectOneCall("bundleArchive_setPersistentState")
		.withParameter("archive", archive)
		.withParameter("state", OSGI_FRAMEWORK_BUNDLE_UNINSTALLED)
		.andReturnValue(CELIX_SUCCESS);

	celix_status_t status = bundle_setPersistentStateUninstalled(bundle);
	LONGS_EQUAL(CELIX_SUCCESS, status);
}

TEST(bundle, revise) {
	bundle_pt bundle = (bundle_pt) apr_palloc(pool, sizeof(*bundle));

	char location[] = "location";
	char inputFile[] = "inputFile";
//	celix_status_t status = bundle_revise(bundle, location, inputFile);
}

TEST(bundle, isLockable) {
	bundle_pt bundle = (bundle_pt) apr_palloc(pool, sizeof(*bundle));
	apr_thread_mutex_create(&bundle->lock, APR_THREAD_MUTEX_UNNESTED, pool);

	bool lockable = false;
	celix_status_t status = bundle_isLockable(bundle, &lockable);
//	FAIL("Test not fully implemented");
}

TEST(bundle, getLockingThread) {
//	FAIL("Test not fully implemented");
}

TEST(bundle, lock) {
//	FAIL("Test not fully implemented");
}

TEST(bundle, unlock) {
//	FAIL("Test not fully implemented");
}

TEST(bundle, close) {
//	FAIL("Test not fully implemented");
}

TEST(bundle, closeAndDelete) {
//	FAIL("Test not fully implemented");
}

TEST(bundle, closeModules) {
//	FAIL("Test not fully implemented");
}

TEST(bundle, refresh) {
//	FAIL("Test not fully implemented");
}

TEST(bundle, getBundleId) {
//	FAIL("Test not fully implemented");
}

TEST(bundle, getRegisteredServices) {
//	FAIL("Test not fully implemented");
}

TEST(bundle, getServicesInUse) {
//	FAIL("Test not fully implemented");
}

TEST(bundle, getMemoryPool) {
//	FAIL("Test not fully implemented");
}

TEST(bundle, setFramework) {
//	FAIL("Test not fully implemented");
}

TEST(bundle, getFramework) {
//	FAIL("Test not fully implemented");
}
