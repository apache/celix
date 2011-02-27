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
 * framework.c
 *
 *  Created on: Mar 23, 2010
 *      Author: alexanderb
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdbool.h>
#include <pthread.h>
#include <math.h>

#include "framework.h"
#include "filter.h"
#include "constants.h"
#include "archive.h"
#include "bundle.h"
#include "wire.h"
#include "resolver.h"
#include "utils.h"
#include "bundle_activator.h"
#include "service_registry.h"
#include "bundle_cache.h"
#include "bundle_archive.h"
#include "bundle_context.h"

struct activator {
	void * userData;
	void (*start)(void * userData, BUNDLE_CONTEXT context);
	void (*stop)(void * userData, BUNDLE_CONTEXT context);
	void (*destroy)(void * userData);
};

//struct exception_context the_exception_context[1];

ARRAY_LIST m_serviceListeners;

HASH_MAP m_installRequestMap;

pthread_mutex_t m_installRequestLock = PTHREAD_MUTEX_INITIALIZER;

void framework_setBundleStateAndNotify(FRAMEWORK framework, BUNDLE bundle, int state);
celix_status_t framework_markBundleResolved(FRAMEWORK framework, MODULE module);

celix_status_t framework_acquireBundleLock(FRAMEWORK framework, BUNDLE bundle, int desiredStates);
bool framework_releaseBundleLock(FRAMEWORK framework, BUNDLE bundle);

bool framework_acquireGlobalLock(FRAMEWORK framework);
void framework_releaseGlobalLock(FRAMEWORK framework);

long framework_getNextBundleId(FRAMEWORK framework);

celix_status_t fw_installBundle2(FRAMEWORK framework, BUNDLE * bundle, long id, char * location, BUNDLE_ARCHIVE archive);

struct fw_serviceListener {
	BUNDLE bundle;
	SERVICE_LISTENER listener;
	FILTER filter;
};

typedef struct fw_serviceListener * FW_SERVICE_LISTENER;

celix_status_t framework_create(FRAMEWORK *framework) {
	*framework = (FRAMEWORK) malloc(sizeof(**framework));
	if (*framework == NULL) {
		return CELIX_ENOMEM;
	}

	apr_status_t rv = apr_initialize();
	if (rv != APR_SUCCESS) {
		return CELIX_START_ERROR;
	}

	apr_pool_create(&(*framework)->mp, NULL);

	BUNDLE bundle = NULL;
	celix_status_t rvb = bundle_create(&bundle, (*framework)->mp);
	if (rvb != CELIX_SUCCESS) {
		return rvb;
	}
	(*framework)->bundle = bundle;
	(*framework)->bundle->framework = (*framework);

	(*framework)->installedBundleMap = NULL;
	(*framework)->registry = NULL;

	pthread_cond_init(&(*framework)->condition, NULL);
	pthread_mutex_init(&(*framework)->mutex, NULL);
	pthread_mutex_init(&(*framework)->bundleLock, NULL);

	(*framework)->interrupted = false;

	(*framework)->globalLockWaitersList = arrayList_create();
	(*framework)->globalLockCount = 0;
	(*framework)->globalLockThread = NULL;
	(*framework)->nextBundleId = 1l;

	m_installRequestMap = hashMap_create(string_hash, string_hash, string_equals, string_equals);

	return CELIX_SUCCESS;
}

