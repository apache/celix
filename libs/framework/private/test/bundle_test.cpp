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
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdio.h>

#include "CppUTest/TestHarness.h"
#include "CppUTest/TestHarness_c.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTestExt/MockSupport.h"
#include "string.h"

extern "C" {
#include "bundle_revision_private.h"
#include "bundle_private.h"
#include "utils.h"
#include "celix_log.h"

framework_logger_pt logger = (framework_logger_pt) 0x42;
}

int main(int argc, char** argv) {
	return RUN_ALL_TESTS(argc, argv);
}

TEST_GROUP(bundle) {
	void setup(void) {
	}

	void teardown(void) {
		mock().checkExpectations();
		mock().clear();
	}
};

static char* my_strdup(const char* s) {
	if (s == NULL) {
		return NULL;
	}

	size_t len = strlen(s);

	char *d = (char*) calloc(len + 1, sizeof(char));

	if (d == NULL) {
		return NULL;
	}

	strncpy(d, s, len);
	return d;
}


TEST(bundle, create) {
	bundle_archive_pt archive = (bundle_archive_pt) 0x10;
	mock().expectOneCall("bundleArchive_createSystemBundleArchive")
							.withOutputParameterReturning("bundle_archive", &archive, sizeof(archive))
							.andReturnValue(CELIX_SUCCESS);

	module_pt module = (module_pt) 0x20;
	mock().expectOneCall("module_createFrameworkModule")
							.ignoreOtherParameters()
							.andReturnValue(module);

	mock().expectOneCall("resolver_addModule")
							.withParameter("module", module);

	bundle_pt actual = NULL;
	celix_status_t status = bundle_create(&actual);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(NULL, actual->context);
	POINTERS_EQUAL(NULL, actual->activator);
	LONGS_EQUAL(OSGI_FRAMEWORK_BUNDLE_INSTALLED, actual->state);
	POINTERS_EQUAL(NULL, actual->handle);
	POINTERS_EQUAL(archive, actual->archive);
	CHECK(actual->modules);
	POINTERS_EQUAL(NULL, actual->manifest);
	//	CHECK(actual->lock)
	LONGS_EQUAL(0, actual->lockCount);
	POINTERS_EQUAL(NULL, actual->lockThread.thread);
	POINTERS_EQUAL(NULL, actual->framework);

	mock().expectOneCall("module_destroy");
	bundle_destroy(actual);
}

TEST(bundle, createFromArchive) {
	framework_pt framework = (framework_pt) 0x10;
	bundle_archive_pt archive = (bundle_archive_pt) 0x20;
	bundle_revision_pt revision = (bundle_revision_pt) 0x21;
	manifest_pt manifest = (manifest_pt) 0x30;

	mock().expectOneCall("bundleArchive_getCurrentRevision")
        				.withParameter("archive", archive)
        				.withOutputParameterReturning("revision", &revision, sizeof(revision))
        				.andReturnValue(CELIX_SUCCESS);

	mock().expectOneCall("bundleRevision_getManifest")
						.withParameter("revision", revision)
						.withOutputParameterReturning("manifest", &manifest, sizeof(manifest))
						.andReturnValue(CELIX_SUCCESS);

	long id = 1;
	mock().expectOneCall("bundleArchive_getId")
						.withParameter("archive", archive)
						.withOutputParameterReturning("id", &id, sizeof(id))
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
						.withOutputParameterReturning("symbolicName", &symbolicName, sizeof(symbolicName))
						.andReturnValue(CELIX_SUCCESS);

	array_list_pt bundles = NULL;
	arrayList_create(&bundles);
	mock().expectOneCall("framework_getBundles")
						.withParameter("framework", framework)
						.andReturnValue(bundles);

	mock().expectOneCall("resolver_addModule")
						.withParameter("module", module);

	bundle_pt actual = NULL;
	celix_status_t status = bundle_createFromArchive(&actual, framework, archive);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(NULL, actual->context);
	POINTERS_EQUAL(NULL, actual->activator);
	LONGS_EQUAL(OSGI_FRAMEWORK_BUNDLE_INSTALLED, actual->state);
	POINTERS_EQUAL(NULL, actual->handle);
	POINTERS_EQUAL(archive, actual->archive);
	CHECK(actual->modules);
	POINTERS_EQUAL(NULL, actual->manifest);
	//	CHECK(actual->lock)
	LONGS_EQUAL(0, actual->lockCount);
	POINTERS_EQUAL(NULL, actual->lockThread.thread);
	POINTERS_EQUAL(framework, actual->framework);

	arrayList_destroy(actual->modules);
	free(actual);


	mock().clear();
}

