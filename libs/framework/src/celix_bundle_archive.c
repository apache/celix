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

#include "celix_bundle_archive.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "celix_bundle_manifest.h"
#include "celix_bundle_manifest_type.h"
#include "celix_constants.h"
#include "celix_compiler.h"
#include "celix_file_utils.h"
#include "celix_framework_utils_private.h"
#include "celix_log.h"
#include "framework_private.h"
#include "celix_utils.h"
#include "celix_properties.h"

/**
 * The bundle archive which is used to store the bundle data and can be reused when a framework is restarted.
 * The lifecycle of a bundle archive is coupled to the lifecycle of the bundle that is created from the archive.
 *
 * @note The bundle archive is thread safe.
 */
struct celix_bundle_archive {
    // initialed during creation and immutable
    celix_framework_t* fw;
    long id;
    char* archiveRoot;
    char* savedBundleStatePropertiesPath;
    char* storeRoot;
    char* resourceCacheRoot;
    char* location;
    celix_bundle_manifest_t* manifest;
    bool cacheValid; // is the cache valid (e.g. not deleted)
    bool valid; // is the archive valid (e.g. not deleted)
};

static celix_status_t celix_bundleArchive_storeBundleStateProperties(celix_bundle_archive_t* archive) {
    bool needUpdate = false;
    celix_autoptr(celix_properties_t) bundleStateProperties = NULL;
    if (celix_utils_fileExists(archive->savedBundleStatePropertiesPath)) {
        celix_status_t status =
            celix_properties_load(archive->savedBundleStatePropertiesPath, 0, &bundleStateProperties);
        if (status != CELIX_SUCCESS) {
            celix_framework_logTssErrors(archive->fw->logger, CELIX_LOG_LEVEL_ERROR);
        }
    }
    if (!bundleStateProperties) {
        bundleStateProperties = celix_properties_create();
    }

    const char* bndVersion = celix_properties_getAsString(celix_bundleManifest_getAttributes(archive->manifest),
                                                          CELIX_BUNDLE_VERSION, "");

    if (!bundleStateProperties) {
        return CELIX_ENOMEM;
    }

    // set/update bundle cache state properties
    if (celix_properties_getAsLong(bundleStateProperties, CELIX_BUNDLE_ARCHIVE_BUNDLE_ID_PROPERTY_NAME, 0) !=
        archive->id) {
        celix_properties_setLong(bundleStateProperties, CELIX_BUNDLE_ARCHIVE_BUNDLE_ID_PROPERTY_NAME, archive->id);
        needUpdate = true;
    }
    if (strcmp(celix_properties_get(bundleStateProperties, CELIX_BUNDLE_ARCHIVE_LOCATION_PROPERTY_NAME, ""),
               archive->location) != 0) {
        celix_properties_set(bundleStateProperties, CELIX_BUNDLE_ARCHIVE_LOCATION_PROPERTY_NAME, archive->location);
        needUpdate = true;
    }
    if (strcmp(celix_properties_get(bundleStateProperties, CELIX_BUNDLE_ARCHIVE_SYMBOLIC_NAME_PROPERTY_NAME, ""),
               celix_bundleManifest_getBundleSymbolicName(archive->manifest)) != 0) {
        celix_properties_set(
            bundleStateProperties,
            CELIX_BUNDLE_ARCHIVE_SYMBOLIC_NAME_PROPERTY_NAME,
            celix_bundleManifest_getBundleSymbolicName(archive->manifest));
        needUpdate = true;
    }
    if (strcmp(celix_properties_get(bundleStateProperties, CELIX_BUNDLE_ARCHIVE_VERSION_PROPERTY_NAME, ""),
               bndVersion) != 0) {
        celix_properties_set(bundleStateProperties, CELIX_BUNDLE_ARCHIVE_VERSION_PROPERTY_NAME, bndVersion);
        needUpdate = true;
    }

    // save bundle cache state properties
    if (needUpdate) {
        celix_status_t status = celix_properties_save(
            bundleStateProperties,
            archive->savedBundleStatePropertiesPath,
            CELIX_PROPERTIES_ENCODE_PRETTY);
        if (status != CELIX_SUCCESS) {
            return status;
        }
    }
    return CELIX_SUCCESS;
}

