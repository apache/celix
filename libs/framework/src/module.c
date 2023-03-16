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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "celix_utils.h"
#include "utils.h"
#include "module.h"
#include "manifest_parser.h"
#include "linked_list_iterator.h"
#include "celix_libloader.h"
#include "celix_framework.h"
#include "celix_constants.h"
#include "framework_private.h"
#include "bundle_archive_private.h"


#ifdef __linux__
#define CELIX_LIBRARY_PREFIX "lib"
#define CELIX_LIBRARY_EXTENSION ".so"
#elif __APPLE__
#define CELIX_LIBRARY_PREFIX "lib"
#define CELIX_LIBRARY_EXTENSION ".dylib"
#elif WIN32
#define CELIX_LIBRARY_PREFIX ""
#define CELIX_LIBRARY_EXTENSION = ".dll";
#endif

struct module {
    celix_framework_t* fw;
	linked_list_pt capabilities;
	linked_list_pt requirements;
	linked_list_pt wires;

	array_list_pt dependentImporters;

	version_pt version;
	char* symbolicName;
	char* group;
    char* name;
    char* description;
	bool resolved;

	manifest_pt headerMap;
	char * id;

	celix_bundle_t *bundle;

    celix_thread_mutex_t handlesLock; //protects libraryHandles and bundleActivatorHandle
    celix_array_list_t* libraryHandles;
    void* bundleActivatorHandle;
};

module_pt module_create(manifest_pt headerMap, const char * moduleId, bundle_pt bundle) {
    module_pt module = NULL;
    manifest_parser_pt mp;
    celix_framework_t* fw = NULL;
    bundle_getFramework(bundle, &fw);

    if (headerMap != NULL && fw != NULL) {
        module = (module_pt) calloc(1, sizeof(*module));
        module->fw = fw;
        module->headerMap = headerMap;
        module->id = strdup(moduleId);
        module->bundle = bundle;
        module->resolved = false;
        module->dependentImporters = NULL;
        arrayList_create(&module->dependentImporters);
        celixThreadMutex_create(&module->handlesLock, NULL);
        module->libraryHandles = celix_arrayList_create();


        if (manifestParser_create(module, headerMap, &mp) == CELIX_SUCCESS) {
            module->symbolicName = NULL;
            manifestParser_getAndDuplicateSymbolicName(mp, &module->symbolicName);

            module->group = NULL;
            manifestParser_getAndDuplicateGroup(mp, &module->group);

            module->name = NULL;
            manifestParser_getAndDuplicateName(mp, &module->name);

            module->description = NULL;
            manifestParser_getAndDuplicateDescription(mp, &module->description);

            module->version = NULL;
            manifestParser_getBundleVersion(mp, &module->version);

            module->capabilities = NULL;
            manifestParser_getCapabilities(mp, &module->capabilities);

            module->requirements = NULL;
            manifestParser_getRequirements(mp, &module->requirements);

            module->wires = NULL;

            manifestParser_destroy(mp);
        }
    }

    return module;
}

module_pt module_createFrameworkModule(celix_framework_t* fw, bundle_pt bundle) {
    module_pt module;

	module = (module_pt) calloc(1, sizeof(*module));
    char modId[2];
    snprintf(modId, 2, "%li", CELIX_FRAMEWORK_BUNDLE_ID);
	if (module) {
        module->fw = fw;
        module->id = celix_utils_strdup(modId);
        module->symbolicName = celix_utils_strdup("celix_framework");
        module->group = celix_utils_strdup("Celix/Framework");
        module->name = celix_utils_strdup("Celix Framework");
        module->description = celix_utils_strdup("The Celix Framework System Bundle");
        module->version = NULL;
        version_createVersion(1, 0, 0, "", &module->version);
        linkedList_create(&module->capabilities);
        linkedList_create(&module->requirements);
        module->dependentImporters = NULL;
        arrayList_create(&module->dependentImporters);
        module->wires = NULL;
        module->headerMap = NULL;
        module->resolved = false;
        module->bundle = bundle;
        celixThreadMutex_create(&module->handlesLock, NULL);
        module->libraryHandles = celix_arrayList_create();
	}
	return module;
}

