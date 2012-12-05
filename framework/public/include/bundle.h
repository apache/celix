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

typedef struct bundle * bundle_t;

#include "celix_errno.h"
#include "bundle_state.h"
#include "bundle_archive.h"
#include "framework.h"
#include "service_reference.h"
#include "bundle_context.h"

FRAMEWORK_EXPORT celix_status_t bundle_create(bundle_t * bundle, apr_pool_t *mp);
FRAMEWORK_EXPORT celix_status_t bundle_createFromArchive(bundle_t * bundle, framework_t framework, bundle_archive_t archive, apr_pool_t *bundlePool);
FRAMEWORK_EXPORT celix_status_t bundle_destroy(bundle_t bundle);

FRAMEWORK_EXPORT celix_status_t bundle_isSystemBundle(bundle_t bundle, bool *systemBundle);
FRAMEWORK_EXPORT celix_status_t bundle_getArchive(bundle_t bundle, bundle_archive_t *archive);
FRAMEWORK_EXPORT celix_status_t bundle_getCurrentModule(bundle_t bundle, module_t *module);
FRAMEWORK_EXPORT array_list_t bundle_getModules(bundle_t bundle);
FRAMEWORK_EXPORT void * bundle_getHandle(bundle_t bundle);
FRAMEWORK_EXPORT void bundle_setHandle(bundle_t bundle, void * handle);
FRAMEWORK_EXPORT ACTIVATOR bundle_getActivator(bundle_t bundle);
FRAMEWORK_EXPORT celix_status_t bundle_setActivator(bundle_t bundle, ACTIVATOR activator);
FRAMEWORK_EXPORT celix_status_t bundle_getManifest(bundle_t bundle, MANIFEST *manifest);
FRAMEWORK_EXPORT celix_status_t bundle_setManifest(bundle_t bundle, MANIFEST manifest);
FRAMEWORK_EXPORT celix_status_t bundle_getContext(bundle_t bundle, bundle_context_t *context);
FRAMEWORK_EXPORT celix_status_t bundle_setContext(bundle_t bundle, bundle_context_t context);
FRAMEWORK_EXPORT celix_status_t bundle_getEntry(bundle_t bundle, char * name, apr_pool_t *pool, char **entry);

FRAMEWORK_EXPORT celix_status_t bundle_start(bundle_t bundle, int options);
FRAMEWORK_EXPORT celix_status_t bundle_update(bundle_t bundle, char *inputFile);
FRAMEWORK_EXPORT celix_status_t bundle_stop(bundle_t bundle, int options);
FRAMEWORK_EXPORT celix_status_t bundle_uninstall(bundle_t bundle);

FRAMEWORK_EXPORT celix_status_t bundle_setState(bundle_t bundle, bundle_state_e state);
FRAMEWORK_EXPORT celix_status_t bundle_setPersistentStateInactive(bundle_t bundle);
FRAMEWORK_EXPORT celix_status_t bundle_setPersistentStateUninstalled(bundle_t bundle);

FRAMEWORK_EXPORT void uninstallBundle(bundle_t bundle);

FRAMEWORK_EXPORT celix_status_t bundle_revise(bundle_t bundle, char * location, char *inputFile);
FRAMEWORK_EXPORT celix_status_t bundle_addModule(bundle_t bundle, module_t module);
FRAMEWORK_EXPORT celix_status_t bundle_closeModules(bundle_t bundle);

// Service Reference Functions
FRAMEWORK_EXPORT array_list_t getUsingBundles(service_reference_t reference);

FRAMEWORK_EXPORT int compareTo(service_reference_t a, service_reference_t b);

FRAMEWORK_EXPORT celix_status_t bundle_getState(bundle_t bundle, bundle_state_e *state);
FRAMEWORK_EXPORT celix_status_t bundle_isLockable(bundle_t bundle, bool *lockable);
FRAMEWORK_EXPORT celix_status_t bundle_getLockingThread(bundle_t bundle, apr_os_thread_t *thread);
FRAMEWORK_EXPORT celix_status_t bundle_lock(bundle_t bundle, bool *locked);
FRAMEWORK_EXPORT celix_status_t bundle_unlock(bundle_t bundle, bool *unlocked);

FRAMEWORK_EXPORT celix_status_t bundle_closeAndDelete(bundle_t bundle);
FRAMEWORK_EXPORT celix_status_t bundle_close(bundle_t bundle);

FRAMEWORK_EXPORT celix_status_t bundle_refresh(bundle_t bundle);
FRAMEWORK_EXPORT celix_status_t bundle_getBundleId(bundle_t bundle, long *id);

FRAMEWORK_EXPORT celix_status_t bundle_getRegisteredServices(bundle_t bundle, apr_pool_t *pool, array_list_t *list);
FRAMEWORK_EXPORT celix_status_t bundle_getServicesInUse(bundle_t bundle, array_list_t *list);

FRAMEWORK_EXPORT celix_status_t bundle_getMemoryPool(bundle_t bundle, apr_pool_t **pool);

FRAMEWORK_EXPORT celix_status_t bundle_setFramework(bundle_t bundle, framework_t framework);
FRAMEWORK_EXPORT celix_status_t bundle_getFramework(bundle_t bundle, framework_t *framework);

#endif /* BUNDLE_H_ */
