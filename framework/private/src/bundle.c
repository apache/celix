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
#include <stdlib.h>
#include <string.h>
#include <apr_strings.h>
#include <apr_portable.h>
#include <apr_thread_proc.h>

#include "bundle.h"
#include "framework.h"
#include "manifest.h"
#include "module.h"
#include "version.h"
#include "array_list.h"
#include "bundle_archive.h"
#include "resolver.h"
#include "utils.h"

celix_status_t bundle_createModule(BUNDLE bundle, MODULE *module);
celix_status_t bundle_closeRevisions(BUNDLE bundle);

celix_status_t bundle_create(BUNDLE * bundle, apr_pool_t *mp) {
    celix_status_t status = CELIX_SUCCESS;
    BUNDLE_ARCHIVE archive = NULL;

	*bundle = (BUNDLE) apr_palloc(mp, sizeof(**bundle));
	if (*bundle == NULL) {
		return CELIX_ENOMEM;
	}
	status = bundleArchive_createSystemBundleArchive(mp, &archive);
	if (status == CELIX_SUCCESS) {
        (*bundle)->memoryPool = mp;
        (*bundle)->archive = archive;
        (*bundle)->activator = NULL;
        (*bundle)->context = NULL;
        (*bundle)->framework = NULL;
        (*bundle)->state = BUNDLE_INSTALLED;
        (*bundle)->modules = arrayList_create();

        MODULE module = module_createFrameworkModule((*bundle));
        bundle_addModule(*bundle, module);
        // (*bundle)->module = module;

        apr_status_t apr_status = apr_thread_mutex_create(&(*bundle)->lock, APR_THREAD_MUTEX_UNNESTED, (*bundle)->memoryPool);
        if (apr_status != APR_SUCCESS) {
        	status = CELIX_ILLEGAL_STATE;
        } else {
			(*bundle)->lockCount = 0;
			(*bundle)->lockThread = NULL;

			resolver_addModule(module);

			(*bundle)->manifest = NULL;
        }
	}

	return status;
}

celix_status_t bundle_createFromArchive(BUNDLE * bundle, FRAMEWORK framework, BUNDLE_ARCHIVE archive, apr_pool_t *bundlePool) {
    celix_status_t status = CELIX_SUCCESS;

    *bundle = (BUNDLE) apr_pcalloc(bundlePool, sizeof(**bundle));
	if (*bundle == NULL) {
		return CELIX_ENOMEM;
	}
	(*bundle)->memoryPool = bundlePool;
	(*bundle)->archive = archive;
	(*bundle)->activator = NULL;
	(*bundle)->context = NULL;
	(*bundle)->framework = framework;
	(*bundle)->state = BUNDLE_INSTALLED;
	(*bundle)->modules = arrayList_create();

	MODULE module;
	status = bundle_createModule(*bundle, &module);
	if (status == CELIX_SUCCESS) {
        bundle_addModule(*bundle, module);

        apr_status_t apr_status = apr_thread_mutex_create(&(*bundle)->lock, APR_THREAD_MUTEX_UNNESTED, (*bundle)->memoryPool);
        if (apr_status != APR_SUCCESS) {
			status = CELIX_ILLEGAL_STATE;
		} else {
			(*bundle)->lockCount = 0;
			(*bundle)->lockThread = NULL;

			resolver_addModule(module);
		}
	} else {
	    status = CELIX_FILE_IO_EXCEPTION;
	}

	return status;
}

celix_status_t bundle_destroy(BUNDLE bundle) {
	ARRAY_LIST_ITERATOR iter = arrayListIterator_create(bundle->modules);
	while (arrayListIterator_hasNext(iter)) {
		MODULE module = arrayListIterator_next(iter);
		module_destroy(module);
	}
	arrayListIterator_destroy(iter);
	arrayList_destroy(bundle->modules);
	return CELIX_SUCCESS;
}

BUNDLE_ARCHIVE bundle_getArchive(BUNDLE bundle) {
	return bundle->archive;
}

MODULE bundle_getCurrentModule(BUNDLE bundle) {
	return arrayList_get(bundle->modules, arrayList_size(bundle->modules) - 1);
}