TEST(bundle, getArchive) {
	bundle_pt bundle = (bundle_pt) malloc(sizeof(*bundle));
	bundle_archive_pt archive = (bundle_archive_pt) 0x10;
	bundle->archive = archive;

	bundle_archive_pt actual = NULL;
	LONGS_EQUAL(CELIX_SUCCESS, bundle_getArchive(bundle, &actual));
	POINTERS_EQUAL(archive, actual);

	mock().expectOneCall("framework_logCode")
							.withParameter("code", CELIX_ILLEGAL_ARGUMENT);

	actual = NULL;
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, bundle_getArchive(NULL, &actual));

	free(bundle);


	mock().clear();
}

TEST(bundle, getCurrentModule) {
	bundle_pt bundle = (bundle_pt) malloc(sizeof(*bundle));
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

	arrayList_destroy(bundle->modules);
	free(bundle);
}

TEST(bundle, getModules) {
	bundle_pt bundle = (bundle_pt) malloc(sizeof(*bundle));
	array_list_pt modules = (array_list_pt) 0x10;
	bundle->modules = modules;

	array_list_pt actual = bundle_getModules(bundle);
	POINTERS_EQUAL(modules, actual);

	free(bundle);
}

TEST(bundle, getHandle) {
	bundle_pt bundle = (bundle_pt) malloc(sizeof(*bundle));
	void *expected = (void *) 0x10;
	bundle->handle = expected;

	void *actual = bundle_getHandle(bundle);
	POINTERS_EQUAL(expected, actual);

	free(bundle);
}

TEST(bundle, setHandle) {
	bundle_pt bundle = (bundle_pt) malloc(sizeof(*bundle));
	void *expected = (void *) 0x10;

	bundle_setHandle(bundle, expected);
	POINTERS_EQUAL(expected, bundle->handle);

	free(bundle);
}

TEST(bundle, getActivator) {
	bundle_pt bundle = (bundle_pt) malloc(sizeof(*bundle));
	celix_bundle_activator_t *expected = (celix_bundle_activator_t*) 0x10;
	bundle->activator = expected;

	celix_bundle_activator_t *actual = bundle_getActivator(bundle);
	POINTERS_EQUAL(expected, actual);

	free(bundle);
}

TEST(bundle, setActivator) {
	bundle_pt bundle = (bundle_pt) malloc(sizeof(*bundle));
	celix_bundle_activator_t *expected = (celix_bundle_activator_t*) 0x10;

	celix_status_t status = bundle_setActivator(bundle, expected);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(expected, bundle->activator);

	free(bundle);
}

TEST(bundle, getContext) {
	bundle_pt bundle = (bundle_pt) malloc(sizeof(*bundle));
	bundle_context_pt expected = (bundle_context_pt) 0x10;
	bundle->context = expected;

	bundle_context_pt actual = NULL;
	celix_status_t status = bundle_getContext(bundle, &actual);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(expected, actual);

	free(bundle);
}

TEST(bundle, setContext) {
	bundle_pt bundle = (bundle_pt) malloc(sizeof(*bundle));
	bundle_context_pt expected = (bundle_context_pt) 0x10;

	celix_status_t status = bundle_setContext(bundle, expected);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(expected, bundle->context);

	free(bundle);
}

TEST(bundle, getEntry) {
	bundle_pt bundle = (bundle_pt) malloc(sizeof(*bundle));
	framework_pt framework = (framework_pt) 0x10;
	bundle->framework = framework;

	char name[] = "name";
	char *expected = (char *) 0x20;
	mock().expectOneCall("framework_getBundleEntry")
						.withParameter("framework", framework)
						.withParameter("bundle", bundle)
						.withParameter("name", name)
						.withOutputParameterReturning("entry", &expected, sizeof(expected))
						.andReturnValue(CELIX_SUCCESS);

	char *actual = NULL;
	celix_status_t status = bundle_getEntry(bundle, name, &actual);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(expected, actual);

    //free(actual);
	free(bundle);
}