static celix_status_t celix_bundleArchive_removeResourceCache(celix_bundle_archive_t* archive) {
    celix_status_t status = CELIX_SUCCESS;
    const char* error = NULL;
    struct stat st;
    status = lstat(archive->resourceCacheRoot, &st);
    if (status == -1 && errno == ENOENT) {
        return CELIX_SUCCESS;
    } else if(status == -1 && errno != ENOENT) {
        status = CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO,errno);
        fw_logCode(archive->fw->logger, CELIX_LOG_LEVEL_ERROR, status, "Failed to stat bundle archive directory '%s'", archive->resourceCacheRoot);
        return status;
    }
    assert(status == 0);
    // celix_utils_deleteDirectory does not work for dangling symlinks, so handle this case separately
    if (S_ISLNK(st.st_mode)) {
        status = unlink(archive->resourceCacheRoot);
        if (status == -1) {
            status = CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO,errno);
            error = "Failed to remove existing bundle symlink";
        }
    } else {
        status = celix_utils_deleteDirectory(archive->resourceCacheRoot, &error);
    }
    if (status != CELIX_SUCCESS) {
        fw_logCode(archive->fw->logger, CELIX_LOG_LEVEL_ERROR, status, "Failed to remove existing bundle archive revision directory '%s': %s", archive->resourceCacheRoot, error);
    }
    return status;
}

static celix_status_t
celix_bundleArchive_extractBundle(celix_bundle_archive_t* archive, const char* bundleUrl) {
    celix_status_t status = CELIX_SUCCESS;
    bool extractBundle = true;

    //get revision mod time;
    struct timespec revisionMod;
    status = celix_utils_getLastModified(archive->resourceCacheRoot, &revisionMod);
    if (status != CELIX_SUCCESS && errno != ENOENT) {
        fw_logCode(archive->fw->logger, CELIX_LOG_LEVEL_ERROR, status, "Failed to get last modified time for bundle archive revision directory '%s'", archive->resourceCacheRoot);
        return status;
    }

    //check if bundle location is newer than current revision
    if (status == CELIX_SUCCESS) {
        extractBundle = celix_framework_utils_isBundleUrlNewerThan(archive->fw, bundleUrl, &revisionMod);
    }

    if (!extractBundle) {
        fw_log(archive->fw->logger, CELIX_LOG_LEVEL_TRACE, "Bundle archive %s is up to date, no need to extract bundle.", bundleUrl);
        return status;
    }

    /*
     * Note always remove the current revision dir. This is needed to remove files that are not present
     * in the new bundle zip, but it seems this is also needed to ensure that the lib files get a new inode.
     * If dlopen/dlsym is used with newer files, but with the same inode already used in dlopen/dlsym this leads to
     * segfaults.
     */
    status = celix_bundleArchive_removeResourceCache(archive);
    if (status != CELIX_SUCCESS) {
        return status;
    }
    status = celix_framework_utils_extractBundle(archive->fw, bundleUrl, archive->resourceCacheRoot);
    if (status != CELIX_SUCCESS) {
        fw_log(archive->fw->logger, CELIX_LOG_LEVEL_ERROR, "Failed to initialize archive. Failed to extract bundle zip to revision directory.");
        return status;
    }
    return status;
}

/**
 * Initialize archive by creating the bundle cache directory, optionally extracting the bundle from the bundle file,
 * reading the bundle state properties, reading the bundle manifest and updating the bundle state properties.
 */