celix_status_t fw_init(FRAMEWORK framework) {
	celix_status_t lock = framework_acquireBundleLock(framework, framework->bundle, BUNDLE_INSTALLED|BUNDLE_RESOLVED|BUNDLE_STARTING|BUNDLE_ACTIVE);
	if (lock != CELIX_SUCCESS) {
		framework_releaseBundleLock(framework, framework->bundle);
		return lock;
	}

	if ((bundle_getState(framework->bundle) == BUNDLE_INSTALLED) || (bundle_getState(framework->bundle) == BUNDLE_RESOLVED)) {
		PROPERTIES props = properties_create();
		properties_set(props, (char *) FRAMEWORK_STORAGE, ".cache");
		framework->cache = bundleCache_create(props, framework->mp);

		if (bundle_getState(framework->bundle) == BUNDLE_INSTALLED) {
			// clean cache
			// bundleCache_delete(framework->cache);
		}
	}

	framework->installedBundleMap = hashMap_create(string_hash, NULL, string_equals, NULL);

	hashMap_put(framework->installedBundleMap, bundleArchive_getLocation(bundle_getArchive(framework->bundle)), framework->bundle);

	if (resolver_resolve(bundle_getModule(framework->bundle)) == NULL) {
		printf("Unresolved constraints in System Bundle\n");
		framework_releaseBundleLock(framework, framework->bundle);
		return CELIX_BUNDLE_EXCEPTION;
	}

	// reload archives from cache
	ARRAY_LIST archives = bundleCache_getArchives(framework->cache);
	int arcIdx;
	for (arcIdx = 0; arcIdx < arrayList_size(archives); arcIdx++) {
		BUNDLE_ARCHIVE archive = (BUNDLE_ARCHIVE) arrayList_get(archives, arcIdx);
//		framework->nextBundleId = fmaxl(framework->nextBundleId, bundleArchive_getId(archive) + 1);

		if (bundleArchive_getPersistentState(archive) == BUNDLE_UNINSTALLED) {
			bundleArchive_closeAndDelete(archive);
		} else {
			BUNDLE bundle;
			fw_installBundle2(framework, &bundle, bundleArchive_getId(archive), bundleArchive_getLocation(archive), archive);
		}
	}
	framework->registry = serviceRegistry_create(fw_serviceChanged);

	framework_setBundleStateAndNotify(framework, framework->bundle, BUNDLE_STARTING);

	pthread_cond_init(&framework->shutdownGate, NULL);

	void * handle = dlopen(NULL, RTLD_LAZY|RTLD_LOCAL);
	if (handle == NULL) {
		printf ("%s\n", dlerror());
		framework_releaseBundleLock(framework, framework->bundle);
		return CELIX_START_ERROR;
	}

	bundle_setHandle(framework->bundle, handle);

	ACTIVATOR activator = (ACTIVATOR) malloc(sizeof(*activator));
	void * (*create)();
	void (*start)(void * handle, BUNDLE_CONTEXT context);
	void (*stop)(void * handle, BUNDLE_CONTEXT context);
	void (*destroy)(void * handle);
	create = dlsym(bundle_getHandle(framework->bundle), BUNDLE_ACTIVATOR_CREATE);
	start = dlsym(bundle_getHandle(framework->bundle), BUNDLE_ACTIVATOR_START);
	stop = dlsym(bundle_getHandle(framework->bundle), BUNDLE_ACTIVATOR_STOP);
	destroy = dlsym(bundle_getHandle(framework->bundle), BUNDLE_ACTIVATOR_DESTROY);
	activator->start = start;
	activator->stop = stop;
	activator->destroy = destroy;
	bundle_setActivator(framework->bundle, activator);

	BUNDLE_CONTEXT context = bundleContext_create(framework, framework->bundle);
	bundle_setContext(framework->bundle, context);

	void * userData = NULL;
	if (create != NULL) {
		userData = create();
	}
	activator->userData = userData;

	if (start != NULL) {
		start(userData, bundle_getContext(framework->bundle));
	}


	m_serviceListeners = arrayList_create();
	framework_releaseBundleLock(framework, framework->bundle);

	return CELIX_SUCCESS;
}