ARRAY_LIST bundle_getModules(BUNDLE bundle) {
    return bundle->modules;
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

celix_status_t bundle_getEntry(BUNDLE bundle, char * name, apr_pool_t *pool, char **entry) {
	return framework_getBundleEntry(bundle->framework, bundle, name, pool, entry);
}

BUNDLE_STATE bundle_getState(BUNDLE bundle) {
	return bundle->state;
}

void bundle_setState(BUNDLE bundle, BUNDLE_STATE state) {
	bundle->state = state;
}

MANIFEST bundle_getManifest(BUNDLE bundle) {
	return bundle->manifest;
}

void bundle_setManifest(BUNDLE bundle, MANIFEST manifest) {
	bundle->manifest = manifest;
}

celix_status_t bundle_createModule(BUNDLE bundle, MODULE *module) {
	celix_status_t status = CELIX_SUCCESS;
	MANIFEST headerMap = NULL;
	status = getManifest(bundle->archive, &headerMap);
	if (status == CELIX_SUCCESS) {
        long bundleId;
        status = bundleArchive_getId(bundle->archive, &bundleId);
        if (status == CELIX_SUCCESS) {
			int revision = 0;
			char moduleId[sizeof(bundleId) + sizeof(revision) + 2];
			sprintf(moduleId, "%ld.%d", bundleId, revision);

			*module = module_create(headerMap, apr_pstrdup(bundle->memoryPool, moduleId), bundle);

			if (*module != NULL) {
				VERSION bundleVersion = module_getVersion(*module);
				char * symName = module_getSymbolicName(*module);

				ARRAY_LIST bundles = framework_getBundles(bundle->framework);
				int i;
				for (i = 0; i < arrayList_size(bundles); i++) {
					BUNDLE check = (BUNDLE) arrayList_get(bundles, i);

					long id;
					if (bundleArchive_getId(check->archive, &id) == CELIX_SUCCESS) {
						if (id != bundleId) {
							char * sym = module_getSymbolicName(bundle_getCurrentModule(check));
							VERSION version = module_getVersion(bundle_getCurrentModule(check));
							if ((symName != NULL) && (sym != NULL) && !strcmp(symName, sym) &&
									!version_compareTo(bundleVersion, version)) {
								printf("Bundle symbolic name and version are not unique: %s:%s\n", sym, version_toString(version));
								status = CELIX_BUNDLE_EXCEPTION;
								break;
							}
						}
					}
				}
				arrayList_destroy(bundles);
			}
        }
	}

	return status;
}

void startBundle(BUNDLE bundle, int options) {
    if (bundle != NULL) {
        fw_startBundle(bundle->framework, bundle, options);
    }
}

celix_status_t bundle_update(BUNDLE bundle, char *inputFile) {
	return framework_updateBundle(bundle->framework, bundle, inputFile);
}

void stopBundle(BUNDLE bundle, int options) {
	fw_stopBundle(bundle->framework, bundle, ((options & 1) == 0));
}

celix_status_t bundle_uninstall(BUNDLE bundle) {
    celix_status_t status = CELIX_SUCCESS;

    fw_uninstallBundle(bundle->framework, bundle);

    return status;
}

celix_status_t bundle_setPersistentStateInactive(BUNDLE bundle) {
	if (!bundle_isSystemBundle(bundle)) {
		bundleArchive_setPersistentState(bundle->archive, BUNDLE_INSTALLED);
	}
	return CELIX_SUCCESS;
}

celix_status_t bundle_setPersistentStateUninstalled(BUNDLE bundle) {
    if (!bundle_isSystemBundle(bundle)) {
        bundleArchive_setPersistentState(bundle->archive, BUNDLE_UNINSTALLED);
    }
    return CELIX_SUCCESS;
}

celix_status_t bundle_isUsed(BUNDLE bundle, bool *used) {
	bool unresolved = true;
	ARRAY_LIST_ITERATOR iter = arrayListIterator_create(bundle->modules);
	while (arrayListIterator_hasNext(iter)) {
		MODULE module = arrayListIterator_next(iter);
		if (module_isResolved(module)) {
			unresolved = false;
		}
	}
	arrayListIterator_destroy(iter);
	*used = false;
	iter = arrayListIterator_create(bundle->modules);
	while (arrayListIterator_hasNext(iter) && !unresolved && !*used) {
		MODULE module = arrayListIterator_next(iter);
//		module_getD
	}
	arrayListIterator_destroy(iter);
	return CELIX_SUCCESS;
}

celix_status_t bundle_revise(BUNDLE bundle, char * location, char *inputFile) {
	celix_status_t status = CELIX_SUCCESS;
	status = bundleArchive_revise(bundle_getArchive(bundle), location, inputFile);
	if (status == CELIX_SUCCESS) {
		MODULE module;
		status = bundle_createModule(bundle, &module);
		if (status == CELIX_SUCCESS) {
			status = bundle_addModule(bundle, module);
		} else {
			bool rolledback;
			status = bundleArchive_rollbackRevise(bundle_getArchive(bundle), &rolledback);
			if (status == CELIX_SUCCESS) {
				status = CELIX_BUNDLE_EXCEPTION;
			}
		}
	}
	return status;
}

//bool bundle_rollbackRevise(BUNDLE bundle) {
//	MODULE module = arrayList_remove(bundle->modules, arrayList_set(bundle->modules) - 1);
//	return resolver_removeModule(module);
//}

celix_status_t bundle_addModule(BUNDLE bundle, MODULE module) {
	arrayList_add(bundle->modules, module);
	resolver_addModule(module);
	return CELIX_SUCCESS;
}

bool bundle_isSystemBundle(BUNDLE bundle) {
	long bundleId;
	bundleArchive_getId(bundle_getArchive(bundle), &bundleId);
	return bundleId == 0;
}

bool bundle_isLockable(BUNDLE bundle) {
	bool lockable = false;
	apr_thread_mutex_lock(bundle->lock);

	lockable = (bundle->lockCount == 0) || (bundle->lockThread == pthread_self());

	apr_thread_mutex_unlock(bundle->lock);

	return lockable;
}

pthread_t bundle_getLockingThread(BUNDLE bundle) {
	pthread_t lockingThread = NULL;
	apr_thread_mutex_lock(bundle->lock);

	lockingThread = bundle->lockThread;

	apr_thread_mutex_unlock(bundle->lock);

	return lockingThread;
}

bool bundle_lock(BUNDLE bundle) {
	apr_thread_mutex_lock(bundle->lock);
	bool equals;

	thread_equalsSelf(bundle->lockThread, &equals);
	if ((bundle->lockCount > 0) && !equals) {
		apr_thread_mutex_unlock(bundle->lock);
		return false;
	}
	bundle->lockCount++;
	bundle->lockThread = apr_os_thread_current();

	apr_thread_mutex_unlock(bundle->lock);
	return true;
}

celix_status_t bundle_unlock(BUNDLE bundle, bool *unlocked) {
	celix_status_t status = CELIX_SUCCESS;

	bool equals;

	apr_thread_mutex_lock(bundle->lock);

	if ((bundle->lockCount == 0)) {
		*unlocked = false;
	} else {
		thread_equalsSelf(bundle->lockThread, &equals);
		if ((bundle->lockCount > 0) && !equals) {
			return false;
		}
		bundle->lockCount--;
		if (bundle->lockCount == 0) {
			bundle->lockThread = NULL;
		}
		*unlocked = true;
	}

	apr_thread_mutex_unlock(bundle->lock);

	return status;
}

celix_status_t bundle_close(BUNDLE bundle) {
    celix_status_t status = CELIX_SUCCESS;

    bundle_closeModules(bundle);
    bundle_closeRevisions(bundle);
    BUNDLE_ARCHIVE archive = bundle_getArchive(bundle);
    bundleArchive_close(archive);

    return status;
}

celix_status_t bundle_closeAndDelete(BUNDLE bundle) {
    celix_status_t status = CELIX_SUCCESS;

    bundle_closeModules(bundle);
    bundle_closeRevisions(bundle);
    BUNDLE_ARCHIVE archive = bundle_getArchive(bundle);
    bundleArchive_closeAndDelete(archive);

    return status;
}

celix_status_t bundle_closeRevisions(BUNDLE bundle) {
    celix_status_t status = CELIX_SUCCESS;

    // TODO implement this
    return status;
}

celix_status_t bundle_closeModules(BUNDLE bundle) {
    celix_status_t status = CELIX_SUCCESS;

    int i = 0;
    for (i = 0; i < arrayList_size(bundle->modules); i++) {
        MODULE module = arrayList_get(bundle->modules, i);
        resolver_removeModule(module);
        module_setWires(module, NULL);
    }

    return status;
}

celix_status_t bundle_refresh(BUNDLE bundle) {
	celix_status_t status = CELIX_SUCCESS;
	MODULE module;

	status = bundle_closeModules(bundle);
	if (status == CELIX_SUCCESS) {
		arrayList_clear(bundle->modules);
		status = bundle_createModule(bundle, &module);
		if (status == CELIX_SUCCESS) {
			status = bundle_addModule(bundle, module);
			if (status == CELIX_SUCCESS) {
				bundle->state = BUNDLE_INSTALLED;
			}
		}
	}
    return status;
}

celix_status_t bundle_getBundleId(BUNDLE bundle, long *id) {
	celix_status_t status = CELIX_SUCCESS;
	status = bundleArchive_getId(bundle_getArchive(bundle), id);
	return status;
}


