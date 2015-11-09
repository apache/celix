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
 * bundle.h
 *
 *  \date       Mar 23, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef BUNDLE_H_
#define BUNDLE_H_

typedef struct bundle * bundle_pt;

#include "celix_errno.h"
#include "bundle_state.h"
#include "bundle_archive.h"
#include "framework.h"
#include "wire.h"
#include "module.h"
#include "service_reference.h"
#include "bundle_context.h"
#include "celix_log.h"
#include "celix_threads.h"

FRAMEWORK_EXPORT celix_status_t bundle_create(bundle_pt * bundle);
FRAMEWORK_EXPORT celix_status_t bundle_createFromArchive(bundle_pt * bundle, framework_pt framework, bundle_archive_pt archive);
FRAMEWORK_EXPORT celix_status_t bundle_destroy(bundle_pt bundle);

FRAMEWORK_EXPORT celix_status_t bundle_isSystemBundle(bundle_pt bundle, bool *systemBundle);
FRAMEWORK_EXPORT celix_status_t bundle_getArchive(bundle_pt bundle, bundle_archive_pt *archive);
FRAMEWORK_EXPORT celix_status_t bundle_getCurrentModule(bundle_pt bundle, module_pt *module);
FRAMEWORK_EXPORT array_list_pt bundle_getModules(bundle_pt bundle);
FRAMEWORK_EXPORT void * bundle_getHandle(bundle_pt bundle);
FRAMEWORK_EXPORT void bundle_setHandle(bundle_pt bundle, void * handle);
FRAMEWORK_EXPORT activator_pt bundle_getActivator(bundle_pt bundle);
FRAMEWORK_EXPORT celix_status_t bundle_setActivator(bundle_pt bundle, activator_pt activator);
FRAMEWORK_EXPORT celix_status_t bundle_getContext(bundle_pt bundle, bundle_context_pt *context);
FRAMEWORK_EXPORT celix_status_t bundle_setContext(bundle_pt bundle, bundle_context_pt context);
FRAMEWORK_EXPORT celix_status_t bundle_getEntry(bundle_pt bundle, char * name, char **entry);

FRAMEWORK_EXPORT celix_status_t bundle_start(bundle_pt bundle);
FRAMEWORK_EXPORT celix_status_t bundle_startWithOptions(bundle_pt bundle, int options);
FRAMEWORK_EXPORT celix_status_t bundle_update(bundle_pt bundle, char *inputFile);
FRAMEWORK_EXPORT celix_status_t bundle_stop(bundle_pt bundle);
FRAMEWORK_EXPORT celix_status_t bundle_stopWithOptions(bundle_pt bundle, int options);
FRAMEWORK_EXPORT celix_status_t bundle_uninstall(bundle_pt bundle);

FRAMEWORK_EXPORT celix_status_t bundle_setState(bundle_pt bundle, bundle_state_e state);
FRAMEWORK_EXPORT celix_status_t bundle_setPersistentStateInactive(bundle_pt bundle);
FRAMEWORK_EXPORT celix_status_t bundle_setPersistentStateUninstalled(bundle_pt bundle);

FRAMEWORK_EXPORT void uninstallBundle(bundle_pt bundle);

FRAMEWORK_EXPORT celix_status_t bundle_revise(bundle_pt bundle, char * location, char *inputFile);
FRAMEWORK_EXPORT celix_status_t bundle_addModule(bundle_pt bundle, module_pt module);
FRAMEWORK_EXPORT celix_status_t bundle_closeModules(bundle_pt bundle);

// Service Reference Functions
FRAMEWORK_EXPORT array_list_pt getUsingBundles(service_reference_pt reference);

FRAMEWORK_EXPORT int compareTo(service_reference_pt a, service_reference_pt b);

FRAMEWORK_EXPORT celix_status_t bundle_getState(bundle_pt bundle, bundle_state_e *state);
FRAMEWORK_EXPORT celix_status_t bundle_isLockable(bundle_pt bundle, bool *lockable);
FRAMEWORK_EXPORT celix_status_t bundle_getLockingThread(bundle_pt bundle, celix_thread_t *thread);
FRAMEWORK_EXPORT celix_status_t bundle_lock(bundle_pt bundle, bool *locked);
FRAMEWORK_EXPORT celix_status_t bundle_unlock(bundle_pt bundle, bool *unlocked);

FRAMEWORK_EXPORT celix_status_t bundle_closeAndDelete(bundle_pt bundle);
FRAMEWORK_EXPORT celix_status_t bundle_close(bundle_pt bundle);

FRAMEWORK_EXPORT celix_status_t bundle_refresh(bundle_pt bundle);
FRAMEWORK_EXPORT celix_status_t bundle_getBundleId(bundle_pt bundle, long *id);

FRAMEWORK_EXPORT celix_status_t bundle_getRegisteredServices(bundle_pt bundle, array_list_pt *list);
FRAMEWORK_EXPORT celix_status_t bundle_getServicesInUse(bundle_pt bundle, array_list_pt *list);

FRAMEWORK_EXPORT celix_status_t bundle_setFramework(bundle_pt bundle, framework_pt framework);
FRAMEWORK_EXPORT celix_status_t bundle_getFramework(bundle_pt bundle, framework_pt *framework);

#endif /* BUNDLE_H_ */