celix_status_t framework_start(FRAMEWORK framework) {
	celix_status_t lock = framework_acquireBundleLock(framework, framework->bundle, BUNDLE_INSTALLED|BUNDLE_RESOLVED|BUNDLE_STARTING|BUNDLE_ACTIVE);
	if (lock != CELIX_SUCCESS) {
		printf("could not get lock\n");
		framework_releaseBundleLock(framework, framework->bundle);
		return lock;
	}
	if ((bundle_getState(framework->bundle) == BUNDLE_INSTALLED) || (bundle_getState(framework->bundle) == BUNDLE_RESOLVED)) {
		fw_init(framework);
	}

	if (bundle_getState(framework->bundle) == BUNDLE_STARTING) {
		framework_setBundleStateAndNotify(framework, framework->bundle, BUNDLE_ACTIVE);
	}

	framework_releaseBundleLock(framework, framework->bundle);
	return CELIX_SUCCESS;
}

void framework_stop(FRAMEWORK framework) {
	fw_stopBundle(framework, framework->bundle, 0);
}

celix_status_t fw_installBundle(FRAMEWORK framework, BUNDLE * bundle, char * location) {
	return fw_installBundle2(framework, bundle, -1, location, NULL);
}

celix_status_t fw_installBundle2(FRAMEWORK framework, BUNDLE * bundle, long id, char * location, BUNDLE_ARCHIVE archive) {
  	framework_acquireInstallLock(framework, location);

	if (bundle_getState(framework->bundle) == BUNDLE_STOPPING || bundle_getState(framework->bundle) == BUNDLE_UNINSTALLED) {
		printf("The framework has been shutdown.\n");
		framework_releaseInstallLock(framework, location);
		return CELIX_FRAMEWORK_SHUTDOWN;
	}

	*bundle = framework_getBundle(framework, location);
	if (*bundle != NULL) {
		framework_releaseInstallLock(framework, location);
		return CELIX_SUCCESS;
	}

	if (archive == NULL) {
		id = framework_getNextBundleId(framework);
		archive = bundleCache_createArchive(framework->cache, id, location); //fw_createArchive(id, location);
	} else {
		// purge revision
		// multiple revisions not yet implemented
	}

	bool locked = framework_acquireGlobalLock(framework);
	if (!locked) {
		printf("Unable to acquire the global lock to install the bundle\n");
		framework_releaseInstallLock(framework, location);
		return CELIX_BUNDLE_EXCEPTION;
	}

	celix_status_t rv = bundle_createFromArchive(bundle, framework, archive);
	framework_releaseGlobalLock(framework);

	hashMap_put(framework->installedBundleMap, location, *bundle);

  	framework_releaseInstallLock(framework, location);

	return CELIX_SUCCESS;
}

