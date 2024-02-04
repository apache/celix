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

#include "bundle_private.h"

#include <stdlib.h>
#include <string.h>
#include <service_tracker.h>
#include <celix_constants.h>
#include <assert.h>
#include <unistd.h>

#include "framework_private.h"
#include "utils.h"
#include "celix_file_utils.h"
#include "bundle_archive_private.h"
#include "bundle_context_private.h"
#include "service_tracker_private.h"


static char* celix_bundle_getBundleOrPersistentStoreEntry(const celix_bundle_t* bnd, bool bundleEntry, const char* name);
celix_status_t bundle_createModule(bundle_pt bundle, module_pt *module);
celix_status_t bundle_closeRevisions(const_bundle_pt bundle);

celix_status_t celix_bundle_createFromArchive(celix_framework_t *framework, bundle_archive_pt archive, celix_bundle_t **bundleOut) {
    celix_status_t status = CELIX_SUCCESS;
    celix_bundle_t* bundle = calloc(1, sizeof(*bundle));

    if (!bundle) {
        status = CELIX_ENOMEM;
        fw_logCode(framework->logger, CELIX_LOG_LEVEL_ERROR, status, "Cannot create bundle from archive, out of memory.");
        return status;
    }

    bundle->framework = framework;
    bundle->archive = archive;
    bundle->modules = celix_arrayList_create();
    bundle->state = OSGI_FRAMEWORK_BUNDLE_INSTALLED;
    bundle->handle = NULL;
    bundle->activator = NULL;
    bundle->context = NULL;
    bundle->symbolicName = NULL;
    bundle->name = NULL;
    bundle->group = NULL;
    bundle->description = NULL;

    if (bundle->modules == NULL) {
        status = CELIX_ENOMEM;
        fw_logCode(framework->logger, CELIX_LOG_LEVEL_ERROR, status, "Cannot create bundle from archive, out of memory.");
        free(bundle);
        return status;
    }

    module_pt module;
    status = bundle_createModule(bundle, &module);
    if (status != CELIX_SUCCESS) {
        fw_logCode(framework->logger, CELIX_LOG_LEVEL_ERROR, status, "Cannot create bundle from archive, cannot create module.");
        celix_arrayList_destroy(bundle->modules);
        free(bundle);
        return status;
    } else {
        bundle_addModule(bundle, module);
    }

    *bundleOut = bundle;
	return status;
}

celix_status_t bundle_destroy(bundle_pt bundle) {
    for (int i = 0; i < celix_arrayList_size(bundle->modules); ++i) {
        module_pt module = celix_arrayList_get(bundle->modules, i);
        module_destroy(module);
    }
    celix_arrayList_destroy(bundle->modules);

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
	return status;
}

celix_status_t bundle_getCurrentModule(const_bundle_pt bundle, module_pt *module) {
	celix_status_t status = CELIX_SUCCESS;

	if (bundle == NULL || celix_arrayList_size(bundle->modules)==0 ) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		*module = celix_arrayList_get(bundle->modules, celix_arrayList_size(bundle->modules) - 1);
	}

	return status;
}

celix_array_list_t* bundle_getModules(const_bundle_pt bundle) {
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
	*entry = celix_bundle_getBundleOrPersistentStoreEntry(bundle, true, name);
    return *entry == NULL ? CELIX_ILLEGAL_ARGUMENT : CELIX_SUCCESS;
}

celix_status_t bundle_getState(const_bundle_pt bundle, bundle_state_e *state) {
	if (bundle==NULL) {
		*state = CELIX_BUNDLE_STATE_UNKNOWN;
		return CELIX_BUNDLE_EXCEPTION;
	}
    __atomic_load(&bundle->state, state, __ATOMIC_ACQUIRE);
	return CELIX_SUCCESS;
}

celix_status_t bundle_setState(bundle_pt bundle, bundle_state_e state) {
    __atomic_store_n(&bundle->state, state, __ATOMIC_RELEASE);
	return CELIX_SUCCESS;
}