static celix_status_t celix_bundleArchive_createCacheDirectory(celix_bundle_archive_t* archive, celix_bundle_manifest_t** manifestOut) {
    *manifestOut = NULL;
    if (celix_utils_fileExists(archive->archiveRoot)) {
        fw_log(archive->fw->logger, CELIX_LOG_LEVEL_TRACE, "Bundle archive root for bundle id %li already exists.",
               archive->id);
    }

    //create archive root
    const char* errorStr = NULL;
    celix_status_t status = celix_utils_createDirectory(archive->archiveRoot, false, &errorStr);
    if (status != CELIX_SUCCESS) {
        fw_log(archive->fw->logger, CELIX_LOG_LEVEL_ERROR, "Failed to initialize archive. Failed to create bundle root archive dir: %s", errorStr);
        return status;
    }

    //create store directory
    status = celix_utils_createDirectory(archive->storeRoot, false, &errorStr);
    if (status != CELIX_SUCCESS) {
        fw_log(archive->fw->logger, CELIX_LOG_LEVEL_ERROR, "Failed to initialize archive. Failed to create bundle store dir: %s", errorStr);
        return status;
    }

    //extract bundle zip to revision directory
    status = celix_bundleArchive_extractBundle(archive, archive->location);
    if (status != CELIX_SUCCESS) {
        fw_log(archive->fw->logger, CELIX_LOG_LEVEL_ERROR, "Failed to initialize archive. Failed to extract bundle.");
        return status;
    }

    //read manifest from extracted bundle zip
    celix_autoptr(celix_bundle_manifest_t) manifest = NULL;
    char pathBuffer[512];
    char* manifestPath = celix_utils_writeOrCreateString(pathBuffer, sizeof(pathBuffer), "%s/%s", archive->resourceCacheRoot, CELIX_BUNDLE_MANIFEST_REL_PATH);
    if (manifestPath == NULL) {
        fw_log(archive->fw->logger, CELIX_LOG_LEVEL_ERROR, "Failed to initialize archive. Failed to create manifest path.");
        return CELIX_ENOMEM;
    }
    status = celix_bundleManifest_createFromFile(manifestPath, &manifest);
    celix_utils_freeStringIfNotEqual(pathBuffer, manifestPath);
    if (status != CELIX_SUCCESS) {
        celix_framework_logTssErrors(archive->fw->logger, CELIX_LOG_LEVEL_ERROR);
        fw_log(archive->fw->logger, CELIX_LOG_LEVEL_ERROR, "Failed to initialize archive. Cannot read manifest.");
        return status;
    }

    *manifestOut = celix_steal_ptr(manifest);
    return status;
}

celix_status_t celix_bundleArchive_create(celix_framework_t* fw, const char *archiveRoot, long id, const char *location, celix_bundle_archive_t** bundle_archive) {
    celix_status_t status = CELIX_SUCCESS;
    celix_autoptr(celix_bundle_archive_t) archive = calloc(1, sizeof(*archive));
    bool isFrameworkBundle = id == CELIX_FRAMEWORK_BUNDLE_ID;

    if (!archive) {
        fw_log(fw->logger, CELIX_LOG_LEVEL_ERROR, "Could not allocate memory for the bundle archive.");
        return ENOMEM;
    }

    archive->fw = fw;
    archive->id = id;

    if (isFrameworkBundle) {
        archive->resourceCacheRoot = getcwd(NULL, 0);
        archive->storeRoot = getcwd(NULL, 0);
        if (archive->resourceCacheRoot == NULL || archive->storeRoot == NULL) {
            status = CELIX_ENOMEM;
        }
    } else {
        status = CELIX_ENOMEM;
        do {
            archive->location = celix_utils_strdup(location);
            if (archive->location == NULL) {
                break;
            }
            archive->archiveRoot = celix_utils_strdup(archiveRoot);
            if (archive->archiveRoot == NULL) {
                break;
            }
            if (asprintf(&archive->savedBundleStatePropertiesPath, "%s/%s", archiveRoot,
                         CELIX_BUNDLE_ARCHIVE_STATE_PROPERTIES_FILE_NAME) < 0) {
                break;
            }
            if (asprintf(&archive->storeRoot, "%s/%s", archive->archiveRoot, CELIX_BUNDLE_ARCHIVE_STORE_DIRECTORY_NAME) < 0) {
                break;
            }
            if (asprintf(&archive->resourceCacheRoot, "%s/%s", archive->archiveRoot, CELIX_BUNDLE_ARCHIVE_RESOURCE_CACHE_NAME) < 0) {
                break;
            }
            status = CELIX_SUCCESS;
        } while (0);
    }
    if (status != CELIX_SUCCESS) {
        fw_log(fw->logger, CELIX_LOG_LEVEL_ERROR, "Failed to create bundle archive; Out of memory.");
        return status;
    }

    if (isFrameworkBundle) {
        status = celix_bundleManifest_createFrameworkManifest(&archive->manifest);
    } else {
        status = celix_bundleArchive_createCacheDirectory(archive, &archive->manifest);
    }
    if (status != CELIX_SUCCESS) {
        fw_logCode(fw->logger, CELIX_LOG_LEVEL_ERROR, status, "Failed to create bundle archive.");
        if (!isFrameworkBundle) {
            celix_utils_deleteDirectory(archive->archiveRoot, NULL);
        }
        return status;
    }

    if (!isFrameworkBundle) {
        status = celix_bundleArchive_storeBundleStateProperties(archive);
        if (status != CELIX_SUCCESS) {
            fw_logCode(fw->logger, CELIX_LOG_LEVEL_ERROR, status, "Failed to store bundle state properties.");
            celix_utils_deleteDirectory(archive->archiveRoot, NULL);
            return status;
        }
    }
    archive->cacheValid = true;
    archive->valid = true;
    *bundle_archive = celix_steal_ptr(archive);
    return CELIX_SUCCESS;
}