TEST(bundle, getState) {
	bundle_pt bundle = (bundle_pt) malloc(sizeof(*bundle));
	bundle->state = OSGI_FRAMEWORK_BUNDLE_ACTIVE;

	bundle_state_e actual = OSGI_FRAMEWORK_BUNDLE_UNKNOWN;
	LONGS_EQUAL(CELIX_SUCCESS, bundle_getState(bundle, &actual));
	POINTERS_EQUAL(OSGI_FRAMEWORK_BUNDLE_ACTIVE, actual);

	//get unknown bundle's state
	LONGS_EQUAL(CELIX_BUNDLE_EXCEPTION, bundle_getState((bundle_pt)NULL, &actual));
	POINTERS_EQUAL(OSGI_FRAMEWORK_BUNDLE_UNKNOWN, actual);

	free(bundle);
}

TEST(bundle, setState) {
	bundle_pt bundle = (bundle_pt) malloc(sizeof(*bundle));
	bundle->state = OSGI_FRAMEWORK_BUNDLE_UNKNOWN;

	celix_status_t status = bundle_setState(bundle, OSGI_FRAMEWORK_BUNDLE_INSTALLED);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(OSGI_FRAMEWORK_BUNDLE_INSTALLED, bundle->state);

	free(bundle);
}

/*//declared here, since its non-static, but not actually exported by bundle.h or bundle_private.h
extern celix_status_t bundle_createModule(bundle_pt bundle, module_pt *module);

TEST(bundle, createModule){
	bundle_pt bundle = (bundle_pt) malloc(sizeof(*bundle));

	module_pt module = (module_pt) 0x01;
	module_pt module2 = (module_pt) 0x02;
	module_pt module3 = (module_pt) 0x03;
	arrayList_create(&bundle->modules);
	arrayList_add(bundle->modules, module);
	arrayList_add(bundle->modules, module2);
	arrayList_add(bundle->modules, module3);

	framework_pt framework = (framework_pt) 0x04;
	bundle->framework = framework;

	bundle_archive_pt archive = (bundle_archive_pt) 0x05;
	bundle->archive = archive;

	module_pt module_new = (module_pt) 0x06;
	bundle_revision_pt revision = (bundle_revision_pt) 0x07;
	version_pt version = (version_pt) 0x08;
	manifest_pt manifest = (manifest_pt) 0x09;

	long id = 666;
	char * symbolicName = my_strdup("name");

	bundle_pt bundle2 = (bundle_pt) malloc(sizeof(*bundle));
	module_pt module4 = (module_pt) 0x0A;
	arrayList_create(&bundle2->modules);
	arrayList_add(bundle2->modules, module4);
	bundle2->framework = framework;

	bundle_archive_pt archive2 = (bundle_archive_pt) 0x0B;
	bundle2->archive = archive2;

	version_pt version2 = (version_pt) 0x0C;
	long id2 = 667;

	array_list_pt bundles;
	arrayList_create(&bundles);
	arrayList_add(bundles, bundle2);

	//expected calls from bundle_createModule
	mock().expectOneCall("bundleArchive_getCurrentRevision")
			.withParameter("archive", archive)
        	.withOutputParameterReturning("revision", &revision, sizeof(revision));

	mock().expectOneCall("bundleRevision_getManifest")
			.withParameter("revision", revision)
			.withOutputParameterReturning("manifest", &manifest, sizeof(manifest));

	mock().expectOneCall("bundleArchive_getId")
			.withParameter("archive", archive)
			.withOutputParameterReturning("id", &id, sizeof(id));

	mock().expectOneCall("module_create")
			.withParameter("headerMap", manifest)
			.withParameter("moduleId", "666.0")
			.withParameter("bundle", bundle)
			.andReturnValue(module_new);

	mock().expectOneCall("module_getVersion")
			.withParameter("module", module_new)
			.andReturnValue(version);

	mock().expectOneCall("module_getSymbolicName")
			.withParameter("module", module_new)
			.withOutputParameterReturning("symbolicName", &symbolicName, sizeof(char*));

	mock().expectOneCall("framework_getBundles")
			.withParameter("framework", framework)
			.andReturnValue(bundles);

	mock().expectOneCall("bundleArchive_getId")
			.withParameter("archive", archive2)
			.withOutputParameterReturning("id", &id2, sizeof(id2));

	//returning same symbolic name for module_new as for module4
	mock().expectOneCall("module_getSymbolicName")
			.withParameter("module", module4)
			.withOutputParameterReturning("symbolicName", &symbolicName, sizeof(char*));

	//returning different version for module_new as for module4
	mock().expectOneCall("module_getVersion")
			.withParameter("module", module4)
			.andReturnValue(version2);

	int result = 1; //1 means not equal
	mock().expectOneCall("version_compareTo")
			.withParameter("version", version)
			.withParameter("compare", version2)
			.withOutputParameterReturning("result", &result,sizeof(result));

	LONGS_EQUAL(CELIX_SUCCESS, bundle_createModule(bundle, &module_new));

	arrayList_destroy(bundle->modules);
	arrayList_destroy(bundle2->modules);

	free(bundle);
	free(bundle2);
	free(symbolicName);
}*/

