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
 *  \date       Mar 23, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
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

struct bundle {
	bundle_context_t context;
	ACTIVATOR activator;
	bundle_state_e state;
	void * handle;
	bundle_archive_t archive;
	array_list_t modules;
	MANIFEST manifest;
	apr_pool_t *memoryPool;

	apr_thread_mutex_t *lock;
	int lockCount;
	apr_os_thread_t lockThread;

	struct framework * framework;
};

celix_status_t bundle_createModule(bundle_t bundle, module_t *module);
celix_status_t bundle_closeRevisions(bundle_t bundle);

celix_status_t bundle_create(bundle_t * bundle, apr_pool_t *mp) {
    celix_status_t status = CELIX_SUCCESS;
    bundle_archive_t archive = NULL;

	*bundle = (bundle_t) apr_palloc(mp, sizeof(**bundle));
	if (*bundle == NULL) {
		return CELIX_ENOMEM;
	}
	status = bundleArchive_createSystemBundleArchive(mp, &archive);
	if (status == CELIX_SUCCESS) {
        module_t module;
		apr_status_t apr_status;

		(*bundle)->memoryPool = mp;
        (*bundle)->archive = archive;
        (*bundle)->activator = NULL;
        (*bundle)->context = NULL;
        (*bundle)->framework = NULL;
        (*bundle)->state = BUNDLE_INSTALLED;
        (*bundle)->modules = NULL;
        arrayList_create(mp, &(*bundle)->modules);

        module = module_createFrameworkModule((*bundle));
        bundle_addModule(*bundle, module);
        // (*bundle)->module = module;

        apr_status = apr_thread_mutex_create(&(*bundle)->lock, APR_THREAD_MUTEX_UNNESTED, (*bundle)->memoryPool);
        if (apr_status != APR_SUCCESS) {
        	status = CELIX_ILLEGAL_STATE;
        } else {
			(*bundle)->lockCount = 0;
			(*bundle)->lockThread = 0;

			resolver_addModule(module);

			(*bundle)->manifest = NULL;
        }
	}

	return status;
}

celix_status_t bundle_createFromArchive(bundle_t * bundle, framework_t framework, bundle_archive_t archive, apr_pool_t *bundlePool) {
    module_t module;
	
	celix_status_t status = CELIX_SUCCESS;

	*bundle = (bundle_t) apr_pcalloc(bundlePool, sizeof(**bundle));
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

	
	status = bundle_createModule(*bundle, &module);
	if (status == CELIX_SUCCESS) {
        apr_status_t apr_status;
		
		bundle_addModule(*bundle, module);
        apr_status = apr_thread_mutex_create(&(*bundle)->lock, APR_THREAD_MUTEX_UNNESTED, (*bundle)->memoryPool);
        if (apr_status != APR_SUCCESS) {
			status = CELIX_ILLEGAL_STATE;
		} else {
			(*bundle)->lockCount = 0;
			(*bundle)->lockThread = 0;

			resolver_addModule(module);
		}
	} else {
	    status = CELIX_FILE_IO_EXCEPTION;
	}

	return status;
}

celix_status_t bundle_destroy(bundle_t bundle) {
	array_list_iterator_t iter = arrayListIterator_create(bundle->modules);
	while (arrayListIterator_hasNext(iter)) {
		module_t module = arrayListIterator_next(iter);
		module_destroy(module);
	}
	arrayListIterator_destroy(iter);
	arrayList_destroy(bundle->modules);
	return CELIX_SUCCESS;
}