void module_destroy(module_pt module) {
	arrayList_destroy(module->dependentImporters);

	version_destroy(module->version);

	if (module->wires != NULL) {
        linked_list_iterator_pt iter = linkedListIterator_create(module->wires, 0);
        while (linkedListIterator_hasNext(iter)) {
            wire_pt next = linkedListIterator_next(iter);
            linkedListIterator_remove(iter);
            wire_destroy(next);
        }
        linkedListIterator_destroy(iter);
        linkedList_destroy(module->wires);
	}

	if (module->requirements != NULL) {
	    linked_list_iterator_pt iter = linkedListIterator_create(module->requirements, 0);
        while (linkedListIterator_hasNext(iter)) {
            requirement_pt next = linkedListIterator_next(iter);
            linkedListIterator_remove(iter);
            requirement_destroy(next);
        }
        linkedListIterator_destroy(iter);
        linkedList_destroy(module->requirements);
	}

	if (module->capabilities != NULL) {
	    linked_list_iterator_pt iter = linkedListIterator_create(module->capabilities, 0);
        while (linkedListIterator_hasNext(iter)) {
            capability_pt next = linkedListIterator_next(iter);
            linkedListIterator_remove(iter);
            capability_destroy(next);
        }
        linkedListIterator_destroy(iter);
        linkedList_destroy(module->capabilities);
    }

	module->headerMap = NULL;

	free(module->id);
	free(module->symbolicName);
    free(module->name);
    free(module->group);
    free(module->description);
    celix_arrayList_destroy(module->libraryHandles);
    celixThreadMutex_destroy(&module->handlesLock);
	free(module);
}

wire_pt module_getWire(module_pt module, const char * serviceName) {
	wire_pt wire = NULL;
	if (module->wires != NULL) {
		linked_list_iterator_pt iterator = linkedListIterator_create(module->wires, 0);
		while (linkedListIterator_hasNext(iterator)) {
			const char* name;
			wire_pt next = linkedListIterator_next(iterator);
			capability_pt cap = NULL;
			wire_getCapability(next, &cap);
			capability_getServiceName(cap, &name);
			if (strcasecmp(name, serviceName) == 0) {
				wire = next;
			}
		}
		linkedListIterator_destroy(iterator);
	}
	return wire;
}

version_pt module_getVersion(module_pt module) {
	return module->version;
}

celix_status_t module_getSymbolicName(module_pt module, const char **symbolicName) {
	celix_status_t status = CELIX_SUCCESS;
	if (module == NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		*symbolicName = module->symbolicName;
	}
	return status;
}

celix_status_t module_getName(module_pt module, const char **symbolicName) {
    celix_status_t status = CELIX_SUCCESS;
    if (module == NULL) {
        status = CELIX_ILLEGAL_ARGUMENT;
    } else {
        *symbolicName = module->name;
    }
    return status;
}

celix_status_t module_getGroup(module_pt module, const char **symbolicName) {
    celix_status_t status = CELIX_SUCCESS;
    if (module == NULL) {
        status = CELIX_ILLEGAL_ARGUMENT;
    } else {
        *symbolicName = module->group;
    }
    return status;
}

celix_status_t module_getDescription(module_pt module, const char **symbolicName) {
    celix_status_t status = CELIX_SUCCESS;
    if (module == NULL) {
        status = CELIX_ILLEGAL_ARGUMENT;
    } else {
        *symbolicName = module->description;
    }
    return status;
}

char * module_getId(module_pt module) {
	return module->id;
}

linked_list_pt module_getWires(module_pt module) {
	return module->wires;
}

void module_setWires(module_pt module, linked_list_pt wires) {
    int i = 0;
    for (i = 0; (module->wires != NULL) && (i < linkedList_size(module->wires)); i++) {
        wire_pt wire = (wire_pt) linkedList_get(module->wires, i);
        module_pt exporter = NULL;
        wire_getExporter(wire, &exporter);
        module_removeDependentImporter(exporter, module);
    }

    if (module->wires != NULL) {
		linked_list_iterator_pt iter = linkedListIterator_create(module->wires, 0);
		while (linkedListIterator_hasNext(iter)) {
			wire_pt next = linkedListIterator_next(iter);
			linkedListIterator_remove(iter);
			wire_destroy(next);
		}
		linkedListIterator_destroy(iter);
		linkedList_destroy(module->wires);
	}

	module->wires = wires;

	for (i = 0; (module->wires != NULL) && (i < linkedList_size(module->wires)); i++) {
        wire_pt wire = (wire_pt) linkedList_get(module->wires, i);
        module_pt exporter = NULL;
        wire_getExporter(wire, &exporter);
        module_addDependentImporter(exporter, module);
    }
}

