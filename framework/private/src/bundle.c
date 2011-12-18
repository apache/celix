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
        (*bundle)->modules = NULL;
        arrayList_create(mp, &(*bundle)->modules);

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
	(*bundle)->modules = NULL;
	arrayList_create(bundlePool, &(*bundle)->modules);

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

celix_status_t bundle_getCurrentModule(BUNDLE bundle, MODULE *module) {
	celix_status_t status = CELIX_SUCCESS;

	if (bundle == NULL || *module != NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		*module = arrayList_get(bundle->modules, arrayList_size(bundle->modules) - 1);
	}

	return status;
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

celix_status_t bundle_setActivator(BUNDLE bundle, ACTIVATOR activator) {
	bundle->activator = activator;
	return CELIX_SUCCESS;
}

celix_status_t bundle_getContext(BUNDLE bundle, BUNDLE_CONTEXT *context) {
	*context = bundle->context;
	return CELIX_SUCCESS;
}

celix_status_t bundle_setContext(BUNDLE bundle, BUNDLE_CONTEXT context) {
	bundle->context = context;
	return CELIX_SUCCESS;
}

celix_status_t bundle_getEntry(BUNDLE bundle, char * name, apr_pool_t *pool, char **entry) {
	return framework_getBundleEntry(bundle->framework, bundle, name, pool, entry);
}

celix_status_t bundle_getState(BUNDLE bundle, BUNDLE_STATE *state) {
	*state = bundle->state;
	return CELIX_SUCCESS;
}

celix_status_t bundle_setState(BUNDLE bundle, BUNDLE_STATE state) {
	bundle->state = state;
	return CELIX_SUCCESS;
}

celix_status_t bundle_getManifest(BUNDLE bundle, MANIFEST *manifest) {
	*manifest = bundle->manifest;
	return CELIX_SUCCESS;
}

celix_status_t bundle_setManifest(BUNDLE bundle, MANIFEST manifest) {
	bundle->manifest = manifest;
	return CELIX_SUCCESS;
}

celix_status_t bundle_createModule(BUNDLE bundle, MODULE *module) {
	celix_status_t status = CELIX_SUCCESS;
	MANIFEST headerMap = NULL;
	status = getManifest(bundle->archive, bundle->memoryPool, &headerMap);
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
				char * symName = NULL;
				status = module_getSymbolicName(*module, &symName);
				if (status == CELIX_SUCCESS) {
					ARRAY_LIST bundles = framework_getBundles(bundle->framework);
					int i;
					for (i = 0; i < arrayList_size(bundles); i++) {
						BUNDLE check = (BUNDLE) arrayList_get(bundles, i);

						long id;
						if (bundleArchive_getId(check->archive, &id) == CELIX_SUCCESS) {
							if (id != bundleId) {
								MODULE mod = NULL;
								char * sym = NULL;
								status = bundle_getCurrentModule(check, &mod);
								status = module_getSymbolicName(mod, &sym);

								VERSION version = module_getVersion(mod);
								int cmp;
								version_compareTo(bundleVersion, version, &cmp);
								if ((symName != NULL) && (sym != NULL) && !strcmp(symName, sym) &&
										!cmp) {
									char *versionString = NULL;
									version_toString(version, bundle->memoryPool, &versionString);
									printf("Bundle symbolic name and version are not unique: %s:%s\n", sym, versionString);
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
	}

	return status;
}

celix_status_t bundle_start(BUNDLE bundle, int options) {
	celix_status_t status = CELIX_SUCCESS;
    if (bundle != NULL) {
        status = fw_startBundle(bundle->framework, bundle, options);
    }
    return status;
}

celix_status_t bundle_update(BUNDLE bundle, char *inputFile) {
	return framework_updateBundle(bundle->framework, bundle, inputFile);
}

celix_status_t bundle_stop(BUNDLE bundle, int options) {
	return fw_stopBundle(bundle->framework, bundle, ((options & 1) == 0));
}

celix_status_t bundle_uninstall(BUNDLE bundle) {
    return fw_uninstallBundle(bundle->framework, bundle);
}

celix_status_t bundle_setPersistentStateInactive(BUNDLE bundle) {
	celix_status_t status = CELIX_SUCCESS;
	bool systemBundle;

	status = bundle_isSystemBundle(bundle, &systemBundle);
	if (status == CELIX_SUCCESS) {
		if (!systemBundle) {
			status = bundleArchive_setPersistentState(bundle->archive, BUNDLE_INSTALLED);
		}
	}

	return status;
}

celix_status_t bundle_setPersistentStateUninstalled(BUNDLE bundle) {
	celix_status_t status = CELIX_SUCCESS;
	bool systemBundle;

	status = bundle_isSystemBundle(bundle, &systemBundle);
	if (status == CELIX_SUCCESS) {
		if (!systemBundle) {
			status = bundleArchive_setPersistentState(bundle->archive, BUNDLE_UNINSTALLED);
		}
	}

    return status;
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
		module_getDependents(module);
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

celix_status_t bundle_isSystemBundle(BUNDLE bundle, bool *systemBundle) {
	celix_status_t status = CELIX_SUCCESS;
	long bundleId;

	status = bundleArchive_getId(bundle_getArchive(bundle), &bundleId);
	if (status == CELIX_SUCCESS) {
		*systemBundle = (bundleId == 0);
	}

	return status;
}

celix_status_t bundle_isLockable(BUNDLE bundle, bool *lockable) {
	celix_status_t status = CELIX_SUCCESS;
	apr_status_t apr_status;

	apr_status = apr_thread_mutex_lock(bundle->lock);
	if (apr_status != APR_SUCCESS) {
		status = CELIX_BUNDLE_EXCEPTION;
	} else {
		bool equals;
		status = thread_equalsSelf(bundle->lockThread, &equals);
		if (status == CELIX_SUCCESS) {
			*lockable = (bundle->lockCount == 0) || (equals);
		}

		apr_status = apr_thread_mutex_unlock(bundle->lock);
		if (apr_status != APR_SUCCESS) {
			status = CELIX_BUNDLE_EXCEPTION;
		}
	}

	return status;
}

celix_status_t bundle_getLockingThread(BUNDLE bundle, apr_os_thread_t *thread) {
	celix_status_t status = CELIX_SUCCESS;
	apr_status_t apr_status;

	apr_status = apr_thread_mutex_lock(bundle->lock);
	if (apr_status != APR_SUCCESS) {
		status = CELIX_BUNDLE_EXCEPTION;
	} else {
		*thread = bundle->lockThread;

		apr_status = apr_thread_mutex_unlock(bundle->lock);
		if (apr_status != APR_SUCCESS) {
			status = CELIX_BUNDLE_EXCEPTION;
		}
	}

	return status;
}

celix_status_t bundle_lock(BUNDLE bundle, bool *locked) {
	celix_status_t status = CELIX_SUCCESS;
	bool equals;

	apr_thread_mutex_lock(bundle->lock);

	status = thread_equalsSelf(bundle->lockThread, &equals);
	if (status == CELIX_SUCCESS) {
		if ((bundle->lockCount > 0) && !equals) {
			*locked = false;
		} else {
			bundle->lockCount++;
			bundle->lockThread = apr_os_thread_current();
			*locked = true;
		}
	}

	apr_thread_mutex_unlock(bundle->lock);

	return status;
}

celix_status_t bundle_unlock(BUNDLE bundle, bool *unlocked) {
	celix_status_t status = CELIX_SUCCESS;

	bool equals;

	apr_thread_mutex_lock(bundle->lock);

	if ((bundle->lockCount == 0)) {
		*unlocked = false;
	} else {
		status = thread_equalsSelf(bundle->lockThread, &equals);
		if (status == CELIX_SUCCESS) {
			if ((bundle->lockCount > 0) && !equals) {
				return false;
			}
			bundle->lockCount--;
			if (bundle->lockCount == 0) {
				bundle->lockThread = NULL;
			}
			*unlocked = true;
		}
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

celix_status_t bundle_getRegisteredServices(BUNDLE bundle, apr_pool_t *pool, ARRAY_LIST *list) {
	celix_status_t status = CELIX_SUCCESS;

	status = fw_getBundleRegisteredServices(bundle->framework, pool, bundle, list);

	return status;
}

celix_status_t bundle_getServicesInUse(BUNDLE bundle, ARRAY_LIST *list) {
	celix_status_t status = CELIX_SUCCESS;

	status = fw_getBundleServicesInUse(bundle->framework, bundle, list);

	return status;
}