void celix_bundleArchive_destroy(celix_bundle_archive_t* archive) {
    if (archive != NULL) {
        free(archive->location);
        free(archive->savedBundleStatePropertiesPath);
        free(archive->archiveRoot);
        free(archive->resourceCacheRoot);
        free(archive->storeRoot);
        celix_bundleManifest_destroy(archive->manifest);
        free(archive);
    }
}

long celix_bundleArchive_getId(const celix_bundle_archive_t* archive) {
    return archive->id;
}

celix_bundle_manifest_t* celix_bundleArchive_getManifest(const celix_bundle_archive_t* archive) {
    return archive->manifest;
}

const char* celix_bundleArchive_getSymbolicName(const celix_bundle_archive_t* archive) {
    return  celix_bundleManifest_getBundleSymbolicName(celix_bundleArchive_getManifest(archive));
}

const char* celix_bundleArchive_getLocation(const celix_bundle_archive_t* archive) {
    return archive->location;
}

const char* celix_bundleArchive_getArchiveRoot(const celix_bundle_archive_t* archive) {
    return archive->archiveRoot;
}

celix_status_t celix_bundleArchive_getLastModified(const celix_bundle_archive_t* archive, struct timespec* lastModified) {
    return celix_utils_getLastModified(archive->resourceCacheRoot, lastModified);
}

const char* celix_bundleArchive_getPersistentStoreRoot(const celix_bundle_archive_t* archive) {
    return archive->storeRoot;
}

const char* celix_bundleArchive_getCurrentRevisionRoot(const celix_bundle_archive_t* archive) {
    return archive->resourceCacheRoot;
}

void celix_bundleArchive_invalidate(celix_bundle_archive_t* archive) {
    archive->valid = false;
    archive->cacheValid = false;
}

void celix_bundleArchive_invalidateCache(celix_bundle_archive_t* archive) {
    archive->cacheValid = false;
}

bool celix_bundleArchive_isCacheValid(const celix_bundle_archive_t* archive) {
    return archive->cacheValid;
}

void celix_bundleArchive_removeInvalidDirs(celix_bundle_archive_t* archive) {
    if (archive->id == CELIX_FRAMEWORK_BUNDLE_ID) {
        return;
    }
    if (!archive->valid) {
        celix_status_t status = CELIX_SUCCESS;
        const char* err = NULL;
        status = celix_utils_deleteDirectory(archive->archiveRoot, &err);
        framework_logIfError(archive->fw->logger, status, NULL, "Failed to remove invalid archive root '%s': %s", archive->archiveRoot, err);
    } else if (!archive->cacheValid){
        (void)celix_bundleArchive_removeResourceCache(archive);
    }
}