celix_status_t fw_startBundle(FRAMEWORK framework, BUNDLE bundle, int options ATTRIBUTE_UNUSED) {
	celix_status_t lock = framework_acquireBundleLock(framework, bundle, BUNDLE_INSTALLED|BUNDLE_RESOLVED|BUNDLE_STARTING|BUNDLE_ACTIVE);
	if (lock != CELIX_SUCCESS) {
		printf("Unable to start\n");
		framework_releaseBundleLock(framework, bundle);
		return lock;
	}

	HASH_MAP wires;

	void * handle;
	BUNDLE_CONTEXT context;

	switch (bundle_getState(bundle)) {
		case BUNDLE_UNINSTALLED:
			printf("Cannot start bundle since it is uninstalled.");
			framework_releaseBundleLock(framework, bundle);
			return CELIX_ILLEGAL_STATE;
		case BUNDLE_STARTING:
			printf("Cannot start bundle since it is starting.");
			framework_releaseBundleLock(framework, bundle);
			return CELIX_BUNDLE_EXCEPTION;
		case BUNDLE_STOPPING:
			printf("Cannot start bundle since it is stopping.");
			framework_releaseBundleLock(framework, bundle);
			return CELIX_ILLEGAL_STATE;
		case BUNDLE_ACTIVE:
			printf("Cannot start bundle since it is already active.");
			framework_releaseBundleLock(framework, bundle);
			return CELIX_SUCCESS;
		case BUNDLE_INSTALLED:
			wires = resolver_resolve(bundle_getModule(bundle));
			framework_markResolvedModules(framework, wires);
		case BUNDLE_RESOLVED:
			context = bundleContext_create(framework, bundle);
			bundle_setContext(bundle, context);

			MANIFEST manifest = getManifest(bundle_getArchive(bundle));
			char * library = manifest_getValue(manifest, HEADER_LIBRARY);

	#ifdef __linux__
			 char * library_prefix = "lib";
			 char * library_extension = ".so";
	#elif __APPLE__
			 char * library_prefix = "lib";
			 char * library_extension = ".dylib";
	#elif WIN32
			 char * library_prefix = "";
			 char * library_extension = ".dll";
	#endif

			char * root = bundleArchive_getArchiveRoot(bundle_getArchive(bundle));
			char * revision = bundleArchive_getRevision(bundle_getArchive(bundle));
			char libraryPath[strlen(root) + strlen(revision) + strlen(library) +
							 strlen(library_prefix) + strlen(library_extension) + 10];
			strcpy(libraryPath, root);
			strcat(libraryPath, "/version");
			strcat(libraryPath, revision);
			strcat(libraryPath, "/");
			strcat(libraryPath, library_prefix);
			strcat(libraryPath, library);
			strcat(libraryPath, library_extension);

			handle = dlopen(libraryPath, RTLD_LAZY|RTLD_LOCAL);
			if (handle == NULL) {
				printf ("%s\n", dlerror());
				framework_releaseBundleLock(framework, bundle);
				return CELIX_BUNDLE_EXCEPTION;
			}

			bundle_setHandle(bundle, handle);

			ACTIVATOR activator = (ACTIVATOR) malloc(sizeof(*activator));
			void * (*create)();
			void (*start)(void * userData, BUNDLE_CONTEXT context);
			void (*stop)(void * userData, BUNDLE_CONTEXT context);
			void (*destroy)(void * userData);
			create = dlsym(bundle_getHandle(bundle), BUNDLE_ACTIVATOR_CREATE);
			start = dlsym(bundle_getHandle(bundle), BUNDLE_ACTIVATOR_START);
			stop = dlsym(bundle_getHandle(bundle), BUNDLE_ACTIVATOR_STOP);
			destroy = dlsym(bundle_getHandle(bundle), BUNDLE_ACTIVATOR_DESTROY);
			activator->start = start;
			activator->stop = stop;
			activator->destroy = destroy;
			bundle_setActivator(bundle, activator);

			framework_setBundleStateAndNotify(framework, bundle, BUNDLE_STARTING);

			void * userData = NULL;
			if (create != NULL) {
				userData = create();
			}
			activator->userData = userData;

			if (start != NULL) {
				start(userData, bundle_getContext(bundle));
			}

			framework_setBundleStateAndNotify(framework, bundle, BUNDLE_ACTIVE);

			break;
	}

	framework_releaseBundleLock(framework, bundle);
	return CELIX_SUCCESS;
}

