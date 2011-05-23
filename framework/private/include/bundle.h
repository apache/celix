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

bool bundle_isSystemBundle(BUNDLE bundle);
BUNDLE_ARCHIVE bundle_getArchive(BUNDLE bundle);
MODULE bundle_getCurrentModule(BUNDLE bundle);
ARRAY_LIST bundle_getModules(BUNDLE bundle);
void * bundle_getHandle(BUNDLE bundle);
void bundle_setHandle(BUNDLE bundle, void * handle);
ACTIVATOR bundle_getActivator(BUNDLE bundle);
void bundle_setActivator(BUNDLE bundle, ACTIVATOR activator);
MANIFEST bundle_getManifest(BUNDLE bundle);
void bundle_setManifest(BUNDLE bundle, MANIFEST manifest);
BUNDLE_CONTEXT bundle_getContext(BUNDLE bundle);
void bundle_setContext(BUNDLE bundle, BUNDLE_CONTEXT context);
celix_status_t bundle_getEntry(BUNDLE bundle, char * name, char **entry);

void startBundle(BUNDLE bundle, int options);
celix_status_t bundle_update(BUNDLE bundle, char *inputFile);
void stopBundle(BUNDLE bundle, int options);
celix_status_t bundle_uninstall(BUNDLE bundle);

celix_status_t bundle_setPersistentStateInactive(BUNDLE bundle);

void uninstallBundle(BUNDLE bundle);

celix_status_t bundle_revise(BUNDLE bundle, char * location, char *inputFile);

// Service Reference Functions
ARRAY_LIST getUsingBundles(SERVICE_REFERENCE reference);

int compareTo(SERVICE_REFERENCE a, SERVICE_REFERENCE b);


BUNDLE_STATE bundle_getState(BUNDLE bundle);
bool bundle_isLockable(BUNDLE bundle);
pthread_t bundle_getLockingThread(BUNDLE bundle);
bool bundle_lock(BUNDLE bundle);
bool bundle_unlock(BUNDLE bundle);

celix_status_t bundle_closeAndDelete(BUNDLE bundle);
celix_status_t bundle_close(BUNDLE bundle);

celix_status_t bundle_refresh(BUNDLE bundle);

#endif /* BUNDLE_H_ */
