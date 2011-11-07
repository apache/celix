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
 *  Created on: Mar 23, 2010
 *      Author: alexanderb
 */

#ifndef BUNDLE_H_
#define BUNDLE_H_

#include <apr_general.h>

#include "headers.h"
#include "celix_errno.h"

celix_status_t bundle_create(BUNDLE * bundle, apr_pool_t *mp);
celix_status_t bundle_createFromArchive(BUNDLE * bundle, FRAMEWORK framework, BUNDLE_ARCHIVE archive, apr_pool_t *bundlePool);
celix_status_t bundle_destroy(BUNDLE bundle);

celix_status_t bundle_isSystemBundle(BUNDLE bundle, bool *systemBundle);
BUNDLE_ARCHIVE bundle_getArchive(BUNDLE bundle);
celix_status_t bundle_getCurrentModule(BUNDLE bundle, MODULE *module);
ARRAY_LIST bundle_getModules(BUNDLE bundle);
void * bundle_getHandle(BUNDLE bundle);
void bundle_setHandle(BUNDLE bundle, void * handle);
ACTIVATOR bundle_getActivator(BUNDLE bundle);
celix_status_t bundle_setActivator(BUNDLE bundle, ACTIVATOR activator);
celix_status_t bundle_getManifest(BUNDLE bundle, MANIFEST *manifest);
celix_status_t bundle_setManifest(BUNDLE bundle, MANIFEST manifest);
celix_status_t bundle_getContext(BUNDLE bundle, BUNDLE_CONTEXT *context);
celix_status_t bundle_setContext(BUNDLE bundle, BUNDLE_CONTEXT context);
celix_status_t bundle_getEntry(BUNDLE bundle, char * name, apr_pool_t *pool, char **entry);

celix_status_t bundle_start(BUNDLE bundle, int options);
celix_status_t bundle_update(BUNDLE bundle, char *inputFile);
celix_status_t bundle_stop(BUNDLE bundle, int options);
celix_status_t bundle_uninstall(BUNDLE bundle);

celix_status_t bundle_setState(BUNDLE bundle, BUNDLE_STATE state);
celix_status_t bundle_setPersistentStateInactive(BUNDLE bundle);
celix_status_t bundle_setPersistentStateUninstalled(BUNDLE bundle);

void uninstallBundle(BUNDLE bundle);

celix_status_t bundle_revise(BUNDLE bundle, char * location, char *inputFile);
celix_status_t bundle_addModule(BUNDLE bundle, MODULE module);
celix_status_t bundle_closeModules(BUNDLE bundle);

// Service Reference Functions
ARRAY_LIST getUsingBundles(SERVICE_REFERENCE reference);

int compareTo(SERVICE_REFERENCE a, SERVICE_REFERENCE b);


celix_status_t bundle_getState(BUNDLE bundle, BUNDLE_STATE *state);
celix_status_t bundle_isLockable(BUNDLE bundle, bool *lockable);
celix_status_t bundle_getLockingThread(BUNDLE bundle, apr_os_thread_t *thread);
celix_status_t bundle_lock(BUNDLE bundle, bool *locked);
celix_status_t bundle_unlock(BUNDLE bundle, bool *unlocked);

celix_status_t bundle_closeAndDelete(BUNDLE bundle);
celix_status_t bundle_close(BUNDLE bundle);

celix_status_t bundle_refresh(BUNDLE bundle);
celix_status_t bundle_getBundleId(BUNDLE bundle, long *id);

celix_status_t bundle_getRegisteredServices(BUNDLE bundle, ARRAY_LIST *list);

#endif /* BUNDLE_H_ */