void fw_stopBundle(FRAMEWORK framework, BUNDLE bundle, int options ATTRIBUTE_UNUSED) {
	celix_status_t lock = framework_acquireBundleLock(framework, bundle, BUNDLE_INSTALLED|BUNDLE_RESOLVED|BUNDLE_STARTING|BUNDLE_ACTIVE);
	if (lock != CELIX_SUCCESS) {
		printf("Cannot stop bundle");
		framework_releaseBundleLock(framework, bundle);
		return;
	}

	switch (bundle_getState(bundle)) {
		case BUNDLE_UNINSTALLED:
			printf("Cannot stop bundle since it is uninstalled.");
			framework_releaseBundleLock(framework, bundle);
			return;
		case BUNDLE_STARTING:
			printf("Cannot stop bundle since it is starting.");
			framework_releaseBundleLock(framework, bundle);
			return;
		case BUNDLE_STOPPING:
			printf("Cannot stop bundle since it is stopping.");
			framework_releaseBundleLock(framework, bundle);
			return;
		case BUNDLE_INSTALLED:
		case BUNDLE_RESOLVED:
			framework_releaseBundleLock(framework, bundle);
			return;
		case BUNDLE_ACTIVE:
			// only valid state
			break;
	}

	framework_setBundleStateAndNotify(framework, bundle, BUNDLE_STOPPING);

	if (bundle_getActivator(bundle)->stop != NULL) {
		bundle_getActivator(bundle)->stop(bundle_getActivator(bundle)->userData, bundle_getContext(bundle));
	}

	if (bundle_getActivator(bundle)->destroy != NULL) {
		bundle_getActivator(bundle)->destroy(bundle_getActivator(bundle)->userData);
	}

	if (module_getId(bundle_getModule(bundle)) != 0) {
		bundle_getActivator(bundle)->start = NULL;
		bundle_getActivator(bundle)->stop = NULL;
		bundle_getActivator(bundle)->userData = NULL;
		free(bundle_getActivator(bundle));
		bundle_setActivator(bundle, NULL);

		serviceRegistry_unregisterServices(framework->registry, bundle);
		serviceRegistry_ungetServices(framework->registry, bundle);

		dlclose(bundle_getHandle(bundle));
	}

	framework_setBundleStateAndNotify(framework, bundle, BUNDLE_RESOLVED);

	framework_releaseBundleLock(framework, bundle);
}

celix_status_t fw_registerService(FRAMEWORK framework, SERVICE_REGISTRATION *registration, BUNDLE bundle, char * serviceName, void * svcObj, PROPERTIES properties) {
	if (serviceName == NULL) {
		printf("Service name cannot be null");
		return CELIX_ILLEGAL_ARGUMENT;
	} else if (svcObj == NULL) {
		printf("Service object cannot be null");
		return CELIX_ILLEGAL_ARGUMENT;
	}

	celix_status_t lock = framework_acquireBundleLock(framework, bundle, BUNDLE_STARTING|BUNDLE_ACTIVE);
	if (lock != CELIX_SUCCESS) {
		printf("Can only register services while bundle is active or starting");
		framework_releaseBundleLock(framework, bundle);
		return CELIX_ILLEGAL_STATE;
	}
	*registration = serviceRegistry_registerService(framework->registry, bundle, serviceName, svcObj, properties);
	framework_releaseBundleLock(framework, bundle);

	return CELIX_SUCCESS;
}

celix_status_t fw_getServiceReferences(FRAMEWORK framework, ARRAY_LIST *references, BUNDLE bundle ATTRIBUTE_UNUSED, char * serviceName, char * sfilter) {
	FILTER filter = NULL;
	if (sfilter != NULL) {
		filter = filter_create(sfilter);
	}

	*references = serviceRegistry_getServiceReferences(framework->registry, serviceName, filter);

	if (filter != NULL) {
		filter_destroy(filter);
	}

	int refIdx = 0;
	for (refIdx = 0; (*references != NULL) && refIdx < arrayList_size(*references); refIdx++) {
		SERVICE_REFERENCE ref = (SERVICE_REFERENCE) arrayList_get(*references, refIdx);
		SERVICE_REGISTRATION reg = ref->registration;
		char * serviceName = properties_get(reg->properties, (char *) OBJECTCLASS);
		if (!serviceReference_isAssignableTo(ref, bundle, serviceName)) {
			arrayList_remove(*references, refIdx);
			refIdx--;
		}
	}

	return CELIX_SUCCESS;
}

void * fw_getService(FRAMEWORK framework, BUNDLE bundle ATTRIBUTE_UNUSED, SERVICE_REFERENCE reference) {
	return serviceRegistry_getService(framework->registry, bundle, reference);
}

