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

#include "headers.h"

BUNDLE bundle_create();

BUNDLE bundle_createFromArchive(FRAMEWORK framework, BUNDLE_ARCHIVE archive);

BUNDLE_ARCHIVE bundle_getArchive(BUNDLE bundle);
MODULE bundle_getModule(BUNDLE bundle);
void * bundle_getHandle(BUNDLE bundle);
void bundle_setHandle(BUNDLE bundle, void * handle);
ACTIVATOR bundle_getActivator(BUNDLE bundle);
void bundle_setActivator(BUNDLE bundle, ACTIVATOR activator);
BUNDLE_CONTEXT bundle_getContext(BUNDLE bundle);
void bundle_setContext(BUNDLE bundle, BUNDLE_CONTEXT context);

void startBundle(BUNDLE bundle, int options);

void stopBundle(BUNDLE bundle, int options);

//void updateBundle(BUNDLE bundle, InputStream input);

void uninstallBundle(BUNDLE bundle);

// Service Reference Functions
ARRAY_LIST getUsingBundles(SERVICE_REFERENCE reference);

int compareTo(SERVICE_REFERENCE a, SERVICE_REFERENCE b);


BUNDLE_STATE bundle_getState(BUNDLE bundle);
bool bundle_isLockable(BUNDLE bundle);
pthread_t bundle_getLockingThread(BUNDLE bundle);
bool bundle_lock(BUNDLE bundle);
bool bundle_unlock(BUNDLE bundle);


#endif /* BUNDLE_H_ */