TEST(bundle, start) {
	bundle_pt bundle = (bundle_pt) malloc(sizeof(*bundle));
	framework_pt framework = (framework_pt) 0x10;
	bundle_archive_pt archive = (bundle_archive_pt) 0x20;
	bundle->framework = framework;
	bundle->archive = archive;

	int options = 42;

	long id = 1;
	mock().expectNCalls(2, "bundleArchive_getId")
        				.withParameter("archive", archive)
        				.withOutputParameterReturning("id", &id, sizeof(id))
        				.andReturnValue(CELIX_SUCCESS);

	mock().expectOneCall("fw_startBundle")
						.withParameter("framework", framework)
						.withParameter("bundle", bundle)
						.withParameter("options", options)
						.andReturnValue(CELIX_SUCCESS);

	celix_status_t status = bundle_startWithOptions(bundle, options);
	LONGS_EQUAL(CELIX_SUCCESS, status);

	mock().expectOneCall("fw_startBundle")
						.withParameter("framework", framework)
						.withParameter("bundle", bundle)
						.withParameter("options", 0)
						.andReturnValue(CELIX_SUCCESS);

	status = bundle_start(bundle);
	LONGS_EQUAL(CELIX_SUCCESS, status);

	free(bundle);
}

TEST(bundle, update) {
	bundle_pt bundle = (bundle_pt) malloc(sizeof(*bundle));
	framework_pt framework = (framework_pt) 0x10;
	bundle_archive_pt archive = (bundle_archive_pt) 0x20;
	bundle->framework = framework;
	bundle->archive = archive;

	long id = 1;
	mock().expectOneCall("bundleArchive_getId")
        				.withParameter("archive", archive)
        				.withOutputParameterReturning("id", &id, sizeof(id))
        				.andReturnValue(CELIX_SUCCESS);

	char input[] = "input";
	mock().expectOneCall("framework_updateBundle")
						.withParameter("framework", framework)
						.withParameter("bundle", bundle)
						.withParameter("inputFile", input)
						.andReturnValue(CELIX_SUCCESS);

	celix_status_t status = bundle_update(bundle, input);
	LONGS_EQUAL(CELIX_SUCCESS, status);

	free(bundle);
}

TEST(bundle, stop) {
	bundle_pt bundle = (bundle_pt) malloc(sizeof(*bundle));
	framework_pt framework = (framework_pt) 0x10;
	bundle_archive_pt archive = (bundle_archive_pt) 0x20;
	bundle->framework = framework;
	bundle->archive = archive;

	long id = 1;
	mock().expectNCalls(2, "bundleArchive_getId")
	        				.withParameter("archive", archive)
	        				.withOutputParameterReturning("id", &id, sizeof(id))
	        				.andReturnValue(CELIX_SUCCESS);

	int options = 1;
	mock().expectOneCall("fw_stopBundle")
						.withParameter("framework", framework)
						.withParameter("bundle", bundle)
						.withParameter("record", 1)
						.andReturnValue(CELIX_SUCCESS);

	celix_status_t status = bundle_stopWithOptions(bundle, options);
	LONGS_EQUAL(CELIX_SUCCESS, status);

	mock().expectOneCall("fw_stopBundle")
						.withParameter("framework", framework)
						.withParameter("bundle", bundle)
						.withParameter("record", 0)
						.andReturnValue(CELIX_SUCCESS);

	status = bundle_stop(bundle);
	LONGS_EQUAL(CELIX_SUCCESS, status);

	free(bundle);
}