bool framework_ungetService(FRAMEWORK framework, BUNDLE bundle ATTRIBUTE_UNUSED, SERVICE_REFERENCE reference) {
	return serviceRegistry_ungetService(framework->registry, bundle, reference);
}

void fw_addServiceListener(BUNDLE bundle, SERVICE_LISTENER listener, char * sfilter) {
	FW_SERVICE_LISTENER fwListener = (FW_SERVICE_LISTENER) malloc(sizeof(*fwListener));
	fwListener->bundle = bundle;
	if (sfilter != NULL) {
		FILTER filter = filter_create(sfilter);
		fwListener->filter = filter;
	}
	fwListener->listener = listener;
	arrayList_add(m_serviceListeners, fwListener);
}

void fw_removeServiceListener(BUNDLE bundle, SERVICE_LISTENER listener) {
	unsigned int i;
	FW_SERVICE_LISTENER element;
	for (i = 0; i < arrayList_size(m_serviceListeners); i++) {
		element = (FW_SERVICE_LISTENER) arrayList_get(m_serviceListeners, i);
		if (element->listener == listener && element->bundle == bundle) {
			arrayList_remove(m_serviceListeners, i);
			i--;
			element->bundle = NULL;
			filter_destroy(element->filter);
			element->filter = NULL;
			element->listener = NULL;
			free(element);
			element = NULL;
			break;
		}
	}
}

void fw_serviceChanged(SERVICE_EVENT event, PROPERTIES oldprops) {
	unsigned int i;
	FW_SERVICE_LISTENER element;
	SERVICE_REGISTRATION registration = event->reference->registration;
	if (arrayList_size(m_serviceListeners) > 0) {
		for (i = 0; i < arrayList_size(m_serviceListeners); i++) {
			int matched = 0;
			element = (FW_SERVICE_LISTENER) arrayList_get(m_serviceListeners, i);
			matched = (element->filter == NULL) || filter_match(element->filter, registration->properties);
			if (matched) {
				element->listener->serviceChanged(element->listener, event);
			} else if (event->type == MODIFIED) {
				int matched = (element->filter == NULL) || filter_match(element->filter, oldprops);
				if (matched) {
					SERVICE_EVENT endmatch = (SERVICE_EVENT) malloc(sizeof(*endmatch));
					endmatch->reference = event->reference;
					endmatch->type = MODIFIED_ENDMATCH;
					element->listener->serviceChanged(element->listener, endmatch);
				}
			}
		}
	}
}

MANIFEST getManifest(BUNDLE_ARCHIVE archive) {
	char * root = bundleArchive_getArchiveRoot(archive);
	char * revision = bundleArchive_getRevision(archive);
	char manifest[strlen(root) + strlen(revision) + 30];
	strcpy(manifest, root);
	strcat(manifest, "/version");
	strcat(manifest, revision);
	strcat(manifest, "/MANIFEST/MANIFEST.MF");
	return manifest_read(manifest);
}

long framework_getNextBundleId(FRAMEWORK framework) {
	long id = framework->nextBundleId;
	framework->nextBundleId++;
	return id;
}

celix_status_t framework_markResolvedModules(FRAMEWORK framework, HASH_MAP resolvedModuleWireMap) {
	if (resolvedModuleWireMap != NULL) {
		LINKED_LIST wireList = linkedList_create();
		HASH_MAP_ITERATOR iterator = hashMapIterator_create(resolvedModuleWireMap);
		while (hashMapIterator_hasNext(iterator)) {
			HASH_MAP_ENTRY entry = hashMapIterator_nextEntry(iterator);
			MODULE module = (MODULE) hashMapEntry_getKey(entry);
			LINKED_LIST wires = (LINKED_LIST) hashMapEntry_getValue(entry);

			linkedList_clear(wireList);

			if (linkedList_size(wires) > 0) {
				module_setWires(module, wires);
			}

			module_setResolved(module);
			resolver_moduleResolved(module);
			framework_markBundleResolved(framework, module);
		}
	}
	return CELIX_SUCCESS;
}

