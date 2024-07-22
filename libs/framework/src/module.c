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

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bundle_archive_private.h"
#include "celix_bundle_manifest.h"
#include "celix_constants.h"
#include "celix_framework.h"
#include "celix_libloader.h"
#include "celix_module.h"
#include "celix_utils.h"
#include "framework_private.h"
#include "utils.h"
#include "celix_bundle_private.h"

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

struct celix_module {
    celix_framework_t* fw;
    bool resolved;
    celix_bundle_t* bundle;
    celix_thread_mutex_t handlesLock; // protects libraryHandles
    celix_array_list_t* libraryHandles;
};

static celix_bundle_manifest_t* celix_module_getManifestFromBundle(celix_bundle_t* bundle) {
    bundle_archive_t* archive = celix_bundle_getArchive(bundle);
    celix_bundle_revision_t* revision = NULL;
    (void)bundleArchive_getCurrentRevision(archive, &revision);
    return celix_bundleRevision_getManifest(revision);
}

celix_module_t* module_create(celix_bundle_t* bundle) {
    assert(bundle != NULL);

    celix_framework_t* fw;
    bundle_getFramework(bundle, &fw);

    celix_autoptr(celix_module_t) module = calloc(1, sizeof(*module));
    celix_autoptr(celix_array_list_t) libraryHandles = celix_arrayList_createPointerArray();
    if (!module || !libraryHandles) {
        fw_log(fw->logger, CELIX_LOG_LEVEL_ERROR, "Failed to create module, out of memory");
        return NULL;
    }

    celix_status_t status = celixThreadMutex_create(&module->handlesLock, NULL);
    if (status != CELIX_SUCCESS) {
        fw_log(fw->logger,
               CELIX_LOG_LEVEL_ERROR,
               "Failed to create module, error creating mutex: %s",
               celix_strerror(errno));
        celix_framework_logTssErrors(fw->logger, CELIX_LOG_LEVEL_ERROR);
        return NULL;
    }

    module->fw = fw;
    module->bundle = bundle;
    module->resolved = false;
    module->libraryHandles = celix_steal_ptr(libraryHandles);

    return celix_steal_ptr(module);
}

celix_module_t* module_createFrameworkModule(celix_framework_t* fw, bundle_pt bundle) {
    celix_autoptr(celix_module_t) module = calloc(1, sizeof(*module));
    celix_autoptr(celix_array_list_t) libraryHandles = celix_arrayList_createPointerArray();
    if (!module || !libraryHandles) {
        fw_log(fw->logger, CELIX_LOG_LEVEL_ERROR, "Failed to create module, out of memory");
        return NULL;
    }

    celix_status_t status = celixThreadMutex_create(&module->handlesLock, NULL);
    if (status != CELIX_SUCCESS) {
        fw_log(fw->logger,
               CELIX_LOG_LEVEL_ERROR,
               "Failed to create module, error creating mutex: %s",
               celix_strerror(errno));
        celix_framework_logTssErrors(fw->logger, CELIX_LOG_LEVEL_ERROR);
        return NULL;
    }

    module->fw = fw;
    module->bundle = bundle;
    module->resolved = false;
    module->libraryHandles = celix_steal_ptr(libraryHandles);

    return celix_steal_ptr(module);
}

void module_destroy(celix_module_t* module) {
    if (module) {
        celix_arrayList_destroy(module->libraryHandles);
        celixThreadMutex_destroy(&module->handlesLock);
        free(module);
    }
}

const celix_version_t* module_getVersion(celix_module_t* module) {
    celix_bundle_manifest_t* man = celix_module_getManifestFromBundle(module->bundle);
    return celix_bundleManifest_getBundleVersion(man);
}

celix_status_t module_getSymbolicName(celix_module_t* module, const char** symbolicName) {
    celix_status_t status = CELIX_SUCCESS;
    if (module == NULL) {
        status = CELIX_ILLEGAL_ARGUMENT;
    } else {
        celix_bundle_manifest_t* man = celix_module_getManifestFromBundle(module->bundle);
        *symbolicName = celix_bundleManifest_getBundleSymbolicName(man);
    }
    return status;
}

celix_status_t module_getName(celix_module_t* module, const char **name) {
    celix_status_t status = CELIX_SUCCESS;
    if (module == NULL) {
        status = CELIX_ILLEGAL_ARGUMENT;
    } else {
        celix_bundle_manifest_t* man = celix_module_getManifestFromBundle(module->bundle);
        *name = celix_bundleManifest_getBundleName(man);
    }
    return status;
}

