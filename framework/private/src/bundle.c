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

#include "framework_private.h"
#include "bundle_private.h"
#include "manifest.h"
#include "module.h"
#include "version.h"
#include "array_list.h"
#include "bundle_archive.h"
#include "bundle_revision.h"
#include "resolver.h"
#include "utils.h"
#include "celix_log.h"

celix_status_t bundle_createModule(bundle_pt bundle, module_pt *module);
celix_status_t bundle_closeRevisions(bundle_pt bundle);

celix_status_t bundle_create(bundle_pt * bundle, apr_pool_t *mp) {
    celix_status_t status = CELIX_SUCCESS;
    bundle_archive_pt archive = NULL;

	*bundle = (bundle_pt) apr_palloc(mp, sizeof(**bundle));
	if (*bundle == NULL) {
		return CELIX_ENOMEM;
	}
	status = bundleArchive_createSystemBundleArchive(mp, &archive);
	if (status == CELIX_SUCCESS) {
        module_pt module;
		apr_status_t apr_status;

		(*bundle)->memoryPool = mp;
        (*bundle)->archive = archive;
        (*bundle)->activator = NULL;
        (*bundle)->context = NULL;
        (*bundle)->framework = NULL;
        (*bundle)->state = OSGI_FRAMEWORK_BUNDLE_INSTALLED;
        (*bundle)->modules = NULL;
        arrayList_create(&(*bundle)->modules);

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
        }
	}

	framework_logIfError(status, NULL, "Failed to create bundle");

	return status;
}

celix_status_t bundle_createFromArchive(bundle_pt * bundle, framework_pt framework, bundle_archive_pt archive, apr_pool_t *bundlePool) {
    module_pt module;
	
	celix_status_t status = CELIX_SUCCESS;

	*bundle = (bundle_pt) apr_pcalloc(bundlePool, sizeof(**bundle));
	if (*bundle == NULL) {
		return CELIX_ENOMEM;
	}
	(*bundle)->memoryPool = bundlePool;
	(*bundle)->archive = archive;
	(*bundle)->activator = NULL;
	(*bundle)->context = NULL;
	(*bundle)->framework = framework;
	(*bundle)->state = OSGI_FRAMEWORK_BUNDLE_INSTALLED;
	(*bundle)->modules = NULL;
	arrayList_create(&(*bundle)->modules);

	
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

	framework_logIfError(status, NULL, "Failed to create bundle");

	return status;
}

celix_status_t bundle_destroy(bundle_pt bundle) {
	array_list_iterator_pt iter = arrayListIterator_create(bundle->modules);
	while (arrayListIterator_hasNext(iter)) {
		module_pt module = arrayListIterator_next(iter);
		module_destroy(module);
	}
	arrayListIterator_destroy(iter);
	arrayList_destroy(bundle->modules);
	apr_thread_mutex_destroy(bundle->lock);

	apr_pool_destroy(bundle->memoryPool);
	return CELIX_SUCCESS;
}

