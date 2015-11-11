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
 * bundle_mock.c
 *
 *  \date       Feb 7, 2013
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include "CppUTestExt/MockSupport_c.h"

#include "bundle_private.h"

celix_status_t bundle_create(bundle_pt * bundle) {
	mock_c()->actualCall("bundle_create")
			->withOutputParameter("bundle", bundle);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundle_createFromArchive(bundle_pt * bundle, framework_pt framework, bundle_archive_pt archive) {
	mock_c()->actualCall("bundle_createFromArchive")
			->withPointerParameters("bundle", bundle)
			->withPointerParameters("framework", framework)
			->withPointerParameters("archive", archive);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundle_destroy(bundle_pt bundle) {
	mock_c()->actualCall("destroy");
	return mock_c()->returnValue().value.intValue;
}


celix_status_t bundle_isSystemBundle(bundle_pt bundle, bool *systemBundle) {
	mock_c()->actualCall("bundle_isSystembundle")
			->withPointerParameters("bundle", bundle)
			->withOutputParameter("systemBundle", systemBundle);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundle_getArchive(bundle_pt bundle, bundle_archive_pt *archive) {
	mock_c()->actualCall("bundle_getArchive");
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundle_getCurrentModule(bundle_pt bundle, module_pt *module) {
	mock_c()->actualCall("bundle_getCurrentModule")
		->withPointerParameters("bundle", bundle)
		->withOutputParameter("module", (void **) module);
	return mock_c()->returnValue().value.intValue;
}

array_list_pt bundle_getModules(bundle_pt bundle) {
	mock_c()->actualCall("bundle_getModules");
	return mock_c()->returnValue().value.pointerValue;
}

void * bundle_getHandle(bundle_pt bundle) {
	mock_c()->actualCall("bundle_getHandle");
	return mock_c()->returnValue().value.pointerValue;
}

void bundle_setHandle(bundle_pt bundle, void * handle) {
	mock_c()->actualCall("bundle_setHandle");
}

activator_pt bundle_getActivator(bundle_pt bundle) {
	mock_c()->actualCall("bundle_getActivator");
	return mock_c()->returnValue().value.pointerValue;
}

celix_status_t bundle_setActivator(bundle_pt bundle, activator_pt activator) {
	mock_c()->actualCall("bundle_setActivator");
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundle_getContext(bundle_pt bundle, bundle_context_pt *context) {
	mock_c()->actualCall("bundle_getContext")
			->withPointerParameters("bundle", bundle)
			->withOutputParameter("context", context);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundle_setContext(bundle_pt bundle, bundle_context_pt context) {
	mock_c()->actualCall("bundle_setContext");
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundle_getEntry(bundle_pt bundle, char * name, char **entry) {
	mock_c()->actualCall("bundle_getEntry")
			->withPointerParameters("bundle", bundle)
			->withStringParameters("name", name)
			->withOutputParameter("entry", entry);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundle_start(bundle_pt bundle) {
	mock_c()->actualCall("bundle_start");
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundle_startWithOptions(bundle_pt bundle, int options) {
	mock_c()->actualCall("bundle_startWithOptions");
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundle_update(bundle_pt bundle, char *inputFile) {
	mock_c()->actualCall("bundle_update");
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundle_stop(bundle_pt bundle) {
	mock_c()->actualCall("bundle_stop");
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundle_stopWithOptions(bundle_pt bundle, int options) {
	mock_c()->actualCall("bundle_stopWithOptions");
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundle_uninstall(bundle_pt bundle) {
	mock_c()->actualCall("bundle_uninstall");
	return mock_c()->returnValue().value.intValue;
}


celix_status_t bundle_setState(bundle_pt bundle, bundle_state_e state) {
	mock_c()->actualCall("bundle_setState");
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundle_setPersistentStateInactive(bundle_pt bundle) {
	mock_c()->actualCall("bundle_setPersistentStateInactive");
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundle_setPersistentStateUninstalled(bundle_pt bundle) {
	mock_c()->actualCall("bundle_setPersistentStateUninstalled");
	return mock_c()->returnValue().value.intValue;
}


void uninstallBundle(bundle_pt bundle) {
	mock_c()->actualCall("uninstallBundle");
}


celix_status_t bundle_revise(bundle_pt bundle, char * location, char *inputFile) {
	mock_c()->actualCall("bundle_revise");
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundle_addModule(bundle_pt bundle, module_pt module) {
	mock_c()->actualCall("bundle_addModule");
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundle_closeModules(bundle_pt bundle) {
	mock_c()->actualCall("bundle_closeModules");
	return mock_c()->returnValue().value.intValue;
}


// Service Reference Functions
array_list_pt getUsingBundles(service_reference_pt reference) {
	mock_c()->actualCall("getUsingBundles");
	return mock_c()->returnValue().value.pointerValue;
}


int compareTo(service_reference_pt a, service_reference_pt b) {
	mock_c()->actualCall("service_reference_pt");
	return mock_c()->returnValue().value.intValue;
}


celix_status_t bundle_getState(bundle_pt bundle, bundle_state_e *state) {
	mock_c()->actualCall("bundle_getState")
			->withPointerParameters("bundle", bundle)
			->withOutputParameter("state", state);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundle_isLockable(bundle_pt bundle, bool *lockable) {
	mock_c()->actualCall("bundle_isLockable")
			->withPointerParameters("bundle", bundle)
			->withOutputParameter("lockable", lockable);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundle_getLockingThread(bundle_pt bundle, celix_thread_t *thread) {
	mock_c()->actualCall("bundle_getLockingThread")
			->withPointerParameters("bundle", bundle)
			->withOutputParameter("thread", thread);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundle_lock(bundle_pt bundle, bool *locked) {
	mock_c()->actualCall("bundle_lock")
			->withPointerParameters("bundle", bundle)
			->withOutputParameter("locked", locked);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundle_unlock(bundle_pt bundle, bool *unlocked) {
	mock_c()->actualCall("bundle_unlock")
			->withPointerParameters("bundle", bundle)
			->withOutputParameter("unlocked", unlocked);
	return mock_c()->returnValue().value.intValue;
}


celix_status_t bundle_closeAndDelete(bundle_pt bundle) {
	mock_c()->actualCall("bundle_closeAndDelete");
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundle_close(bundle_pt bundle) {
	mock_c()->actualCall("bundle_close");
	return mock_c()->returnValue().value.intValue;
}


celix_status_t bundle_refresh(bundle_pt bundle) {
	mock_c()->actualCall("bundle_refresh");
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundle_getBundleId(bundle_pt bundle, long *id) {
	mock_c()->actualCall("bundle_getBundleId")
			->withPointerParameters("bundle", bundle)
			->withOutputParameter("id", id);
	return mock_c()->returnValue().value.intValue;
}


celix_status_t bundle_getRegisteredServices(bundle_pt bundle, array_list_pt *list) {
	mock_c()->actualCall("bundle_getRegisteredServices")
			->withPointerParameters("bundle", bundle)
			->withOutputParameter("list", list);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundle_getServicesInUse(bundle_pt bundle, array_list_pt *list) {
	mock_c()->actualCall("bundle_getServicesInUse")
			->withPointerParameters("bundle", bundle)
			->withOutputParameter("list", list);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundle_setFramework(bundle_pt bundle, framework_pt framework) {
	mock_c()->actualCall("bundle_setFramework");
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundle_getFramework(bundle_pt bundle, framework_pt *framework) {
	mock_c()->actualCall("bundle_getFramework")
			->withPointerParameters("bundle", bundle)
			->withOutputParameter("framework", framework);
	return mock_c()->returnValue().value.intValue;
}