celix_status_t framework_markBundleResolved(FRAMEWORK framework, MODULE module) {
	BUNDLE bundle = module_getBundle(module);
	if (bundle != NULL) {
		framework_acquireBundleLock(framework, bundle, BUNDLE_INSTALLED|BUNDLE_RESOLVED|BUNDLE_ACTIVE);
		if (bundle_getState(bundle) != BUNDLE_INSTALLED) {
			printf("Trying to resolve a resolved bundle");
		} else {
			framework_setBundleStateAndNotify(framework, bundle, BUNDLE_RESOLVED);
			// framework_fireBundleEvent(BUNDLE_EVENT_RESOLVED, bundle);
		}
		framework_releaseBundleLock(framework, bundle);
	}

	return CELIX_SUCCESS;
}

ARRAY_LIST framework_getBundles(FRAMEWORK framework) {
	ARRAY_LIST bundles = arrayList_create();
	HASH_MAP_ITERATOR iterator = hashMapIterator_create(framework->installedBundleMap);
	while (hashMapIterator_hasNext(iterator)) {
		BUNDLE bundle = hashMapIterator_nextValue(iterator);
		arrayList_add(bundles, bundle);
	}
	return bundles;
}

BUNDLE framework_getBundle(FRAMEWORK framework, char * location) {
	BUNDLE bundle = hashMap_get(framework->installedBundleMap, location);
	return bundle;
}

BUNDLE framework_getBundleById(FRAMEWORK framework, long id) {
	HASH_MAP_ITERATOR iter = hashMapIterator_create(framework->installedBundleMap);
	while (hashMapIterator_hasNext(iter)) {
		BUNDLE b = hashMapIterator_nextValue(iter);
		if (bundleArchive_getId(bundle_getArchive(b)) == id) {
			return b;
		}
	}
	return NULL;
}

int framework_acquireInstallLock(FRAMEWORK framework, char * location) {
	pthread_mutex_lock(&m_installRequestLock);

	while (hashMap_get(m_installRequestMap, location) != NULL) {
		pthread_cond_wait(&framework->condition, &m_installRequestLock);
	}
	hashMap_put(m_installRequestMap, location, location);

	pthread_mutex_unlock(&m_installRequestLock);
}

int framework_releaseInstallLock(FRAMEWORK framework, char * location) {
	pthread_mutex_lock(&m_installRequestLock);

	hashMap_remove(m_installRequestMap, location);
	pthread_cond_broadcast(&framework->condition);

	pthread_mutex_unlock(&m_installRequestLock);
}

void framework_setBundleStateAndNotify(FRAMEWORK framework, BUNDLE bundle, int state) {
	pthread_mutex_lock(&framework->bundleLock);

	bundle_setState(bundle, state);
	pthread_cond_broadcast(&framework->condition);

	pthread_mutex_unlock(&framework->bundleLock);
}

celix_status_t framework_acquireBundleLock(FRAMEWORK framework, BUNDLE bundle, int desiredStates) {
	pthread_mutex_lock(&framework->bundleLock);

	while (!bundle_isLockable(bundle)
			|| ((framework->globalLockThread != NULL)
			&& (framework->globalLockThread != pthread_self()))) {
		if ((desiredStates & bundle_getState(bundle)) == 0) {
			pthread_mutex_unlock(&framework->bundleLock);
			return CELIX_ILLEGAL_STATE;
		} else if (framework->globalLockThread == pthread_self()
				&& (bundle_getLockingThread(bundle) != NULL)
				&& arrayList_contains(framework->globalLockWaitersList, bundle_getLockingThread(bundle))) {
			framework->interrupted = true;
//			pthread_cond_signal_thread_np(&framework->condition, bundle_getLockingThread(bundle));
			pthread_cond_signal(&framework->condition);
		}

		pthread_cond_wait(&framework->condition, &framework->bundleLock);
	}

	if ((desiredStates & bundle_getState(bundle)) == 0) {
		pthread_mutex_unlock(&framework->bundleLock);
		return CELIX_ILLEGAL_STATE;
	}

	if (!bundle_lock(bundle)) {
		pthread_mutex_unlock(&framework->bundleLock);
		return CELIX_ILLEGAL_STATE;
	}

	pthread_mutex_unlock(&framework->bundleLock);
	return CELIX_SUCCESS;
}