bool module_isResolved(module_pt module) {
	return module->resolved;
}

void module_setResolved(module_pt module) {
	module->resolved = true;
}

bundle_pt module_getBundle(module_pt module) {
	return module->bundle;
}

linked_list_pt module_getRequirements(module_pt module) {
	return module->requirements;
}

linked_list_pt module_getCapabilities(module_pt module) {
	return module->capabilities;
}

array_list_pt module_getDependentImporters(module_pt module) {
    return module->dependentImporters;
}

void module_addDependentImporter(module_pt module, module_pt importer) {
    if (!arrayList_contains(module->dependentImporters, importer)) {
        arrayList_add(module->dependentImporters, importer);
    }
}

void module_removeDependentImporter(module_pt module, module_pt importer) {
    arrayList_removeElement(module->dependentImporters, importer);
}

//----------------------------------------------------
//TODO add implementation (functions not implemented but already exported)
array_list_pt module_getDependentRequirers(module_pt module) {
	return NULL;
}

void module_addDependentRequirer(module_pt module, module_pt requirer) {
}

void module_removeDependentRequirer(module_pt module, module_pt requirer) {
}
//----------------------------------------------------

array_list_pt module_getDependents(module_pt module) {
    array_list_pt dependents = NULL;
    arrayList_create(&dependents);

    arrayList_addAll(dependents, module->dependentImporters);

    return dependents;
}

celix_status_t celix_module_closeLibraries(celix_module_t* module) {
    celix_status_t status = CELIX_SUCCESS;
    celix_bundle_context_t *fwCtx = celix_framework_getFrameworkContext(module->fw);
    celixThreadMutex_lock(&module->handlesLock);
    for (int i = 0; i < celix_arrayList_size(module->libraryHandles); i++) {
        void *handle = celix_arrayList_get(module->libraryHandles, i);
        celix_libloader_close(fwCtx, handle);
    }
    celix_arrayList_clear(module->libraryHandles);
    module->bundleActivatorHandle = NULL;
    celixThreadMutex_unlock(&module->handlesLock);
    return status;
}

static celix_status_t celix_module_loadLibrary(celix_module_t* module, const char* path, void** handle) {
    celix_status_t status = CELIX_SUCCESS;
    celix_bundle_context_t *fwCtx = celix_framework_getFrameworkContext(module->fw);
    void *libHandle = celix_libloader_open(fwCtx, path);
    if (libHandle == NULL) {
        status = CELIX_BUNDLE_EXCEPTION;
        const char* error = celix_libloader_getLastError();
        fw_logCode(module->fw->logger, CELIX_LOG_LEVEL_ERROR, status, "Cannot open library %s: %s", path, error);
    } else {
        celixThreadMutex_lock(&module->handlesLock);
        celix_arrayList_add(module->libraryHandles, libHandle);
        celixThreadMutex_unlock(&module->handlesLock);
        *handle = libHandle;
    }
    return status;
}


static celix_status_t celix_module_loadLibraryForManifestEntry(celix_module_t* module, const char *library, bundle_archive_pt archive, void **handle) {
    celix_status_t status = CELIX_SUCCESS;

    const char *error = NULL;
    char libraryPath[512];
    const char* revRoot = celix_bundleArchive_getCurrentRevisionRoot(archive);

    if (!revRoot) {
        fw_logCode(module->fw->logger, CELIX_LOG_LEVEL_ERROR, status, "Could not get bundle archive information");
        return status;
    }

    char* path;
    if (strstr(library, CELIX_LIBRARY_EXTENSION)) {
        path = celix_utils_writeOrCreateString(libraryPath, sizeof(libraryPath), "%s/%s", revRoot, library);
    } else {
        path = celix_utils_writeOrCreateString(libraryPath, sizeof(libraryPath), "%s/%s%s%s", revRoot, CELIX_LIBRARY_PREFIX, library, CELIX_LIBRARY_EXTENSION);
    }

    if (!path) {
        fw_logCode(module->fw->logger, CELIX_LOG_LEVEL_ERROR, status, "Cannot create full library path");
        return status;
    }

    status = celix_module_loadLibrary(module, path, handle);
    celix_utils_freeStringIfNotEqual(libraryPath, path);
    framework_logIfError(module->fw->logger, status, error, "Could not load library: %s", libraryPath);
    return status;
}