celix_status_t bundle_createModule(bundle_pt bundle, module_pt* moduleOut) {
	celix_status_t status = CELIX_SUCCESS;
	bundle_archive_pt archive = NULL;
	bundle_revision_pt revision = NULL;
	manifest_pt headerMap = NULL;
    long bundleId = 0;

	status = CELIX_DO_IF(status, bundle_getArchive(bundle, &archive));
	status = CELIX_DO_IF(status, bundleArchive_getCurrentRevision(archive, &revision));
	status = CELIX_DO_IF(status, bundleRevision_getManifest(revision, &headerMap));
    status = bundleArchive_getId(bundle->archive, &bundleId);

    if (status != CELIX_SUCCESS) {
        fw_logCode(bundle->framework->logger, CELIX_LOG_LEVEL_ERROR, status, "Cannot create module, cannot get bundle archive, revision, manifest or bundle id.");
        return status;
    }

    module_pt module = NULL;
    if (bundleId == CELIX_FRAMEWORK_BUNDLE_ID) {
        module = module_createFrameworkModule(bundle->framework, bundle);
    } else {
        int revision_no = 0;
        char moduleId[512];
        snprintf(moduleId, sizeof(moduleId), "%ld.%d", bundleId, revision_no);
        module = module_create(headerMap, moduleId, bundle);
    }
    if (!module) {
        status = CELIX_BUNDLE_EXCEPTION;
        fw_logCode(bundle->framework->logger, CELIX_LOG_LEVEL_ERROR, status, "Cannot create module.");
        return status;
    }


    const char * symName = NULL;
    status = module_getSymbolicName(module, &symName);
    assert(status == CELIX_SUCCESS);
    /*
     * NOTE only allowing a single bundle with a symbolic name.
     * OSGi spec allows same symbolic name and different versions, but this is risky with
     * the behaviour of dlopen when opening shared libraries with the same SONAME.
     */
    bool alreadyInstalled = celix_framework_isBundleAlreadyInstalled(bundle->framework, symName);
    if (alreadyInstalled) {
        status = CELIX_BUNDLE_EXCEPTION;
        fw_logCode(bundle->framework->logger, CELIX_LOG_LEVEL_ERROR, status, "Cannot create module, bundle with symbolic name '%s' already installed.", symName);
    }
    if (status == CELIX_SUCCESS) {
        *moduleOut = module;
    } else {
        module_destroy(module);
    }

    return status;
}

celix_status_t bundle_start(celix_bundle_t* bundle) {
    //note deprecated call use celix_bundleContext_startBundle instead
    return celix_framework_startBundle(bundle->framework, celix_bundle_getId(bundle));
}

celix_status_t bundle_update(bundle_pt bundle, const char* updatedBundleUrl) {
    return celix_framework_updateBundle(bundle->framework, celix_bundle_getId(bundle), updatedBundleUrl);
}

celix_status_t bundle_stop(bundle_pt bundle) {
    //note deprecated call use celix_bundleContext_stopBundle instead
    return celix_framework_stopBundle(bundle->framework, celix_bundle_getId(bundle));
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
			status = bundleArchive_setPersistentState(bundle->archive, CELIX_BUNDLE_STATE_INSTALLED);
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
			status = bundleArchive_setPersistentState(bundle->archive, CELIX_BUNDLE_STATE_UNINSTALLED);
		}
	}

	framework_logIfError(bundle->framework->logger, status, NULL, "Failed to set persistent state to uninstalled");

    return status;
}

celix_status_t bundle_revise(bundle_pt bundle, const char * location, const char *inputFile) {
    fw_log(bundle->framework->logger, CELIX_LOG_LEVEL_DEBUG, "Usage of bundle_revise is deprecated and no longer needed. Called for bundle %s", bundle->symbolicName);
    return CELIX_SUCCESS;
}

celix_status_t bundle_addModule(bundle_pt bundle, module_pt module) {
	celix_arrayList_add(bundle->modules, module);

    //free previous module info
    free(bundle->symbolicName);
    free(bundle->name);
    free(bundle->group);
    free(bundle->description);


    //set new module info
    const char *sn = NULL;
    const char *n = NULL;
    const char *g = NULL;
    const char *d = NULL;
    module_getSymbolicName(module, &sn);
    module_getGroup(module, &g);
    module_getName(module, &n);
    module_getDescription(module, &d);
    bundle->symbolicName = celix_utils_strdup(sn);
    bundle->name = celix_utils_strdup(n);
    bundle->group = celix_utils_strdup(g);
    bundle->description = celix_utils_strdup(d);

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
    fw_log(bundle->framework->logger, CELIX_LOG_LEVEL_DEBUG, "Usage of bundle_close is deprecated and no longer needed. Called for bundle %s", bundle->symbolicName);
    return CELIX_SUCCESS;
}

celix_status_t bundle_closeAndDelete(const_bundle_pt bundle) {
    fw_log(bundle->framework->logger, CELIX_LOG_LEVEL_DEBUG, "Usage of bundle_closeAndDelete is deprecated and no longer needed. Called for bundle %s", bundle->symbolicName);
    return CELIX_SUCCESS;
}

celix_status_t bundle_closeRevisions(const_bundle_pt bundle) {
    celix_status_t status = CELIX_SUCCESS;
    return status;
}

