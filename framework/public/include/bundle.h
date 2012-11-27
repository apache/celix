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
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef BUNDLE_H_
#define BUNDLE_H_

#include <apr_general.h>
#include <apr_portable.h>

typedef struct bundle * BUNDLE;

#include "celix_errno.h"
#include "bundle_state.h"
#include "bundle_archive.h"
#include "framework.h"
#include "service_reference.h"
#include "bundle_context.h"

FRAMEWORK_EXPORT celix_status_t bundle_create(BUNDLE * bundle, apr_pool_t *mp);
FRAMEWORK_EXPORT celix_status_t bundle_createFromArchive(BUNDLE * bundle, FRAMEWORK framework, bundle_archive_t archive, apr_pool_t *bundlePool);
FRAMEWORK_EXPORT celix_status_t bundle_destroy(BUNDLE bundle);

FRAMEWORK_EXPORT celix_status_t bundle_isSystemBundle(BUNDLE bundle, bool *systemBundle);
FRAMEWORK_EXPORT celix_status_t bundle_getArchive(BUNDLE bundle, bundle_archive_t *archive);
FRAMEWORK_EXPORT celix_status_t bundle_getCurrentModule(BUNDLE bundle, MODULE *module);
FRAMEWORK_EXPORT ARRAY_LIST bundle_getModules(BUNDLE bundle);
FRAMEWORK_EXPORT void * bundle_getHandle(BUNDLE bundle);
FRAMEWORK_EXPORT void bundle_setHandle(BUNDLE bundle, void * handle);
FRAMEWORK_EXPORT ACTIVATOR bundle_getActivator(BUNDLE bundle);
FRAMEWORK_EXPORT celix_status_t bundle_setActivator(BUNDLE bundle, ACTIVATOR activator);
FRAMEWORK_EXPORT celix_status_t bundle_getManifest(BUNDLE bundle, MANIFEST *manifest);
FRAMEWORK_EXPORT celix_status_t bundle_setManifest(BUNDLE bundle, MANIFEST manifest);
FRAMEWORK_EXPORT celix_status_t bundle_getContext(BUNDLE bundle, bundle_context_t *context);
FRAMEWORK_EXPORT celix_status_t bundle_setContext(BUNDLE bundle, bundle_context_t context);
FRAMEWORK_EXPORT celix_status_t bundle_getEntry(BUNDLE bundle, char * name, apr_pool_t *pool, char **entry);

FRAMEWORK_EXPORT celix_status_t bundle_start(BUNDLE bundle, int options);
FRAMEWORK_EXPORT celix_status_t bundle_update(BUNDLE bundle, char *inputFile);
FRAMEWORK_EXPORT celix_status_t bundle_stop(BUNDLE bundle, int options);
FRAMEWORK_EXPORT celix_status_t bundle_uninstall(BUNDLE bundle);

FRAMEWORK_EXPORT celix_status_t bundle_setState(BUNDLE bundle, BUNDLE_STATE state);
FRAMEWORK_EXPORT celix_status_t bundle_setPersistentStateInactive(BUNDLE bundle);
FRAMEWORK_EXPORT celix_status_t bundle_setPersistentStateUninstalled(BUNDLE bundle);

FRAMEWORK_EXPORT void uninstallBundle(BUNDLE bundle);

FRAMEWORK_EXPORT celix_status_t bundle_revise(BUNDLE bundle, char * location, char *inputFile);
FRAMEWORK_EXPORT celix_status_t bundle_addModule(BUNDLE bundle, MODULE module);
FRAMEWORK_EXPORT celix_status_t bundle_closeModules(BUNDLE bundle);

// Service Reference Functions
FRAMEWORK_EXPORT ARRAY_LIST getUsingBundles(SERVICE_REFERENCE reference);

FRAMEWORK_EXPORT int compareTo(SERVICE_REFERENCE a, SERVICE_REFERENCE b);

FRAMEWORK_EXPORT celix_status_t bundle_getState(BUNDLE bundle, BUNDLE_STATE *state);
FRAMEWORK_EXPORT celix_status_t bundle_isLockable(BUNDLE bundle, bool *lockable);
FRAMEWORK_EXPORT celix_status_t bundle_getLockingThread(BUNDLE bundle, apr_os_thread_t *thread);
FRAMEWORK_EXPORT celix_status_t bundle_lock(BUNDLE bundle, bool *locked);
FRAMEWORK_EXPORT celix_status_t bundle_unlock(BUNDLE bundle, bool *unlocked);

FRAMEWORK_EXPORT celix_status_t bundle_closeAndDelete(BUNDLE bundle);
FRAMEWORK_EXPORT celix_status_t bundle_close(BUNDLE bundle);

FRAMEWORK_EXPORT celix_status_t bundle_refresh(BUNDLE bundle);
FRAMEWORK_EXPORT celix_status_t bundle_getBundleId(BUNDLE bundle, long *id);

FRAMEWORK_EXPORT celix_status_t bundle_getRegisteredServices(BUNDLE bundle, apr_pool_t *pool, ARRAY_LIST *list);
FRAMEWORK_EXPORT celix_status_t bundle_getServicesInUse(BUNDLE bundle, ARRAY_LIST *list);

FRAMEWORK_EXPORT celix_status_t bundle_getMemoryPool(BUNDLE bundle, apr_pool_t **pool);

FRAMEWORK_EXPORT celix_status_t bundle_setFramework(BUNDLE bundle, FRAMEWORK framework);
FRAMEWORK_EXPORT celix_status_t bundle_getFramework(BUNDLE bundle, FRAMEWORK *framework);

#endif /* BUNDLE_H_ */
