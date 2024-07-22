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
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "celix_constants.h"
#include "celix_compiler.h"
#include "celix_file_utils.h"
#include "celix_framework_utils_private.h"
#include "celix_utils_api.h"
#include "celix_log.h"

#include "bundle_archive_private.h"
#include "bundle_revision_private.h"
#include "framework_private.h"

/**
 * The bundle archive which is used to store the bundle data and can be reused when a framework is restarted.
 * The lifecycle of a bundle archive is coupled to the lifecycle of the bundle that is created from the archive.
 *
 * @note The bundle archive is thread safe.
 */
struct bundleArchive {
    // initialed during creation and immutable
    celix_framework_t* fw;
    long id;
    char* archiveRoot;
    char* savedBundleStatePropertiesPath;
    char* storeRoot;
    char* resourceCacheRoot;
    char* bundleSymbolicName; // read from the manifest
    char* bundleVersion;      // read from the manifest
    celix_bundle_revision_t* revision; // the current revision
    char* location;
    bool cacheValid; // is the cache valid (e.g. not deleted)
    bool valid; // is the archive valid (e.g. not deleted)
};

static celix_status_t celix_bundleArchive_storeBundleStateProperties(bundle_archive_pt archive) {
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
               archive->bundleSymbolicName) != 0) {
        celix_properties_set(
            bundleStateProperties, CELIX_BUNDLE_ARCHIVE_SYMBOLIC_NAME_PROPERTY_NAME, archive->bundleSymbolicName);
        needUpdate = true;
    }
    if (strcmp(celix_properties_get(bundleStateProperties, CELIX_BUNDLE_ARCHIVE_VERSION_PROPERTY_NAME, ""),
               archive->bundleVersion) != 0) {
        celix_properties_set(bundleStateProperties, CELIX_BUNDLE_ARCHIVE_VERSION_PROPERTY_NAME, archive->bundleVersion);
        needUpdate = true;
    }

    // save bundle cache state properties
    if (needUpdate) {
        celix_status_t status = celix_properties_save(
            bundleStateProperties, archive->savedBundleStatePropertiesPath, CELIX_PROPERTIES_ENCODE_PRETTY);
        if (status != CELIX_SUCCESS) {
            return status;
        }
    }
    return CELIX_SUCCESS;
}

