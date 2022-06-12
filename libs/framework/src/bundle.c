/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <stdlib.h>
#include <string.h>
#include <service_tracker.h>
#include <celix_constants.h>
#include <celix_api.h>
#include <assert.h>

#include "framework_private.h"
#include "bundle_private.h"
#include "resolver.h"
#include "utils.h"

#include "bundle_context_private.h"
#include "service_tracker_private.h"


celix_status_t bundle_createModule(bundle_pt bundle, module_pt *module);
celix_status_t bundle_closeRevisions(const_bundle_pt bundle);

celix_status_t bundle_create(bundle_pt * bundle) {
    celix_status_t status;
    bundle_archive_pt archive = NULL;

	*bundle = (bundle_pt) calloc(1, sizeof(**bundle));
	if (*bundle == NULL) {
		return CELIX_ENOMEM;
	}
	status = bundleArchive_createSystemBundleArchive(&archive);
	if (status == CELIX_SUCCESS) {
        module_pt module;

        (*bundle)->archive = archive;
        (*bundle)->activator = NULL;
        (*bundle)->context = NULL;
        (*bundle)->framework = NULL;
        (*bundle)->state = OSGI_FRAMEWORK_BUNDLE_INSTALLED;
        (*bundle)->modules = NULL;
        arrayList_create(&(*bundle)->modules);
        (*bundle)->handle = NULL;
        (*bundle)->manifest = NULL;

        module = module_createFrameworkModule((*bundle));
        bundle_addModule(*bundle, module);

	}
	framework_logIfError(celix_frameworkLogger_globalLogger(), status, NULL, "Failed to create bundle");

	return status;
}

celix_status_t bundle_createFromArchive(bundle_pt * bundle, framework_pt framework, bundle_archive_pt archive) {
    module_pt module;
	
	celix_status_t status;

	*bundle = (bundle_pt) calloc(1, (sizeof(**bundle)));
	if (*bundle == NULL) {
		return CELIX_ENOMEM;
	}
	(*bundle)->archive = archive;
	(*bundle)->activator = NULL;
	(*bundle)->context = NULL;
	(*bundle)->handle = NULL;
	(*bundle)->manifest = NULL;
	(*bundle)->framework = framework;
	(*bundle)->state = OSGI_FRAMEWORK_BUNDLE_INSTALLED;
	(*bundle)->modules = NULL;
	arrayList_create(&(*bundle)->modules);
	
	status = bundle_createModule(*bundle, &module);
	if (status == CELIX_SUCCESS) {
		bundle_addModule(*bundle, module);
	} else {
	    status = CELIX_FILE_IO_EXCEPTION;
	}

	framework_logIfError(celix_frameworkLogger_globalLogger(), status, NULL, "Failed to create bundle");

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

	free(bundle->symbolicName);
    free(bundle->name);
    free(bundle->group);
    free(bundle->description);
	free(bundle);

	return CELIX_SUCCESS;
}

celix_status_t bundle_getArchive(const_bundle_pt bundle, bundle_archive_pt *archive) {
	celix_status_t status = CELIX_SUCCESS;

	if (bundle != NULL && *archive == NULL) {
		*archive = bundle->archive;
	} else {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	framework_logIfError(bundle->framework->logger, status, NULL, "Failed to get bundle archive");

	return status;
}

celix_status_t bundle_getCurrentModule(const_bundle_pt bundle, module_pt *module) {
	celix_status_t status = CELIX_SUCCESS;

	if (bundle == NULL || arrayList_size(bundle->modules)==0 ) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		*module = arrayList_get(bundle->modules, arrayList_size(bundle->modules) - 1);
	}

	return status;
}

array_list_pt bundle_getModules(const_bundle_pt bundle) {
    return bundle->modules;
}

void * bundle_getHandle(bundle_pt bundle) {
	return bundle->handle;
}

void bundle_setHandle(bundle_pt bundle, void * handle) {
	bundle->handle = handle;
}

celix_bundle_activator_t* bundle_getActivator(const_bundle_pt bundle) {
	return bundle->activator;
}