celix_status_t bundle_getArchive(bundle_t bundle, bundle_archive_t *archive) {
	celix_status_t status = CELIX_SUCCESS;

	if (bundle != NULL && *archive == NULL) {
		*archive = bundle->archive;
	} else {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	return status;
}

celix_status_t bundle_getCurrentModule(bundle_t bundle, module_t *module) {
	celix_status_t status = CELIX_SUCCESS;

	if (bundle == NULL || *module != NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		*module = arrayList_get(bundle->modules, arrayList_size(bundle->modules) - 1);
	}

	return status;
}

array_list_t bundle_getModules(bundle_t bundle) {
    return bundle->modules;
}

void * bundle_getHandle(bundle_t bundle) {
	return bundle->handle;
}

void bundle_setHandle(bundle_t bundle, void * handle) {
	bundle->handle = handle;
}

ACTIVATOR bundle_getActivator(bundle_t bundle) {
	return bundle->activator;
}

celix_status_t bundle_setActivator(bundle_t bundle, ACTIVATOR activator) {
	bundle->activator = activator;
	return CELIX_SUCCESS;
}

celix_status_t bundle_getContext(bundle_t bundle, bundle_context_t *context) {
	*context = bundle->context;
	return CELIX_SUCCESS;
}

celix_status_t bundle_setContext(bundle_t bundle, bundle_context_t context) {
	bundle->context = context;
	return CELIX_SUCCESS;
}

celix_status_t bundle_getEntry(bundle_t bundle, char * name, apr_pool_t *pool, char **entry) {
	return framework_getBundleEntry(bundle->framework, bundle, name, pool, entry);
}

celix_status_t bundle_getState(bundle_t bundle, bundle_state_e *state) {
	*state = bundle->state;
	return CELIX_SUCCESS;
}

celix_status_t bundle_setState(bundle_t bundle, bundle_state_e state) {
	bundle->state = state;
	return CELIX_SUCCESS;
}

celix_status_t bundle_getManifest(bundle_t bundle, MANIFEST *manifest) {
	*manifest = bundle->manifest;
	return CELIX_SUCCESS;
}

celix_status_t bundle_setManifest(bundle_t bundle, MANIFEST manifest) {
	bundle->manifest = manifest;
	return CELIX_SUCCESS;
}

celix_status_t bundle_createModule(bundle_t bundle, module_t *module) {
	celix_status_t status = CELIX_SUCCESS;
	MANIFEST headerMap = NULL;
	status = getManifest(bundle->archive, bundle->memoryPool, &headerMap);
	if (status == CELIX_SUCCESS) {
        long bundleId;
        status = bundleArchive_getId(bundle->archive, &bundleId);
        if (status == CELIX_SUCCESS) {
			int revision = 0;
			char *moduleId = NULL;
			apr_pool_t *pool = NULL;


			apr_pool_create(&pool, bundle->memoryPool);
			moduleId = (char *) apr_palloc(pool, sizeof(bundleId) + sizeof(revision) +2);
			sprintf(moduleId, "%ld.%d", bundleId, revision);

			*module = module_create(headerMap, apr_pstrdup(bundle->memoryPool, moduleId), bundle);

			if (*module != NULL) {
				version_t bundleVersion = module_getVersion(*module);
				char * symName = NULL;
				status = module_getSymbolicName(*module, &symName);
				if (status == CELIX_SUCCESS) {
					array_list_t bundles = framework_getBundles(bundle->framework);
					unsigned int i;
					for (i = 0; i < arrayList_size(bundles); i++) {
						bundle_t check = (bundle_t) arrayList_get(bundles, i);

						long id;
						if (bundleArchive_getId(check->archive, &id) == CELIX_SUCCESS) {
							if (id != bundleId) {
								module_t mod = NULL;
								char * sym = NULL;
								version_t version;
								int cmp;
								status = bundle_getCurrentModule(check, &mod);
								status = module_getSymbolicName(mod, &sym);

								version = module_getVersion(mod);
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
			apr_pool_destroy(pool);
        }
	}

	return status;
}

celix_status_t bundle_start(bundle_t bundle, int options) {
	celix_status_t status = CELIX_SUCCESS;
    if (bundle != NULL) {
        status = fw_startBundle(bundle->framework, bundle, options);
    }
    return status;
}

celix_status_t bundle_update(bundle_t bundle, char *inputFile) {
	return framework_updateBundle(bundle->framework, bundle, inputFile);
}

celix_status_t bundle_stop(bundle_t bundle, int options) {
	return fw_stopBundle(bundle->framework, bundle, ((options & 1) == 0));
}

celix_status_t bundle_uninstall(bundle_t bundle) {
    return fw_uninstallBundle(bundle->framework, bundle);
}

celix_status_t bundle_setPersistentStateInactive(bundle_t bundle) {
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

celix_status_t bundle_setPersistentStateUninstalled(bundle_t bundle) {
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

celix_status_t bundle_isUsed(bundle_t bundle, bool *used) {
	bool unresolved = true;
	array_list_iterator_t iter = arrayListIterator_create(bundle->modules);
	while (arrayListIterator_hasNext(iter)) {
		module_t module = arrayListIterator_next(iter);
		if (module_isResolved(module)) {
			unresolved = false;
		}
	}
	arrayListIterator_destroy(iter);
	*used = false;
	iter = arrayListIterator_create(bundle->modules);
	while (arrayListIterator_hasNext(iter) && !unresolved && !*used) {
		module_t module = arrayListIterator_next(iter);
		module_getDependents(module);
	}
	arrayListIterator_destroy(iter);
	return CELIX_SUCCESS;
}

celix_status_t bundle_revise(bundle_t bundle, char * location, char *inputFile) {
	celix_status_t status = CELIX_SUCCESS;

	bundle_archive_t archive = NULL;
	status = bundle_getArchive(bundle, &archive);
	if (status == CELIX_SUCCESS) {
		status = bundleArchive_revise(archive, location, inputFile);
		if (status == CELIX_SUCCESS) {
			module_t module;
			status = bundle_createModule(bundle, &module);
			if (status == CELIX_SUCCESS) {
				status = bundle_addModule(bundle, module);
			} else {
				bool rolledback;
				status = bundleArchive_rollbackRevise(archive, &rolledback);
				if (status == CELIX_SUCCESS) {
					status = CELIX_BUNDLE_EXCEPTION;
				}
			}
		}
	}
	return status;
}

//bool bundle_rollbackRevise(bundle_t bundle) {
//	module_t module = arrayList_remove(bundle->modules, arrayList_set(bundle->modules) - 1);
//	return resolver_removeModule(module);
//}

celix_status_t bundle_addModule(bundle_t bundle, module_t module) {
	arrayList_add(bundle->modules, module);
	resolver_addModule(module);
	return CELIX_SUCCESS;
}

celix_status_t bundle_isSystemBundle(bundle_t bundle, bool *systemBundle) {
	celix_status_t status = CELIX_SUCCESS;
	long bundleId;
	bundle_archive_t archive = NULL;

	status = bundle_getArchive(bundle, &archive);
	if (status == CELIX_SUCCESS) {
		status = bundleArchive_getId(archive, &bundleId);
		if (status == CELIX_SUCCESS) {
			*systemBundle = (bundleId == 0);
		}
	}

	return status;
}

celix_status_t bundle_isLockable(bundle_t bundle, bool *lockable) {
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

celix_status_t bundle_getLockingThread(bundle_t bundle, apr_os_thread_t *thread) {
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

celix_status_t bundle_lock(bundle_t bundle, bool *locked) {
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

celix_status_t bundle_unlock(bundle_t bundle, bool *unlocked) {
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
				bundle->lockThread = 0;
			}
			*unlocked = true;
		}
	}

	apr_thread_mutex_unlock(bundle->lock);

	return status;
}

celix_status_t bundle_close(bundle_t bundle) {
	bundle_archive_t archive = NULL;
	
	celix_status_t status = CELIX_SUCCESS;

    bundle_closeModules(bundle);
    bundle_closeRevisions(bundle);
    status = bundle_getArchive(bundle, &archive);
    if (status == CELIX_SUCCESS) {
		bundleArchive_close(archive);
    }

    return status;
}

celix_status_t bundle_closeAndDelete(bundle_t bundle) {
	celix_status_t status = CELIX_SUCCESS;

	bundle_archive_t archive = NULL;

    bundle_closeModules(bundle);
    bundle_closeRevisions(bundle);
    status = bundle_getArchive(bundle, &archive);
    if (status == CELIX_SUCCESS) {
    	bundleArchive_closeAndDelete(archive);
    }

    return status;
}

celix_status_t bundle_closeRevisions(bundle_t bundle) {
    celix_status_t status = CELIX_SUCCESS;

    // TODO implement this
    return status;
}

celix_status_t bundle_closeModules(bundle_t bundle) {
    celix_status_t status = CELIX_SUCCESS;

    unsigned int i = 0;
    for (i = 0; i < arrayList_size(bundle->modules); i++) {
        module_t module = (module_t) arrayList_get(bundle->modules, i);
        resolver_removeModule(module);
        module_setWires(module, NULL);
    }

    return status;
}

celix_status_t bundle_refresh(bundle_t bundle) {
	celix_status_t status = CELIX_SUCCESS;
	module_t module;

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

celix_status_t bundle_getBundleId(bundle_t bundle, long *id) {
	celix_status_t status = CELIX_SUCCESS;
	bundle_archive_t archive = NULL;
	status = bundle_getArchive(bundle, &archive);
	if (status == CELIX_SUCCESS) {
		status = bundleArchive_getId(archive, id);
	}
	return status;
}

celix_status_t bundle_getRegisteredServices(bundle_t bundle, apr_pool_t *pool, array_list_t *list) {
	celix_status_t status = CELIX_SUCCESS;

	status = fw_getBundleRegisteredServices(bundle->framework, pool, bundle, list);

	return status;
}

celix_status_t bundle_getServicesInUse(bundle_t bundle, array_list_t *list) {
	celix_status_t status = CELIX_SUCCESS;

	status = fw_getBundleServicesInUse(bundle->framework, bundle, list);

	return status;
}

celix_status_t bundle_getMemoryPool(bundle_t bundle, apr_pool_t **pool) {
	celix_status_t status = CELIX_SUCCESS;

	if (bundle != NULL && *pool == NULL) {
		*pool = bundle->memoryPool;
	} else {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	return status;
}

celix_status_t bundle_setFramework(bundle_t bundle, framework_t framework) {
	celix_status_t status = CELIX_SUCCESS;

	if (bundle != NULL && framework != NULL) {
		bundle->framework = framework;
	} else {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	return status;
}

celix_status_t bundle_getFramework(bundle_t bundle, framework_t *framework) {
	celix_status_t status = CELIX_SUCCESS;

	if (bundle != NULL && *framework == NULL) {
		*framework = bundle->framework;
	} else {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	return status;
}