static celix_status_t celix_module_loadLibrariesInManifestEntry(celix_module_t* module, const char *librariesIn, const char *activator, bundle_archive_pt archive, void **activatorHandle) {
    celix_status_t status = CELIX_SUCCESS;
    celix_bundle_context_t* fwCtx = celix_framework_getFrameworkContext(module->fw);

    char* last;
    char* libraries = strndup(librariesIn, 1024*10);
    char* token = strtok_r(libraries, ",", &last);
    while (token != NULL) {
        void *handle = NULL;
        char lib[128];
        lib[127] = '\0';

        char *path = NULL;
        char *pathToken = strtok_r(token, ";", &path);
        strncpy(lib, pathToken, 127);
        pathToken = strtok_r(NULL, ";", &path);

        while (pathToken != NULL) {

            /*Disable version should be part of the lib name
            if (strncmp(pathToken, "version", 7) == 0) {
                char *ver = strdup(pathToken);
                char version[strlen(ver) - 9];
                strncpy(version, ver+9, strlen(ver) - 10);
                version[strlen(ver) - 10] = '\0';

                strcat(lib, "-");
                strcat(lib, version);
            }*/
            pathToken = strtok_r(NULL, ";", &path);
        }

        char *trimmedLib = utils_stringTrim(lib);
        status = celix_module_loadLibraryForManifestEntry(module, trimmedLib, archive, &handle);

        if ( (status == CELIX_SUCCESS) && (activator != NULL) && (strcmp(trimmedLib, activator) == 0) ) {
            *activatorHandle = handle;
        } else if ((status != CELIX_SUCCESS) && (handle != NULL)) {
            celix_libloader_close(fwCtx, handle);
        }

        token = strtok_r(NULL, ",", &last);
    }

    free(libraries);
    return status;
}

celix_status_t celix_module_loadLibraries(celix_module_t* module) {
    celix_status_t status = CELIX_SUCCESS;
    celix_bundle_context_t* fwCtx = celix_framework_getFrameworkContext(module->fw);

    celix_library_handle_t* activatorHandle = NULL;
    bundle_archive_pt archive = NULL;
    bundle_revision_pt revision = NULL;
    manifest_pt manifest = NULL;

    status = CELIX_DO_IF(status, bundle_getArchive(module->bundle, &archive));
    status = CELIX_DO_IF(status, bundleArchive_getCurrentRevision(archive, &revision));
    status = CELIX_DO_IF(status, bundleRevision_getManifest(revision, &manifest));
    if (status == CELIX_SUCCESS) {
        const char *privateLibraries = NULL;
        const char *exportLibraries = NULL;
        const char *activator = NULL;

        privateLibraries = manifest_getValue(manifest, CELIX_FRAMEWORK_PRIVATE_LIBRARY);
        exportLibraries = manifest_getValue(manifest, CELIX_FRAMEWORK_EXPORT_LIBRARY);
        //@note not import yet
        activator = manifest_getValue(manifest, CELIX_FRAMEWORK_BUNDLE_ACTIVATOR);

        if (exportLibraries != NULL) {
            status = CELIX_DO_IF(status, celix_module_loadLibrariesInManifestEntry(module, exportLibraries, activator, archive, &activatorHandle));
        }

        if (privateLibraries != NULL) {
            status = CELIX_DO_IF(status,
                                 celix_module_loadLibrariesInManifestEntry(module, privateLibraries, activator, archive, &activatorHandle));
        }

        if (status == CELIX_SUCCESS) {
            bundle_setHandle(module->bundle, activatorHandle); //note deprecated
            celixThreadMutex_lock(&module->handlesLock);
            module->bundleActivatorHandle = activatorHandle;
            celixThreadMutex_unlock(&module->handlesLock);
        } else if (activatorHandle != NULL) {
            celix_libloader_close(fwCtx, activatorHandle);
        }
    }


    if (status != CELIX_SUCCESS) {
        fw_logCode(module->fw->logger, CELIX_LOG_LEVEL_ERROR, status, "Could not load libraries for bundle %s",
                   celix_bundle_getSymbolicName(module->bundle));
    }

    return status;
}

void* celix_module_getBundleActivatorHandler(celix_module_t* module) {
    celixThreadMutex_lock(&module->handlesLock);
    void* handle = module->bundleActivatorHandle;
    celixThreadMutex_unlock(&module->handlesLock);
    return handle;
}
