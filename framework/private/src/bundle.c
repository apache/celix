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
 * bundle.c
 *
 *  Created on: Mar 23, 2010
 *      Author: alexanderb
 */
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "bundle.h"
#include "framework.h"
#include "manifest.h"
#include "module.h"
#include "version.h"
#include "array_list.h"
#include "bundle_archive.h"

struct bundle {
	BUNDLE_CONTEXT context;
	ACTIVATOR activator;
	long lastModified;
	BUNDLE_STATE state;
	void * handle;
	BUNDLE_ARCHIVE archive;
	MODULE module;

	pthread_mutex_t lock;
	int lockCount;
	pthread_t lockThread;

	struct framework * framework;
};

MODULE bundle_createModule(BUNDLE bundle);

BUNDLE bundle_create() {
	BUNDLE bundle = (BUNDLE) malloc(sizeof(*bundle));
	BUNDLE_ARCHIVE archive = bundleArchive_createSystemBundleArchive();
	bundle->archive = archive;
	bundle->activator = NULL;
	bundle->context = NULL;
	bundle->framework = NULL;
	bundle->state = BUNDLE_INSTALLED;


	MODULE module = module_createFrameworkModule();

	bundle->module = module;

	pthread_mutex_init(&bundle->lock, NULL);
	bundle->lockCount = 0;
	bundle->lockThread = NULL;

	resolver_addModule(module);

	return bundle;
}

BUNDLE bundle_createFromArchive(FRAMEWORK framework, BUNDLE_ARCHIVE archive) {
	BUNDLE bundle = malloc(sizeof(*bundle));
	bundle->archive = archive;
	bundle->activator = NULL;
	bundle->context = NULL;
	bundle->framework = framework;
	bundle->state = BUNDLE_INSTALLED;

	MODULE module = bundle_createModule(bundle);
	bundle->module = module;

	pthread_mutex_init(&bundle->lock, NULL);
	bundle->lockCount = 0;
	bundle->lockThread = NULL;

	resolver_addModule(module);

	return bundle;
}

BUNDLE_ARCHIVE bundle_getArchive(BUNDLE bundle) {
	return bundle->archive;
}

MODULE bundle_getModule(BUNDLE bundle) {
	return bundle->module;
}

void * bundle_getHandle(BUNDLE bundle) {
	return bundle->handle;
}

void bundle_setHandle(BUNDLE bundle, void * handle) {
	bundle->handle = handle;
}

ACTIVATOR bundle_getActivator(BUNDLE bundle) {
	return bundle->activator;
}

void bundle_setActivator(BUNDLE bundle, ACTIVATOR activator) {
	bundle->activator = activator;
}

BUNDLE_CONTEXT bundle_getContext(BUNDLE bundle) {
	return bundle->context;
}

void bundle_setContext(BUNDLE bundle, BUNDLE_CONTEXT context) {
	bundle->context = context;
}

BUNDLE_STATE bundle_getState(BUNDLE bundle) {
	return bundle->state;
}

void bundle_setState(BUNDLE bundle, BUNDLE_STATE state) {
	bundle->state = state;
}

MODULE bundle_createModule(BUNDLE bundle) {
	MANIFEST headerMap = getManifest(bundle->archive);
	long bundleId = bundleArchive_getId(bundle->archive);
	int revision = 0;
	char moduleId[sizeof(bundleId) + sizeof(revision) + 2];
	sprintf(moduleId, "%ld.%d", bundleId, revision);

	MODULE module = module_create(headerMap, strdup(moduleId), bundle);


	VERSION bundleVersion = module_getVersion(module);
	char * symName = module_getSymbolicName(module);

	ARRAY_LIST bundles = framework_getBundles(bundle->framework);
	int i;
	for (i = 0; i < arrayList_size(bundles); i++) {
		BUNDLE check = (BUNDLE) arrayList_get(bundles, i);

		long id = bundleArchive_getId(check->archive);
		if (id != bundleArchive_getId(bundle->archive)) {
			char * sym = module_getSymbolicName(check->module);
			VERSION version = module_getVersion(check->module);
			if ((symName != NULL) && (sym != NULL) && !strcmp(symName, sym) &&
					!version_compareTo(bundleVersion, version)) {
				printf("Bundle symbolic name and version are not unique: %s:%s\n", sym, version_toString(version));
				return NULL;
			}
		}
	}

	return module;
}

void startBundle(BUNDLE bundle, int options) {
	fw_startBundle(bundle->framework, bundle, options);

}

void stopBundle(BUNDLE bundle, int options) {
	fw_stopBundle(bundle->framework, bundle, options);
}



bool bundle_isLockable(BUNDLE bundle) {
	bool lockable = false;
	pthread_mutex_lock(&bundle->lock);

	lockable = (bundle->lockCount == 0) || (bundle->lockThread == pthread_self());

	pthread_mutex_unlock(&bundle->lock);

	return lockable;
}

pthread_t bundle_getLockingThread(BUNDLE bundle) {
	pthread_t lockingThread = NULL;
	pthread_mutex_lock(&bundle->lock);

	lockingThread = bundle->lockThread;

	pthread_mutex_unlock(&bundle->lock);
}

bool bundle_lock(BUNDLE bundle) {
	pthread_mutex_lock(&bundle->lock);

	if ((bundle->lockCount > 0) && bundle->lockThread != pthread_self()) {
		return false;
	}
	bundle->lockCount++;
	bundle->lockThread = pthread_self();

	pthread_mutex_unlock(&bundle->lock);
	return true;
}

bool bundle_unlock(BUNDLE bundle) {
	pthread_mutex_lock(&bundle->lock);

	if ((bundle->lockCount == 0)) {
		return false;
	}
	if ((bundle->lockCount > 0) && bundle->lockThread != pthread_self()) {
		return false;
	}
	bundle->lockCount--;
	if (bundle->lockCount == 0) {
		bundle->lockThread = NULL;
	}

	pthread_mutex_unlock(&bundle->lock);
	return true;
}