celix_status_t module_getGroup(celix_module_t* module, const char **group) {
    celix_status_t status = CELIX_SUCCESS;
    if (module == NULL) {
        status = CELIX_ILLEGAL_ARGUMENT;
    } else {
        celix_bundle_manifest_t* man = celix_module_getManifestFromBundle(module->bundle);
        *group = celix_bundleManifest_getBundleGroup(man);
    }
    return status;
}

celix_status_t module_getDescription(celix_module_t* module, const char **description) {
    celix_status_t status = CELIX_SUCCESS;
    if (module == NULL) {
        status = CELIX_ILLEGAL_ARGUMENT;
    } else {
        celix_bundle_manifest_t* man = celix_module_getManifestFromBundle(module->bundle);
        *description = celix_bundleManifest_getBundleDescription(man);
    }
    return status;
}

bool module_isResolved(celix_module_t* module) {
	return module->resolved;
}

void module_setResolved(celix_module_t* module) {
	module->resolved = true;
}

bundle_pt module_getBundle(celix_module_t* module) {
	return module->bundle;
}

celix_status_t celix_module_closeLibraries(celix_module_t* module) {
    celix_status_t status = CELIX_SUCCESS;
    celix_bundle_context_t *fwCtx = celix_framework_getFrameworkContext(module->fw);
    bundle_setHandle(module->bundle, NULL); //note deprecated
    celixThreadMutex_lock(&module->handlesLock);
    for (int i = 0; i < celix_arrayList_size(module->libraryHandles); i++) {
        void *handle = celix_arrayList_get(module->libraryHandles, i);
        celix_libloader_close(fwCtx, handle);
    }
    celix_arrayList_clear(module->libraryHandles);
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

static celix_status_t celix_module_loadLibraryForManifestEntry(celix_module_t* module,
                                                               const char* library,
                                                               bundle_archive_pt archive,
                                                               void** handle) {
    celix_status_t status = CELIX_SUCCESS;

    const char* error = NULL;
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
        path = celix_utils_writeOrCreateString(libraryPath,
                                               sizeof(libraryPath),
                                               "%s/%s%s%s",
                                               revRoot,
                                               CELIX_LIBRARY_PREFIX,
                                               library,
                                               CELIX_LIBRARY_EXTENSION);
    }

    if (!path) {
        fw_logCode(module->fw->logger, CELIX_LOG_LEVEL_ERROR, status, "Cannot create full library path");
        return errno;
    }

    status = celix_module_loadLibrary(module, path, handle);
    celix_utils_freeStringIfNotEqual(libraryPath, path);
    framework_logIfError(module->fw->logger, status, error, "Could not load library: %s", libraryPath);
    return status;
}

static celix_status_t celix_module_loadLibrariesInManifestEntry(celix_module_t* module,
                                                                const celix_array_list_t* libraries,
                                                                const char* activator,
                                                                bundle_archive_pt archive,
                                                                void** activatorHandle) {
    assert(libraries != NULL);
    assert(celix_arrayList_getElementType(libraries) == CELIX_ARRAY_LIST_ELEMENT_TYPE_STRING);
    celix_status_t status = CELIX_SUCCESS;

    for (int i = 0; i < celix_arrayList_size(libraries); i++) {
        const char* library = celix_arrayList_getString(libraries, i);
        void* handle = NULL;
        status = celix_module_loadLibraryForManifestEntry(module, library, archive, &handle);
        if (status != CELIX_SUCCESS) {
            break;
        }
        if (activator != NULL && strcmp(library, activator) == 0) {
            *activatorHandle = handle;
        }
    }
    return status;
}

celix_status_t celix_module_loadLibraries(celix_module_t* module) {
    celix_status_t status = CELIX_SUCCESS;
    celix_library_handle_t* activatorHandle = NULL;
    bundle_archive_t* archive = celix_bundle_getArchive(module->bundle);

    celix_bundle_manifest_t* man = celix_module_getManifestFromBundle(module->bundle);
    const char* activator = celix_bundleManifest_getBundleActivatorLibrary(man);
    const celix_array_list_t* privateLibraries = celix_bundleManifest_getBundlePrivateLibraries(man);

    if (privateLibraries != NULL) {
        status = CELIX_DO_IF(
            status,
            celix_module_loadLibrariesInManifestEntry(module, privateLibraries, activator, archive, &activatorHandle));
    }

    if (status != CELIX_SUCCESS) {
        fw_logCode(module->fw->logger,
                   CELIX_LOG_LEVEL_ERROR,
                   status,
                   "Could not load libraries for bundle %s",
                   celix_bundle_getSymbolicName(module->bundle));
        (void)celix_module_closeLibraries(module);
    }

    bundle_setHandle(module->bundle, activatorHandle);
    return status;
}