celix_status_t bundle_setActivator(bundle_pt bundle, celix_bundle_activator_t *activator) {
	bundle->activator = activator;
	return CELIX_SUCCESS;
}

celix_status_t bundle_getContext(const_bundle_pt bundle, bundle_context_pt *context) {
	*context = bundle->context;
	return CELIX_SUCCESS;
}

celix_status_t bundle_setContext(bundle_pt bundle, bundle_context_pt context) {
	bundle->context = context;
	return CELIX_SUCCESS;
}

celix_status_t bundle_getEntry(const_bundle_pt bundle, const char* name, char** entry) {
	return framework_getBundleEntry(bundle->framework, bundle, name, entry);
}

celix_status_t bundle_getState(const_bundle_pt bundle, bundle_state_e *state) {
	if (bundle==NULL) {
		*state = OSGI_FRAMEWORK_BUNDLE_UNKNOWN;
		return CELIX_BUNDLE_EXCEPTION;
	}
    __atomic_load(&bundle->state, state, __ATOMIC_ACQUIRE);
	return CELIX_SUCCESS;
}

celix_status_t bundle_setState(bundle_pt bundle, bundle_state_e state) {
    __atomic_store_n(&bundle->state, state, __ATOMIC_RELEASE);
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
		long bundleId = 0;
        status = bundleArchive_getId(bundle->archive, &bundleId);
        if (status == CELIX_SUCCESS) {
			int revision_no = 0;
			char moduleId[512];

			snprintf(moduleId, sizeof(moduleId), "%ld.%d", bundleId, revision_no);
			*module = module_create(headerMap, moduleId, bundle);

			if (*module != NULL) {
				version_pt bundleVersion = module_getVersion(*module);
				const char * symName = NULL;
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
								const char * sym = NULL;
								version_pt version;
								int cmp;
								status = bundle_getCurrentModule(check, &mod);
								status = module_getSymbolicName(mod, &sym);

								version = module_getVersion(mod);
								version_compareTo(bundleVersion, version, &cmp);
								if ((symName != NULL) && (sym != NULL) && !strcmp(symName, sym) &&
										!cmp) {
									char *versionString = NULL;
									version_toString(version, &versionString);
									printf("Bundle symbolic name and version are not unique: %s:%s\n", sym, versionString);
									free(versionString);
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

	framework_logIfError(bundle->framework->logger, status, NULL, "Failed to create module");

	return status;
}

celix_status_t bundle_start(celix_bundle_t* bundle) {
    //note deprecated call use celix_bundleContext_startBundle instead
    return celix_framework_startBundle(bundle->framework, celix_bundle_getId(bundle));
}

celix_status_t bundle_update(bundle_pt bundle, const char *inputFile) {
    fw_log(bundle->framework->logger, CELIX_LOG_LEVEL_WARNING, "Bundle update functionality is deprecated. Do not use!");
    assert(!celix_framework_isCurrentThreadTheEventLoop(bundle->framework));
	celix_status_t status = CELIX_SUCCESS;
	if (bundle != NULL) {
		bool systemBundle = false;
		status = bundle_isSystemBundle(bundle, &systemBundle);
		if (status == CELIX_SUCCESS) {
			if (systemBundle) {
				status = CELIX_BUNDLE_EXCEPTION;
			} else {
				status = framework_updateBundle(bundle->framework, bundle, inputFile);
			}
		}
	}

	framework_logIfError(bundle->framework->logger, status, NULL, "Failed to update bundle");

	return status;
}


celix_status_t bundle_stop(bundle_pt bundle) {
    //note deprecated call use celix_bundleContext_startBundle instead
    return celix_framework_startBundle(bundle->framework, celix_bundle_getId(bundle));
}

celix_status_t bundle_uninstall(bundle_pt bundle) {
    //note deprecated call use celix_bundleContext_uninstallBundle instead
    return celix_framework_uninstallBundle(bundle->framework, celix_bundle_getId(bundle));
}

celix_status_t bundle_setPersistentStateInactive(bundle_pt bundle) {
	celix_status_t status;
	bool systemBundle;

	status = bundle_isSystemBundle(bundle, &systemBundle);
	if (status == CELIX_SUCCESS) {
		if (!systemBundle) {
			status = bundleArchive_setPersistentState(bundle->archive, OSGI_FRAMEWORK_BUNDLE_INSTALLED);
		}
	}

	framework_logIfError(bundle->framework->logger, status, NULL, "Failed to set persistent state to inactive");

	return status;
}

celix_status_t bundle_setPersistentStateUninstalled(bundle_pt bundle) {
	celix_status_t status;
	bool systemBundle;

	status = bundle_isSystemBundle(bundle, &systemBundle);
	if (status == CELIX_SUCCESS) {
		if (!systemBundle) {
			status = bundleArchive_setPersistentState(bundle->archive, OSGI_FRAMEWORK_BUNDLE_UNINSTALLED);
		}
	}

	framework_logIfError(bundle->framework->logger, status, NULL, "Failed to set persistent state to uninstalled");

    return status;
}

celix_status_t bundle_revise(bundle_pt bundle, const char * location, const char *inputFile) {
	celix_status_t status;

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

	framework_logIfError(bundle->framework->logger, status, NULL, "Failed to revise bundle");

	return status;
}

//bool bundle_rollbackRevise(bundle_pt bundle) {
//	module_pt module = arrayList_remove(bundle->modules, arrayList_set(bundle->modules) - 1);
//	return resolver_removeModule(module);
//}

celix_status_t bundle_addModule(bundle_pt bundle, module_pt module) {
	arrayList_add(bundle->modules, module);
	resolver_addModule(module);
	if (bundle->symbolicName == NULL) {
		const char *sn = NULL;
        const char *n = NULL;
        const char *g = NULL;
        const char *d = NULL;
		module_getSymbolicName(module, &sn);
        module_getGroup(module, &g);
        module_getName(module, &n);
        module_getDescription(module, &d);
		if (sn != NULL) {
            bundle->symbolicName = celix_utils_strdup(sn);
            bundle->name = celix_utils_strdup(n);
            bundle->group = celix_utils_strdup(g);
            bundle->description = celix_utils_strdup(d);
        }
	}

	return CELIX_SUCCESS;
}

celix_status_t bundle_isSystemBundle(const_bundle_pt bundle, bool *systemBundle) {
	celix_status_t status;
	long bundleId;
	bundle_archive_pt archive = NULL;

	status = bundle_getArchive(bundle, &archive);
	if (status == CELIX_SUCCESS) {
		status = bundleArchive_getId(archive, &bundleId);
		if (status == CELIX_SUCCESS) {
			*systemBundle = (bundleId == 0);
		}
	}

	framework_logIfError(bundle->framework->logger, status, NULL, "Failed to check if bundle is the systembundle");

	return status;
}

celix_status_t bundle_close(const_bundle_pt bundle) {
	bundle_archive_pt archive = NULL;
	
	celix_status_t status;

    bundle_closeModules(bundle);
    bundle_closeRevisions(bundle);
    status = bundle_getArchive(bundle, &archive);
    if (status == CELIX_SUCCESS) {
		bundleArchive_close(archive);
    }

	framework_logIfError(bundle->framework->logger, status, NULL, "Failed to close bundle");

    return status;
}

celix_status_t bundle_closeAndDelete(const_bundle_pt bundle) {
	celix_status_t status;

	bundle_archive_pt archive = NULL;

    bundle_closeModules(bundle);
    bundle_closeRevisions(bundle);
    status = bundle_getArchive(bundle, &archive);
    if (status == CELIX_SUCCESS) {
    	bundleArchive_closeAndDelete(archive);
    }

	framework_logIfError(bundle->framework->logger, status, NULL, "Failed to close and delete bundle");

    return status;
}

celix_status_t bundle_closeRevisions(const_bundle_pt bundle) {
    celix_status_t status = CELIX_SUCCESS;
    return status;
}

celix_status_t bundle_closeModules(const_bundle_pt bundle) {
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
	celix_status_t status;
	module_pt module;

	status = bundle_closeModules(bundle);
	if (status == CELIX_SUCCESS) {
		arrayList_clear(bundle->modules);
		status = bundle_createModule(bundle, &module);
		if (status == CELIX_SUCCESS) {
			status = bundle_addModule(bundle, module);
			if (status == CELIX_SUCCESS) {
                __atomic_store_n(&bundle->state, OSGI_FRAMEWORK_BUNDLE_INSTALLED, __ATOMIC_RELEASE);
			}
		}
	}

	framework_logIfError(bundle->framework->logger, status, NULL, "Failed to refresh bundle");

    return status;
}

celix_status_t bundle_getBundleId(const bundle_t *bundle, long *bndId) {
	celix_status_t status = CELIX_SUCCESS;
	long id = celix_bundle_getId(bundle);
	if (id >= 0) {
		*bndId = id;
	} else {
		status = CELIX_BUNDLE_EXCEPTION;
		*bndId = -1;
	}
	return status;
}

celix_status_t bundle_getRegisteredServices(bundle_pt bundle, array_list_pt *list) {
	celix_status_t status;

	status = fw_getBundleRegisteredServices(bundle->framework, bundle, list);

	framework_logIfError(bundle->framework->logger, status, NULL, "Failed to get registered services");

	return status;
}

celix_status_t bundle_getServicesInUse(bundle_pt bundle, array_list_pt *list) {
	celix_status_t status;

	status = fw_getBundleServicesInUse(bundle->framework, bundle, list);

	framework_logIfError(bundle->framework->logger, status, NULL, "Failed to get in use services");

	return status;
}

celix_status_t bundle_setFramework(bundle_pt bundle, framework_pt framework) {
	celix_status_t status = CELIX_SUCCESS;

	if (bundle != NULL && framework != NULL) {
		bundle->framework = framework;
	} else {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	framework_logIfError(bundle->framework->logger, status, NULL, "Failed to set framework");

	return status;
}

celix_status_t bundle_getFramework(const_bundle_pt bundle, framework_pt *framework) {
	celix_status_t status = CELIX_SUCCESS;

	if (bundle != NULL && *framework == NULL) {
		*framework = bundle->framework;
	} else {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	framework_logIfError(bundle->framework->logger, status, NULL, "Failed to get framework");

	return status;
}

celix_status_t bundle_getBundleLocation(const_bundle_pt bundle, const char **location){

	celix_status_t status;

	bundle_archive_pt archive = NULL;

	status = bundle_getArchive(bundle, &archive);
	if (status != CELIX_SUCCESS){
		printf("[ ERROR ]: Bundle - getBundleLocation (BundleArchive) \n");
		return status;
	}

	status =  bundleArchive_getLocation(archive, location);
	if (status != CELIX_SUCCESS){
		printf("[ ERROR ]:  Bundle - getBundleLocation (BundleArchiveLocation) \n");
		return status;
	}

	return CELIX_SUCCESS;
}






/**********************************************************************************************************************
 **********************************************************************************************************************
 * Updated API
 **********************************************************************************************************************
 **********************************************************************************************************************/

long celix_bundle_getId(const bundle_t* bnd) {
	long bndId = -1;
	bundle_archive_pt archive = NULL;
	bundle_getArchive((bundle_t*)bnd, &archive);
	if (archive != NULL) {
		bundleArchive_getId(archive, &bndId);
	}

	if (bndId < 0) {
		framework_logIfError(celix_frameworkLogger_globalLogger(), CELIX_BUNDLE_EXCEPTION, NULL, "Failed to get bundle id");
	}
	return bndId;
}

celix_bundle_state_e celix_bundle_getState(const bundle_t *bnd) {
    return __atomic_load_n(&bnd->state, __ATOMIC_ACQUIRE);
}

char* celix_bundle_getEntry(const bundle_t* bnd, const char *path) {
	char *entry = NULL;
	if (bnd != NULL && bnd->framework != NULL) {
		framework_getBundleEntry(bnd->framework, (celix_bundle_t*)bnd, path, &entry);
	}
	return entry;
}


const char* celix_bundle_getGroup(const celix_bundle_t *bnd) {
	return bnd->group;
}

const char* celix_bundle_getSymbolicName(const celix_bundle_t *bnd) {
	return bnd->symbolicName;
}

const char* celix_bundle_getName(const celix_bundle_t* bnd) {
    return bnd->name;
}

const char* celix_bundle_getDescription(const celix_bundle_t* bnd) {
    return bnd->description;
}

bool celix_bundle_isSystemBundle(const celix_bundle_t *bnd) {
    return bnd != NULL && celix_bundle_getId(bnd) == 0;
}

celix_array_list_t* celix_bundle_listRegisteredServices(const celix_bundle_t *bnd) {
    long bndId = celix_bundle_getId(bnd);
    celix_array_list_t* result = celix_arrayList_create();
    celix_array_list_t *svcIds = celix_serviceRegistry_listServiceIdsForOwner(bnd->framework->registry, bndId);
    for (int i = 0; i < celix_arrayList_size(svcIds); ++i) {
        long svcId = celix_arrayList_getLong(svcIds, i);
        celix_bundle_service_list_entry_t* entry = calloc(1, sizeof(*entry));
        entry->serviceId = svcId;
        entry->bundleOwner = bndId;
        celix_serviceRegistry_getServiceInfo(bnd->framework->registry, svcId, bndId, &entry->serviceName, &entry->serviceProperties, &entry->factory);
        celix_arrayList_add(result, entry);
    }
    celix_arrayList_destroy(svcIds);
    return result;
}

void celix_bundle_destroyRegisteredServicesList(celix_array_list_t* list) {
    if (list != NULL) {
        for (int i = 0; i < celix_arrayList_size(list); ++i) {
            celix_bundle_service_list_entry_t *entry = celix_arrayList_get(list, i);
            free(entry->serviceName);
            celix_properties_destroy(entry->serviceProperties);
            free(entry);
        }
        celix_arrayList_destroy(list);
    }
}

celix_array_list_t* celix_bundle_listServiceTrackers(const celix_bundle_t *bnd) {
    celix_array_list_t* result = celix_arrayList_create();
    celixThreadMutex_lock(&bnd->context->mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(bnd->context->serviceTrackers);
    while (hashMapIterator_hasNext(&iter)) {
        celix_bundle_context_service_tracker_entry_t *trkEntry = hashMapIterator_nextValue(&iter);
        if (trkEntry->tracker != NULL) {
            celix_bundle_service_tracker_list_entry_t *entry = calloc(1, sizeof(*entry));
            entry->filter = celix_utils_strdup(trkEntry->tracker->filter);
            entry->nrOfTrackedServices = serviceTracker_nrOfTrackedServices(trkEntry->tracker);
            entry->serviceName = celix_utils_strdup(trkEntry->tracker->serviceName);
            entry->bundleOwner = celix_bundle_getId(bnd);

            if (entry->serviceName != NULL) {
                celix_arrayList_add(result, entry);
            } else {
                framework_logIfError(bnd->framework->logger, CELIX_BUNDLE_EXCEPTION, NULL,
                                     "Failed to get service name from tracker. filter is %s", entry->filter);
                free(entry->filter);
                free(entry);
            }
        }
    }
    celixThreadMutex_unlock(&bnd->context->mutex);
    return result;
}


void celix_bundle_destroyServiceTrackerList(celix_array_list_t* list) {
    if (list != NULL) {
        for (int i = 0; i < celix_arrayList_size(list); ++i) {
            celix_bundle_service_tracker_list_entry_t *entry = celix_arrayList_get(list, i);
            free(entry->filter);
            free(entry->serviceName);
            free(entry);
        }
        celix_arrayList_destroy(list);
    }
}