TEST(bundle, uninstall) {
	bundle_pt bundle = (bundle_pt) malloc(sizeof(*bundle));
	framework_pt framework = (framework_pt) 0x10;
	bundle_archive_pt archive = (bundle_archive_pt) 0x20;
	bundle->framework = framework;
	bundle->archive = archive;

	long id = 666;
	mock().expectOneCall("bundleArchive_getId")
        				.withParameter("archive", archive)
        				.withOutputParameterReturning("id", &id, sizeof(id))
        				.andReturnValue(CELIX_SUCCESS);

	mock().expectOneCall("fw_uninstallBundle")
		.withParameter("framework", framework)
		.withParameter("bundle", bundle)
		.andReturnValue(CELIX_SUCCESS);

	LONGS_EQUAL(CELIX_SUCCESS, bundle_uninstall(bundle));

	//attempt to uninstall framework bundle
	id = 0;
	mock().expectOneCall("bundleArchive_getId")
        				.withParameter("archive", archive)
        				.withOutputParameterReturning("id", &id, sizeof(id))
        				.andReturnValue(CELIX_SUCCESS);

	mock().expectOneCall("framework_logCode")
							.withParameter("code", CELIX_BUNDLE_EXCEPTION);

	LONGS_EQUAL(CELIX_BUNDLE_EXCEPTION, bundle_uninstall(bundle));

	free(bundle);
}

TEST(bundle, setPersistentStateInactive) {
	bundle_pt bundle = (bundle_pt) malloc(sizeof(*bundle));
	bundle_archive_pt archive = (bundle_archive_pt) 0x10;
	bundle->archive = archive;

	long id = 1;
	mock().expectOneCall("bundleArchive_getId")
						.withParameter("archive", archive)
						.withOutputParameterReturning("id", &id, sizeof(id))
						.andReturnValue(CELIX_SUCCESS);

	mock().expectOneCall("bundleArchive_setPersistentState")
						.withParameter("archive", archive)
						.withParameter("state", OSGI_FRAMEWORK_BUNDLE_INSTALLED)
						.andReturnValue(CELIX_SUCCESS);

	LONGS_EQUAL(CELIX_SUCCESS, bundle_setPersistentStateInactive(bundle));

	free(bundle);
}

TEST(bundle, setPersistentStateUninstalled) {
	bundle_pt bundle = (bundle_pt) malloc(sizeof(*bundle));
	bundle_archive_pt archive = (bundle_archive_pt) 0x10;
	bundle->archive = archive;

	long id = 1;
	mock().expectOneCall("bundleArchive_getId")
						.withParameter("archive", archive)
						.withOutputParameterReturning("id", &id, sizeof(id))
						.andReturnValue(CELIX_SUCCESS);

	mock().expectOneCall("bundleArchive_setPersistentState")
						.withParameter("archive", archive)
						.withParameter("state", OSGI_FRAMEWORK_BUNDLE_UNINSTALLED)
						.andReturnValue(CELIX_SUCCESS);

	celix_status_t status = bundle_setPersistentStateUninstalled(bundle);
	LONGS_EQUAL(CELIX_SUCCESS, status);

	free(bundle);
}

TEST(bundle, revise) {
	bundle_pt bundle = (bundle_pt) malloc(sizeof(*bundle));
	arrayList_create(&bundle->modules);
	bundle_archive_pt actual_archive = (bundle_archive_pt) 0x02;
	bundle_revision_pt actual_revision = (bundle_revision_pt) malloc(sizeof(*actual_revision));
	manifest_pt actual_manifest = (manifest_pt) malloc(sizeof(*actual_manifest));
	int actual_id = 666;
	const char * actual_module_id = "666.0";
	bundle->archive = actual_archive;
	char * symbolic_name = NULL;
	char location[] = "location";
	char inputFile[] = "inputFile";

	mock().expectNCalls(2, "bundleArchive_revise")
			.withParameter("archive", actual_archive)
			.withParameter("location", location)
			.withParameter("inputFile", inputFile);

	mock().expectNCalls(2, "bundleArchive_getCurrentRevision")
			.withParameter("archive", bundle->archive)
			.withOutputParameterReturning("revision", &actual_revision, sizeof(actual_revision));

	mock().expectNCalls(2, "bundleRevision_getManifest")
			.withParameter("revision", actual_revision)
			.withOutputParameterReturning("manifest", &actual_manifest, sizeof(actual_manifest));

	mock().expectNCalls(2, "bundleArchive_getId")
			.withParameter("archive", actual_archive)
			.withOutputParameterReturning("id", &actual_id, sizeof(actual_id));

	mock().expectOneCall("module_create")
			.withParameter("headerMap", actual_manifest)
			.withParameter("moduleId", actual_module_id)
			.withParameter("bundle", bundle)
			.andReturnValue((void*)NULL);

	//module create returns NULL during test
	mock().expectOneCall("resolver_addModule")
			.withParameter("module", (void*)NULL);

	LONGS_EQUAL(CELIX_SUCCESS, bundle_revise(bundle, location, inputFile));

	mock().expectOneCall("module_create")
			.withParameter("headerMap", actual_manifest)
			.withParameter("moduleId", actual_module_id)
			.withParameter("bundle", bundle)
			.andReturnValue((void*)0x01);

	mock().expectOneCall("module_getVersion")
			.withParameter("module", (void*)0x01);

	mock().expectOneCall("module_getSymbolicName")
			.withParameter("module", (void*)0x01)
			.withOutputParameterReturning("symbolicName", &symbolic_name,sizeof(symbolic_name))
			.andReturnValue(CELIX_ILLEGAL_ARGUMENT);

	bool rolledBack = true;
	mock().expectOneCall("bundleArchive_rollbackRevise")
			.withParameter("archive", actual_archive)
			.withOutputParameterReturning("rolledback", &rolledBack, sizeof(rolledBack));

	mock().expectOneCall("framework_logCode")
			.withParameter("code", CELIX_ILLEGAL_ARGUMENT);

	mock().expectOneCall("framework_logCode")
			.withParameter("code", CELIX_BUNDLE_EXCEPTION);

	LONGS_EQUAL(CELIX_BUNDLE_EXCEPTION, bundle_revise(bundle, location, inputFile));

	arrayList_destroy(bundle->modules);
	free(bundle);
	free(actual_revision);
	free(actual_manifest);
}