static celix_status_t celix_bundleArchive_removeResourceCache(bundle_archive_t* archive) {
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
celix_bundleArchive_extractBundle(bundle_archive_t* archive, const char* bundleUrl) {
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
static celix_status_t celix_bundleArchive_createCacheDirectory(bundle_archive_pt archive, celix_bundle_manifest_t** manifestOut) {
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

    //populate bundle symbolic name and version from manifest
    archive->bundleSymbolicName = celix_utils_strdup(celix_bundleManifest_getBundleSymbolicName(manifest));
    if (archive->bundleSymbolicName == NULL) {
        fw_log(archive->fw->logger, CELIX_LOG_LEVEL_ERROR, "Failed to initialize archive. Cannot read bundle symbolic name.");
        return CELIX_BUNDLE_EXCEPTION;
    }
    archive->bundleVersion = celix_version_toString(celix_bundleManifest_getBundleVersion(manifest));
    if (archive->bundleVersion == NULL) {
        fw_log(archive->fw->logger, CELIX_LOG_LEVEL_ERROR, "Failed to initialize archive. Cannot read bundle version.");
        return CELIX_BUNDLE_EXCEPTION;
    }

    *manifestOut = celix_steal_ptr(manifest);
    return status;
}

celix_status_t celix_bundleArchive_create(celix_framework_t* fw, const char *archiveRoot, long id, const char *location, bundle_archive_pt *bundle_archive) {
    celix_status_t status = CELIX_SUCCESS;
    const char* error = NULL;
    bundle_archive_pt archive = calloc(1, sizeof(*archive));
    bool isFrameworkBundle = id == CELIX_FRAMEWORK_BUNDLE_ID;

    if (archive == NULL) {
        status = CELIX_ENOMEM;
        goto calloc_failed;
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
        error = "Failed to setup archive paths.";
        goto init_failed;
    }

    celix_bundle_manifest_t* manifest = NULL;
    if (isFrameworkBundle) {
        status = celix_bundleManifest_createFrameworkManifest(&manifest);
    } else {
        status = celix_bundleArchive_createCacheDirectory(archive, &manifest);
    }
    if (status != CELIX_SUCCESS) {
        error = "Failed to initialize archive or create manifest.";
        goto dir_failed;
    }

    status = celix_bundleRevision_create(fw, archive->archiveRoot, archive->location, manifest, &archive->revision);
    if (status != CELIX_SUCCESS) {
        error = "Could not create bundle revision.";
        goto revision_failed;
    }

    if (!isFrameworkBundle) {
        status = celix_bundleArchive_storeBundleStateProperties(archive);
        if (status != CELIX_SUCCESS) {
            error = "Could not store properties.";
            goto store_prop_failed;
        }
    }
    archive->cacheValid = true;
    archive->valid = true;
    *bundle_archive = archive;
    return CELIX_SUCCESS;
store_prop_failed:
revision_failed:
dir_failed:
    if (!isFrameworkBundle) {
        celix_utils_deleteDirectory(archive->archiveRoot, NULL);
    }
init_failed:
    celix_bundleArchive_destroy(archive);
calloc_failed:
    celix_framework_logTssErrors(fw->logger, CELIX_LOG_LEVEL_ERROR);
    framework_logIfError(fw->logger, status, error, "Could not create archive.");
    return status;
}

void celix_bundleArchive_destroy(bundle_archive_pt archive) {
    if (archive != NULL) {
        free(archive->location);
        free(archive->savedBundleStatePropertiesPath);
        free(archive->archiveRoot);
        free(archive->resourceCacheRoot);
        free(archive->storeRoot);
        free(archive->bundleSymbolicName);
        free(archive->bundleVersion);
        celix_bundleRevision_destroy(archive->revision);
        free(archive);
    }
}

celix_status_t bundleArchive_getId(bundle_archive_pt archive, long* id) {
    *id = archive->id;
    return CELIX_SUCCESS;
}

long celix_bundleArchive_getId(bundle_archive_pt archive) {
    return archive->id;
}

const char* celix_bundleArchive_getSymbolicName(bundle_archive_pt archive) {
    return archive->bundleSymbolicName;
}

celix_status_t bundleArchive_getLocation(bundle_archive_pt archive, const char **location) {
    *location = archive->location;
    return CELIX_SUCCESS;
}

char* celix_bundleArchive_getLocation(bundle_archive_pt archive) {
    char* result = NULL;
    if (archive->location) {
        result = celix_utils_strdup(archive->location);
    }
    return result;
}

celix_status_t bundleArchive_getArchiveRoot(bundle_archive_pt archive, const char** archiveRoot) {
    *archiveRoot = archive->archiveRoot;
    return CELIX_SUCCESS;
}

//LCOV_EXCL_START
celix_status_t bundleArchive_getCurrentRevisionNumber(bundle_archive_pt archive, long* revisionNumber) {
    *revisionNumber = 1; // NOTE bundle revision is deprecated
    return CELIX_SUCCESS;
}
//LCOV_EXCL_STOP

celix_status_t bundleArchive_getCurrentRevision(bundle_archive_pt archive, celix_bundle_revision_t** revision) {
    *revision = archive->revision;
    return CELIX_SUCCESS;
}

//LCOV_EXCL_START
celix_status_t bundleArchive_getRevision(bundle_archive_pt archive, long revNr CELIX_UNUSED, celix_bundle_revision_t** revision) {
    return bundleArchive_getCurrentRevision(archive, revision);
}

celix_status_t bundleArchive_getPersistentState(bundle_archive_pt archive CELIX_UNUSED, bundle_state_e *state) {
    fw_log(archive->fw->logger, CELIX_LOG_LEVEL_DEBUG, "Bundle archive persistent state no longer supported");
    *state = CELIX_BUNDLE_STATE_UNKNOWN;
    return CELIX_SUCCESS;
}

celix_status_t bundleArchive_setPersistentState(bundle_archive_pt archive CELIX_UNUSED, bundle_state_e state  CELIX_UNUSED) {
    fw_log(archive->fw->logger, CELIX_LOG_LEVEL_DEBUG, "Bundle archive persistent state no longer supported");
    return CELIX_SUCCESS;
}

celix_status_t bundleArchive_getRefreshCount(bundle_archive_pt archive CELIX_UNUSED, long *refreshCount) {
    fw_log(archive->fw->logger, CELIX_LOG_LEVEL_DEBUG, "Bundle archive refresh count is no longer supported");
    *refreshCount = 0;
    return CELIX_SUCCESS;
}

celix_status_t bundleArchive_setRefreshCount(bundle_archive_pt archive CELIX_UNUSED) {
    fw_log(archive->fw->logger, CELIX_LOG_LEVEL_DEBUG, "Bundle archive refresh count is no longer supported");
    return CELIX_SUCCESS;
}

celix_status_t bundleArchive_getLastModified(bundle_archive_pt archive, time_t* lastModified) {
    struct timespec mod;
    celix_status_t status = celix_bundleArchive_getLastModified(archive, &mod);
    if (status == CELIX_SUCCESS) {
        *lastModified = mod.tv_sec;
    }
    return status;
}
//LCOV_EXCL_STOP

celix_status_t celix_bundleArchive_getLastModified(bundle_archive_pt archive, struct timespec* lastModified) {
    celix_status_t status;
    status = celix_utils_getLastModified(archive->resourceCacheRoot, lastModified);
    return status;
}

//LCOV_EXCL_START
celix_status_t bundleArchive_setLastModified(bundle_archive_pt archive CELIX_UNUSED, time_t lastModifiedTime  CELIX_UNUSED) {
    celix_status_t status = CELIX_SUCCESS;
    char manifestPathBuffer[CELIX_DEFAULT_STRING_CREATE_BUFFER_SIZE];
    char* manifestPath = celix_utils_writeOrCreateString(manifestPathBuffer, sizeof(manifestPathBuffer), "%s/%s", archive->resourceCacheRoot, CELIX_BUNDLE_MANIFEST_REL_PATH);
    status = celix_utils_touch(manifestPath);
    celix_utils_freeStringIfNotEqual(manifestPathBuffer, manifestPath);
    return status;
}

celix_status_t bundleArchive_revise(bundle_archive_pt archive, const char * location CELIX_UNUSED, const char *updatedBundleUrl) {
    fw_log(archive->fw->logger, CELIX_LOG_LEVEL_ERROR, "Revise not supported.");
    return CELIX_BUNDLE_EXCEPTION;
}
celix_status_t bundleArchive_rollbackRevise(bundle_archive_pt archive, bool* rolledback) {
    *rolledback = true;
    fw_log(archive->fw->logger, CELIX_LOG_LEVEL_ERROR, "Revise rollback not supported.");
    return CELIX_BUNDLE_EXCEPTION;
}

celix_status_t bundleArchive_close(bundle_archive_pt archive) {
    // close revision
    // not yet needed/possible
    return CELIX_SUCCESS;
}

celix_status_t bundleArchive_closeAndDelete(bundle_archive_pt archive) {
    fw_log(archive->fw->logger,
           CELIX_LOG_LEVEL_DEBUG,
           "Usage of bundleArchive_closeAndDelete is deprecated and no longer needed. Called for bundle %s",
           archive->bundleSymbolicName);
    return CELIX_SUCCESS;
}
//LCOV_EXCL_STOP

const char* celix_bundleArchive_getPersistentStoreRoot(bundle_archive_t* archive) {
    return archive->storeRoot;
}

const char* celix_bundleArchive_getCurrentRevisionRoot(bundle_archive_t* archive) {
    return archive->resourceCacheRoot;
}


void celix_bundleArchive_invalidate(bundle_archive_pt archive) {
    archive->valid = false;
    archive->cacheValid = false;
}

void celix_bundleArchive_invalidateCache(bundle_archive_pt archive) {
    archive->cacheValid = false;
}

bool celix_bundleArchive_isCacheValid(bundle_archive_pt archive) {
    return archive->cacheValid;
}

void celix_bundleArchive_removeInvalidDirs(bundle_archive_pt archive) {
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