celix_status_t bundle_getArchive(bundle_pt bundle, bundle_archive_pt *archive) {
	celix_status_t status = CELIX_SUCCESS;

	if (bundle != NULL && *archive == NULL) {
		*archive = bundle->archive;
	} else {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	framework_logIfError(status, NULL, "Failed to get bundle archive");

	return status;
}

celix_status_t bundle_getCurrentModule(bundle_pt bundle, module_pt *module) {
	celix_status_t status = CELIX_SUCCESS;

	if (bundle == NULL || *module != NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		*module = arrayList_get(bundle->modules, arrayList_size(bundle->modules) - 1);
	}

	framework_logIfError(status, NULL, "Failed to get bundle module");

	return status;
}

array_list_pt bundle_getModules(bundle_pt bundle) {
    return bundle->modules;
}

void * bundle_getHandle(bundle_pt bundle) {
	return bundle->handle;
}

void bundle_setHandle(bundle_pt bundle, void * handle) {
	bundle->handle = handle;
}

activator_pt bundle_getActivator(bundle_pt bundle) {
	return bundle->activator;
}

celix_status_t bundle_setActivator(bundle_pt bundle, activator_pt activator) {
	bundle->activator = activator;
	return CELIX_SUCCESS;
}

celix_status_t bundle_getContext(bundle_pt bundle, bundle_context_pt *context) {
	*context = bundle->context;
	return CELIX_SUCCESS;
}

celix_status_t bundle_setContext(bundle_pt bundle, bundle_context_pt context) {
	bundle->context = context;
	return CELIX_SUCCESS;
}

celix_status_t bundle_getEntry(bundle_pt bundle, char * name, apr_pool_t *pool, char **entry) {
	return framework_getBundleEntry(bundle->framework, bundle, name, pool, entry);
}

celix_status_t bundle_getState(bundle_pt bundle, bundle_state_e *state) {
	*state = bundle->state;
	return CELIX_SUCCESS;
}

celix_status_t bundle_setState(bundle_pt bundle, bundle_state_e state) {
	bundle->state = state;
	return CELIX_SUCCESS;
}

celix_status_t bundle_createModule(bundle_pt bundle, module_pt *module) {
	celix_status_t status = CELIX_SUCCESS;
	bundle_archive_pt archive = NULL;
	bundle_revision_pt revision = NULL;
	manifest_pt headerMap = NULL;

	status = CELIX_DO_IF(status, bundle_getArchive(bundle, &archive));
	status = CELIX_DO_IF(status, bundleArchive_getCurrentRevision(archive, &revision));
	status = CELIX_DO_IF(status, bundleRevision_getManifest(revision, &headerMap));
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
				version_pt bundleVersion = module_getVersion(*module);
				char * symName = NULL;
				status = module_getSymbolicName(*module, &symName);
				if (status == CELIX_SUCCESS) {
					array_list_pt bundles = framework_getBundles(bundle->framework);
					unsigned int i;
					for (i = 0; i < arrayList_size(bundles); i++) {
						bundle_pt check = (bundle_pt) arrayList_get(bundles, i);

						long id;
						if (bundleArchive_getId(check->archive, &id) == CELIX_SUCCESS) {
							if (id != bundleId) {
								module_pt mod = NULL;
								char * sym = NULL;
								version_pt version;
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

	framework_logIfError(status, NULL, "Failed to create module");

	return status;
}

celix_status_t bundle_start(bundle_pt bundle) {
	return bundle_startWithOptions(bundle, 0);
}

celix_status_t bundle_startWithOptions(bundle_pt bundle, int options) {
	celix_status_t status = CELIX_SUCCESS;
    if (bundle != NULL) {
    	bool systemBundle = false;
    	status = bundle_isSystemBundle(bundle, &systemBundle);
    	if (status == CELIX_SUCCESS) {
    		if (systemBundle) {
    			framework_start(bundle->framework);
    		} else {
    			status = fw_startBundle(bundle->framework, bundle, options);
    		}
    	}
    }

    framework_logIfError(status, NULL, "Failed to start bundle");

    return status;
}

celix_status_t bundle_update(bundle_pt bundle, char *inputFile) {
	celix_status_t status = CELIX_SUCCESS;
	if (bundle != NULL) {
		bool systemBundle = false;
		status = bundle_isSystemBundle(bundle, &systemBundle);
		if (status == CELIX_SUCCESS) {
			if (systemBundle) {
				// #TODO: Support framework update
				status = CELIX_BUNDLE_EXCEPTION;
			} else {
				status = framework_updateBundle(bundle->framework, bundle, inputFile);
			}
		}
	}

	framework_logIfError(status, NULL, "Failed to update bundle");

	return status;
}

celix_status_t bundle_stop(bundle_pt bundle) {
	return bundle_stopWithOptions(bundle, 0);
}

celix_status_t bundle_stopWithOptions(bundle_pt bundle, int options) {
	celix_status_t status = CELIX_SUCCESS;
	if (bundle != NULL) {
		bool systemBundle = false;
		status = bundle_isSystemBundle(bundle, &systemBundle);
		if (status == CELIX_SUCCESS) {
			if (systemBundle) {
				framework_stop(bundle->framework);
			} else {
				status = fw_stopBundle(bundle->framework, bundle, options);
			}
		}
	}

	framework_logIfError(status, NULL, "Failed to stop bundle");

	return status;
}

celix_status_t bundle_uninstall(bundle_pt bundle) {
	celix_status_t status = CELIX_SUCCESS;
	if (bundle != NULL) {
		bool systemBundle = false;
		status = bundle_isSystemBundle(bundle, &systemBundle);
		if (status == CELIX_SUCCESS) {
			if (systemBundle) {
				status = CELIX_BUNDLE_EXCEPTION;
			} else {
				status = fw_uninstallBundle(bundle->framework, bundle);
			}
		}
	}

	framework_logIfError(status, NULL, "Failed to uninstall bundle");

	return status;
}

celix_status_t bundle_setPersistentStateInactive(bundle_pt bundle) {
	celix_status_t status = CELIX_SUCCESS;
	bool systemBundle;

	status = bundle_isSystemBundle(bundle, &systemBundle);
	if (status == CELIX_SUCCESS) {
		if (!systemBundle) {
			status = bundleArchive_setPersistentState(bundle->archive, OSGI_FRAMEWORK_BUNDLE_INSTALLED);
		}
	}

	framework_logIfError(status, NULL, "Failed to set persistent state to inactive");

	return status;
}

celix_status_t bundle_setPersistentStateUninstalled(bundle_pt bundle) {
	celix_status_t status = CELIX_SUCCESS;
	bool systemBundle;

	status = bundle_isSystemBundle(bundle, &systemBundle);
	if (status == CELIX_SUCCESS) {
		if (!systemBundle) {
			status = bundleArchive_setPersistentState(bundle->archive, OSGI_FRAMEWORK_BUNDLE_UNINSTALLED);
		}
	}

	framework_logIfError(status, NULL, "Failed to set persistent state to uninstalled");

    return status;
}

celix_status_t bundle_isUsed(bundle_pt bundle, bool *used) {
	bool unresolved = true;
	array_list_iterator_pt iter = arrayListIterator_create(bundle->modules);
	while (arrayListIterator_hasNext(iter)) {
		module_pt module = arrayListIterator_next(iter);
		if (module_isResolved(module)) {
			unresolved = false;
		}
	}
	arrayListIterator_destroy(iter);
	*used = false;
	iter = arrayListIterator_create(bundle->modules);
	while (arrayListIterator_hasNext(iter) && !unresolved && !*used) {
		module_pt module = arrayListIterator_next(iter);
		module_getDependents(module);
	}
	arrayListIterator_destroy(iter);
	return CELIX_SUCCESS;
}

celix_status_t bundle_revise(bundle_pt bundle, char * location, char *inputFile) {
	celix_status_t status = CELIX_SUCCESS;

	bundle_archive_pt archive = NULL;
	status = bundle_getArchive(bundle, &archive);
	if (status == CELIX_SUCCESS) {
		status = bundleArchive_revise(archive, location, inputFile);
		if (status == CELIX_SUCCESS) {
			module_pt module;
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

	framework_logIfError(status, NULL, "Failed to revise bundle");

	return status;
}

//bool bundle_rollbackRevise(bundle_pt bundle) {
//	module_pt module = arrayList_remove(bundle->modules, arrayList_set(bundle->modules) - 1);
//	return resolver_removeModule(module);
//}

celix_status_t bundle_addModule(bundle_pt bundle, module_pt module) {
	arrayList_add(bundle->modules, module);
	resolver_addModule(module);
	return CELIX_SUCCESS;
}

celix_status_t bundle_isSystemBundle(bundle_pt bundle, bool *systemBundle) {
	celix_status_t status = CELIX_SUCCESS;
	long bundleId;
	bundle_archive_pt archive = NULL;

	status = bundle_getArchive(bundle, &archive);
	if (status == CELIX_SUCCESS) {
		status = bundleArchive_getId(archive, &bundleId);
		if (status == CELIX_SUCCESS) {
			*systemBundle = (bundleId == 0);
		}
	}

	framework_logIfError(status, NULL, "Failed to check if bundle is the systembundle");

	return status;
}

celix_status_t bundle_isLockable(bundle_pt bundle, bool *lockable) {
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

	framework_logIfError(status, NULL, "Failed to check if bundle is lockable");

	return status;
}

celix_status_t bundle_getLockingThread(bundle_pt bundle, apr_os_thread_t *thread) {
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

	framework_logIfError(status, NULL, "Failed to get locking thread");

	return status;
}

celix_status_t bundle_lock(bundle_pt bundle, bool *locked) {
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

	framework_logIfError(status, NULL, "Failed to lock bundle");

	return status;
}

celix_status_t bundle_unlock(bundle_pt bundle, bool *unlocked) {
	celix_status_t status = CELIX_SUCCESS;

	bool equals;

	apr_thread_mutex_lock(bundle->lock);

	if (bundle->lockCount == 0) {
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

	framework_logIfError(status, NULL, "Failed to unlock bundle");

	return status;
}

celix_status_t bundle_close(bundle_pt bundle) {
	bundle_archive_pt archive = NULL;
	
	celix_status_t status = CELIX_SUCCESS;

    bundle_closeModules(bundle);
    bundle_closeRevisions(bundle);
    status = bundle_getArchive(bundle, &archive);
    if (status == CELIX_SUCCESS) {
		bundleArchive_close(archive);
    }

    framework_logIfError(status, NULL, "Failed to close bundle");

    return status;
}

celix_status_t bundle_closeAndDelete(bundle_pt bundle) {
	celix_status_t status = CELIX_SUCCESS;

	bundle_archive_pt archive = NULL;

    bundle_closeModules(bundle);
    bundle_closeRevisions(bundle);
    status = bundle_getArchive(bundle, &archive);
    if (status == CELIX_SUCCESS) {
    	bundleArchive_closeAndDelete(archive);
    }

    framework_logIfError(status, NULL, "Failed to close and delete bundle");

    return status;
}

celix_status_t bundle_closeRevisions(bundle_pt bundle) {
    celix_status_t status = CELIX_SUCCESS;

    // TODO implement this
    return status;
}

celix_status_t bundle_closeModules(bundle_pt bundle) {
    celix_status_t status = CELIX_SUCCESS;

    unsigned int i = 0;
    for (i = 0; i < arrayList_size(bundle->modules); i++) {
        module_pt module = (module_pt) arrayList_get(bundle->modules, i);
        resolver_removeModule(module);
        module_setWires(module, NULL);
    }

    return status;
}

celix_status_t bundle_refresh(bundle_pt bundle) {
	celix_status_t status = CELIX_SUCCESS;
	module_pt module;

	status = bundle_closeModules(bundle);
	if (status == CELIX_SUCCESS) {
		arrayList_clear(bundle->modules);
		status = bundle_createModule(bundle, &module);
		if (status == CELIX_SUCCESS) {
			status = bundle_addModule(bundle, module);
			if (status == CELIX_SUCCESS) {
				bundle->state = OSGI_FRAMEWORK_BUNDLE_INSTALLED;
			}
		}
	}

	framework_logIfError(status, NULL, "Failed to refresh bundle");

    return status;
}

celix_status_t bundle_getBundleId(bundle_pt bundle, long *id) {
	celix_status_t status = CELIX_SUCCESS;
	bundle_archive_pt archive = NULL;
	status = bundle_getArchive(bundle, &archive);
	if (status == CELIX_SUCCESS) {
		status = bundleArchive_getId(archive, id);
	}

	framework_logIfError(status, NULL, "Failed to get bundle id");

	return status;
}

celix_status_t bundle_getRegisteredServices(bundle_pt bundle, apr_pool_t *pool, array_list_pt *list) {
	celix_status_t status = CELIX_SUCCESS;

	status = fw_getBundleRegisteredServices(bundle->framework, pool, bundle, list);

	framework_logIfError(status, NULL, "Failed to get registered services");

	return status;
}

celix_status_t bundle_getServicesInUse(bundle_pt bundle, array_list_pt *list) {
	celix_status_t status = CELIX_SUCCESS;

	status = fw_getBundleServicesInUse(bundle->framework, bundle, list);

	framework_logIfError(status, NULL, "Failed to get in use services");

	return status;
}

celix_status_t bundle_getMemoryPool(bundle_pt bundle, apr_pool_t **pool) {
	celix_status_t status = CELIX_SUCCESS;

	if (bundle != NULL && *pool == NULL) {
		*pool = bundle->memoryPool;
	} else {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	framework_logIfError(status, NULL, "Failed to get memory pool");

	return status;
}

celix_status_t bundle_setFramework(bundle_pt bundle, framework_pt framework) {
	celix_status_t status = CELIX_SUCCESS;

	if (bundle != NULL && framework != NULL) {
		bundle->framework = framework;
	} else {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	framework_logIfError(status, NULL, "Failed to set framework");

	return status;
}

celix_status_t bundle_getFramework(bundle_pt bundle, framework_pt *framework) {
	celix_status_t status = CELIX_SUCCESS;

	if (bundle != NULL && *framework == NULL) {
		*framework = bundle->framework;
	} else {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	framework_logIfError(status, NULL, "Failed to get framework");

	return status;
}