TEST(bundle, isLockable) {
	bundle_pt bundle = (bundle_pt) malloc(sizeof(*bundle));
	celixThreadMutex_create(&bundle->lock, NULL);
	bundle->lockCount = 0;

	bool lockable = false;
	celix_status_t status = bundle_isLockable(bundle, &lockable);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	//	FAIL("Test not fully implemented");

	free(bundle);
}


TEST(bundle, lockingThread) {
	bundle_pt bundle = (bundle_pt) malloc(sizeof(*bundle));
	celixThreadMutex_create(&bundle->lock, NULL);
	celix_thread_t thread;

	bundle->lockCount = 0;

	bool locked = false;
	LONGS_EQUAL(CELIX_SUCCESS, bundle_lock(bundle, &locked));
	CHECK(locked);
	LONGS_EQUAL(1, bundle->lockCount);

	LONGS_EQUAL(CELIX_SUCCESS, bundle_getLockingThread(bundle, &thread));
	bool equals;
	thread_equalsSelf(thread, &equals);
	CHECK(equals);

	bool unlocked;
	bundle->lockCount = 1;
	LONGS_EQUAL(CELIX_SUCCESS, bundle_unlock(bundle, &unlocked));
	CHECK(unlocked);
	LONGS_EQUAL(0, bundle->lockCount);

	//try to unlock unlocked lock
	LONGS_EQUAL(CELIX_SUCCESS, bundle_unlock(bundle, &unlocked));
	CHECK_FALSE(unlocked);
	LONGS_EQUAL(0, bundle->lockCount);

	celixThreadMutex_destroy(&bundle->lock);
	free(bundle);
}

TEST(bundle, close) {
	bundle_pt bundle = (bundle_pt) malloc(sizeof(*bundle));

	module_pt module = (module_pt) 0x01;
	module_pt module2 = (module_pt) 0x02;
	module_pt module3 = (module_pt) 0x03;
	arrayList_create(&bundle->modules);
	arrayList_add(bundle->modules, module);
	arrayList_add(bundle->modules, module2);
	arrayList_add(bundle->modules, module3);

	bundle_archive_pt archive = (bundle_archive_pt) 0x05;
	bundle->archive = archive;

	mock().expectOneCall("resolver_removeModule")
			.withParameter("module", module);

	mock().expectOneCall("resolver_removeModule")
			.withParameter("module", module2);

	mock().expectOneCall("resolver_removeModule")
			.withParameter("module", module3);

	mock().expectNCalls(3, "module_setWires");

	mock().expectOneCall("bundleArchive_close")
			.withParameter("archive", archive);

	LONGS_EQUAL(CELIX_SUCCESS, bundle_close(bundle));

	arrayList_destroy(bundle->modules);
	free(bundle);
}