bool framework_releaseBundleLock(FRAMEWORK framework, BUNDLE bundle) {
	pthread_mutex_lock(&framework->bundleLock);

	if (!bundle_unlock(bundle)) {
		pthread_mutex_unlock(&framework->bundleLock);
		return false;
	}
	if (bundle_getLockingThread(bundle) == NULL) {
		pthread_cond_broadcast(&framework->condition);
	}

	pthread_mutex_unlock(&framework->bundleLock);

	return true;
}

bool framework_acquireGlobalLock(FRAMEWORK framework) {
	pthread_mutex_lock(&framework->bundleLock);

	bool interrupted = false;

	while (!interrupted
			&& (framework->globalLockThread != NULL)
			&& (framework->globalLockThread != pthread_self())) {
		pthread_t currentThread = pthread_self();
		arrayList_add(framework->globalLockWaitersList, currentThread);
		pthread_cond_broadcast(&framework->condition);

		pthread_cond_wait(&framework->condition, &framework->bundleLock);
		if (framework->interrupted) {
			interrupted = true;
			framework->interrupted = false;
		}

		arrayList_removeElement(framework->globalLockWaitersList, currentThread);
	}

	if (!interrupted) {
		framework->globalLockCount++;
		framework->globalLockThread = pthread_self();
	}

	pthread_mutex_unlock(&framework->bundleLock);

	return !interrupted;
}

void framework_releaseGlobalLock(FRAMEWORK framework) {
	pthread_mutex_lock(&framework->bundleLock);

	if (framework->globalLockThread == pthread_self()) {
		framework->globalLockCount--;
		if (framework->globalLockCount == 0) {
			framework->globalLockThread = NULL;
			pthread_cond_broadcast(&framework->condition);
		}
	} else {
		printf("The current thread does not own the global lock");
	}

	pthread_mutex_unlock(&framework->bundleLock);
}

void framework_waitForStop(FRAMEWORK framework) {
	pthread_mutex_lock(&framework->mutex);
	pthread_cond_wait(&framework->shutdownGate, &framework->mutex);
	pthread_mutex_unlock(&framework->mutex);
}

void * framework_shutdown(void * framework) {
	FRAMEWORK fw = (FRAMEWORK) framework;

	HASH_MAP_ITERATOR iterator = hashMapIterator_create(fw->installedBundleMap);
	while (hashMapIterator_hasNext(iterator)) {
		BUNDLE bundle = hashMapIterator_nextValue(iterator);
		if (bundle_getState(bundle) == BUNDLE_ACTIVE || bundle_getState(bundle) == BUNDLE_STARTING) {
			fw_stopBundle(fw, bundle, 0);
		}
	}

	pthread_mutex_lock(&fw->mutex);
	pthread_cond_broadcast(&fw->shutdownGate);
	pthread_mutex_unlock(&fw->mutex);

	pthread_exit(NULL);
}

void bundleActivator_start(void * userData, BUNDLE_CONTEXT context) {
	// nothing to do
}

void bundleActivator_stop(void * userData, BUNDLE_CONTEXT context) {
	pthread_t shutdownThread;
	pthread_create(&shutdownThread, NULL, framework_shutdown, bundleContext_getFramework(context));
}

void bundleActivator_destroy(void * userData) {

}