celix_status_t bundle_refresh(bundle_pt bundle) {
    module_pt module;
    celix_arrayList_clear(bundle->modules);
    celix_status_t status = bundle_createModule(bundle, &module);
    if (status == CELIX_SUCCESS) {
                status = bundle_addModule(bundle, module);
                if (status == CELIX_SUCCESS) {
                        __atomic_store_n(&bundle->state, CELIX_BUNDLE_STATE_INSTALLED, __ATOMIC_RELEASE);
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

celix_status_t bundle_getRegisteredServices(bundle_pt bundle, celix_array_list_t** list) {
	celix_status_t status;

	status = fw_getBundleRegisteredServices(bundle->framework, bundle, list);

	framework_logIfError(bundle->framework->logger, status, NULL, "Failed to get registered services");

	return status;
}

celix_status_t bundle_getServicesInUse(bundle_pt bundle, celix_array_list_t** list) {
	celix_status_t status;

	status = fw_getBundleServicesInUse(bundle->framework, bundle, list);

	framework_logIfError(bundle->framework->logger, status, NULL, "Failed to get in use services");

	return status;
}

celix_status_t bundle_getFramework(const_bundle_pt bundle, framework_pt *framework) {
    if (bundle == NULL || framework == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    *framework = bundle->framework;
    return CELIX_SUCCESS;
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

celix_bundle_state_e celix_bundle_getState(const celix_bundle_t *bnd) {
    return __atomic_load_n(&bnd->state, __ATOMIC_ACQUIRE);
}

static char* celix_bundle_getBundleOrPersistentStoreEntry(const celix_bundle_t* bnd, bool bundleEntry, const char* name) {
    bundle_archive_pt archive = NULL;
    celix_status_t status = bundle_getArchive(bnd, &archive);
    if (status != CELIX_SUCCESS) {
        fw_logCode(bnd->framework->logger, CELIX_BUNDLE_EXCEPTION, status, "Failed to get bundle archive");
        return NULL;
    }

    const char *root;
    if (bundleEntry) {
        root = celix_bundleArchive_getCurrentRevisionRoot(archive);
    } else {
        root = celix_bundleArchive_getPersistentStoreRoot(archive);
    }

    char *entry = NULL;
    if (name == NULL || strnlen(name, 1) == 0) { //NULL or ""
        entry = celix_utils_strdup(root);
    } else if ((strnlen(name, 1) > 0) && (name[0] == '/')) {
        asprintf(&entry, "%s%s", root, name);
    } else {
        asprintf(&entry, "%s/%s", root, name);
    }

    if (celix_utils_fileExists(entry)) {
        return entry;
    } else {
        free(entry);
        return NULL;
    }
}

char* celix_bundle_getEntry(const celix_bundle_t* bnd, const char *path) {
	char *entry = NULL;
	if (bnd != NULL && bnd->framework != NULL) {
        entry = celix_bundle_getBundleOrPersistentStoreEntry(bnd, true, path);
	}
	return entry;
}

char* celix_bundle_getDataFile(const celix_bundle_t* bnd, const char *path) {
    char *entry = NULL;
    if (bnd != NULL && bnd->framework != NULL) {
        entry = celix_bundle_getBundleOrPersistentStoreEntry(bnd, false, path);
    }
    return entry;
}

const char* celix_bundle_getManifestValue(const celix_bundle_t* bnd, const char* attribute) {
	const char* header = NULL;
	if (bnd != NULL) {
        bundle_archive_t* arch = NULL;
        bundle_getArchive(bnd, &arch);
        if (arch != NULL) {
            bundle_revision_t* rev = NULL;
            bundleArchive_getCurrentRevision(arch, &rev);
            if (rev != NULL) {
                manifest_pt man = NULL;
                bundleRevision_getManifest(rev, &man);
                if (man != NULL ) {
                    header = manifest_getValue(man, attribute);
                }
            }
        }
	}
	return header;
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

char* celix_bundle_getLocation(const celix_bundle_t *bnd) {
    char* result = NULL;
    if (bnd->archive != NULL) {
        result = celix_bundleArchive_getLocation(bnd->archive);
    }
    return result;
}

const celix_version_t* celix_bundle_getVersion(const celix_bundle_t *bnd) {
    celix_version_t* result = NULL;
    celix_module_t* mod = NULL;
    bundle_getCurrentModule(bnd, &mod);
    if (mod != NULL) {
        result = module_getVersion(mod);
    }
    return result;
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

bundle_archive_t* celix_bundle_getArchive(const celix_bundle_t *bundle) {
    bundle_archive_t* archive = NULL;
    bundle_getArchive(bundle, &archive);
    return archive;
}