TEST(bundle, closeAndDelete) {
	bundle_archive_pt archive = (bundle_archive_pt) 0x10;
	mock().expectOneCall("bundleArchive_createSystemBundleArchive")
								.withOutputParameterReturning("bundle_archive", &archive, sizeof(archive))
								.andReturnValue(CELIX_SUCCESS);

	module_pt module = (module_pt) 0x20;
	mock().expectOneCall("module_createFrameworkModule")
								.ignoreOtherParameters()
								.andReturnValue(module);

	mock().expectOneCall("resolver_addModule")
								.withParameter("module", module);

	bundle_pt actual = NULL;
	celix_status_t status = bundle_create(&actual);
	LONGS_EQUAL(CELIX_SUCCESS, status);

	mock().expectOneCall("resolver_removeModule")
			.withParameter("module", module);

	mock().expectOneCall("module_setWires");

	mock().expectOneCall("bundleArchive_closeAndDelete");

	status = bundle_closeAndDelete(actual);
	LONGS_EQUAL(CELIX_SUCCESS, status);

	arrayList_destroy(actual->modules);
	free(actual);
}

TEST(bundle, closeModules) {
	bundle_pt bundle = (bundle_pt) malloc(sizeof(*bundle));
	module_pt module = (module_pt) 0x01;
	module_pt module2 = (module_pt) 0x02;
	module_pt module3 = (module_pt) 0x03;

	arrayList_create(&bundle->modules);
	arrayList_add(bundle->modules, module);
	arrayList_add(bundle->modules, module2);
	arrayList_add(bundle->modules, module3);

	mock().expectOneCall("resolver_removeModule")
			.withParameter("module", module);

	mock().expectOneCall("resolver_removeModule")
			.withParameter("module", module2);

	mock().expectOneCall("resolver_removeModule")
			.withParameter("module", module3);

	mock().expectNCalls(3, "module_setWires");

	bundle_closeModules(bundle);

	arrayList_destroy(bundle->modules);
	free(bundle);
}

TEST(bundle, refresh) {
	bundle_pt bundle = (bundle_pt) malloc(sizeof(*bundle));

	module_pt module = (module_pt) 0x01;
	module_pt module2 = (module_pt) 0x02;
	module_pt module3 = (module_pt) 0x03;
	arrayList_create(&bundle->modules);
	arrayList_add(bundle->modules, module);
	arrayList_add(bundle->modules, module2);
	arrayList_add(bundle->modules, module3);

	framework_pt framework = (framework_pt) 0x04;
	bundle->framework = framework;

	bundle_archive_pt archive = (bundle_archive_pt) 0x05;
	bundle->archive = archive;

	module_pt module_new = (module_pt) 0x06;
	bundle_revision_pt revision = (bundle_revision_pt) 0x07;
	version_pt version = (version_pt) 0x08;
	manifest_pt manifest = (manifest_pt) 0x09;

	long id = 666;
	char * symbolicName = my_strdup("name");

	bundle_pt bundle2 = (bundle_pt) malloc(sizeof(*bundle));
	module_pt module4 = (module_pt) 0x0A;
	arrayList_create(&bundle2->modules);
	arrayList_add(bundle2->modules, module4);
	bundle2->framework = framework;

	bundle_archive_pt archive2 = (bundle_archive_pt) 0x0B;
	bundle2->archive = archive2;

	version_pt version2 = (version_pt) 0x0C;
	long id2 = 667;

	array_list_pt bundles;
	arrayList_create(&bundles);
	arrayList_add(bundles, bundle2);

	//expected calls from bundle_refresh
	mock().expectOneCall("resolver_removeModule")
			.withParameter("module", module);

	mock().expectOneCall("resolver_removeModule")
			.withParameter("module", module2);

	mock().expectOneCall("resolver_removeModule")
			.withParameter("module", module3);

	mock().expectNCalls(3, "module_setWires");

	//expected calls from bundle_createModule
	mock().expectOneCall("bundleArchive_getCurrentRevision")
			.withParameter("archive", archive)
        	.withOutputParameterReturning("revision", &revision, sizeof(revision));

	mock().expectOneCall("bundleRevision_getManifest")
			.withParameter("revision", revision)
			.withOutputParameterReturning("manifest", &manifest, sizeof(manifest));

	mock().expectOneCall("bundleArchive_getId")
			.withParameter("archive", archive)
			.withOutputParameterReturning("id", &id, sizeof(id));

	mock().expectOneCall("module_create")
			.withParameter("headerMap", manifest)
			.withParameter("moduleId", "666.0")
			.withParameter("bundle", bundle)
			.andReturnValue(module_new);

	mock().expectOneCall("module_getVersion")
			.withParameter("module", module_new)
			.andReturnValue(version);

	mock().expectOneCall("module_getSymbolicName")
			.withParameter("module", module_new)
			.withOutputParameterReturning("symbolicName", &symbolicName, sizeof(char*));

	mock().expectOneCall("framework_getBundles")
			.withParameter("framework", framework)
			.andReturnValue(bundles);

	mock().expectOneCall("bundleArchive_getId")
			.withParameter("archive", archive2)
			.withOutputParameterReturning("id", &id2, sizeof(id2));

	//returning same symbolic name for module_new as for module4
	mock().expectOneCall("module_getSymbolicName")
			.withParameter("module", module4)
			.withOutputParameterReturning("symbolicName", &symbolicName, sizeof(char*));

	//returning different version for module_new as for module4
	mock().expectOneCall("module_getVersion")
			.withParameter("module", module4)
			.andReturnValue(version2);

	int result = 1; //1 means not equal
	mock().expectOneCall("version_compareTo")
			.withParameter("version", version)
			.withParameter("compare", version2)
			.withOutputParameterReturning("result", &result,sizeof(result));

	//expected calls from bundle_addModule
	mock().expectOneCall("resolver_addModule")
			.withParameter("module", module_new);

	LONGS_EQUAL(CELIX_SUCCESS, bundle_refresh(bundle));

	arrayList_destroy(bundle->modules);
	arrayList_destroy(bundle2->modules);

	free(bundle);
	free(bundle2);
	free(symbolicName);
}

TEST(bundle, getBundleId) {
	bundle_pt bundle = (bundle_pt) malloc(sizeof(*bundle));
	bundle_archive_pt actual_archive = (bundle_archive_pt) 0x42;
	bundle->archive = actual_archive;
	long actual_id = 666;
	long get_id = 0;

	mock().expectOneCall("bundleArchive_getId")
			.withParameter("archive", actual_archive)
			.withOutputParameterReturning("id", &actual_id, sizeof(actual_id));

	LONGS_EQUAL(CELIX_SUCCESS, bundle_getBundleId(bundle, &get_id));
	LONGS_EQUAL(actual_id, get_id);

	free(bundle);
}

TEST(bundle, getRegisteredServices) {
	bundle_pt bundle = (bundle_pt) malloc(sizeof(*bundle));
	framework_pt framework = (framework_pt) 0x10;
	bundle->framework = framework;
	array_list_pt list = (array_list_pt) 0x20;
	array_list_pt get = NULL;

	mock().expectOneCall("fw_getBundleRegisteredServices")
			.withParameter("framework", framework)
			.withParameter("bundle", bundle)
			.withOutputParameterReturning("services", &list, sizeof(list));

	LONGS_EQUAL(CELIX_SUCCESS, bundle_getRegisteredServices(bundle, &get));
	POINTERS_EQUAL(list, get);

	free(bundle);
}

TEST(bundle, getServicesInUse) {
	bundle_pt bundle = (bundle_pt) malloc(sizeof(*bundle));
	framework_pt framework = (framework_pt) 0x10;
	bundle->framework = framework;
	array_list_pt list = (array_list_pt) 0x20;
	array_list_pt get = NULL;

	mock().expectOneCall("fw_getBundleServicesInUse")
			.withParameter("framework", framework)
			.withParameter("bundle", bundle)
			.withOutputParameterReturning("services", &list, sizeof(list));

	LONGS_EQUAL(CELIX_SUCCESS, bundle_getServicesInUse(bundle, &get));
	POINTERS_EQUAL(list, get);

	free(bundle);
}

TEST(bundle, setFramework) {
	framework_pt framework = (framework_pt) 0x666;
	bundle_pt bundle = (bundle_pt) malloc(sizeof(*bundle));

	LONGS_EQUAL(CELIX_SUCCESS, bundle_setFramework(bundle, framework));
	POINTERS_EQUAL(framework, bundle->framework);

	mock().expectOneCall("framework_logCode")
							.withParameter("code", CELIX_ILLEGAL_ARGUMENT);

	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, bundle_setFramework(bundle, NULL));

	free(bundle);
}

TEST(bundle, getFramework) {
	framework_pt get = NULL;
	framework_pt actual = (framework_pt) 0x666;
	bundle_pt bundle = (bundle_pt) malloc(sizeof(*bundle));

	bundle->framework = actual;

	LONGS_EQUAL(CELIX_SUCCESS, bundle_getFramework(bundle, &get));

	free(bundle);
	bundle = NULL;

	mock().expectOneCall("framework_logCode")
							.withParameter("code", CELIX_ILLEGAL_ARGUMENT);

	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, bundle_getFramework(bundle, &get));

	free(bundle);
}
