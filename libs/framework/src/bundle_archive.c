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

#include "bundle_archive_private.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include "celix_constants.h"
#include "celix_utils_api.h"
#include "linked_list_iterator.h"
#include "framework_private.h"
#include "celix_file_utils.h"
#include "bundle_revision_private.h"
#include "celix_framework_utils_private.h"


celix_status_t celix_bundleArchive_getLastModifiedInternal(bundle_archive_pt archive, struct timespec *lastModified);

/**
 * The bundle archive which is used to store the bundle data and can be reused when a framework is restarted.
 * The lifecycle of a bundle archive is coupled to the lifecycle of the bundle that is created from the archive.
 *
 * @note The bundle archive is thread safe.
 */
struct bundleArchive {
    //initialed during creation and immutable
	celix_framework_t* fw;
	long id;
	char *archiveRoot;
    char *savedBundleStatePropertiesPath;
    char* storeRoot;
    char* resourceCacheRoot;
    bool isSystemBundle;
    char* bundleSymbolicName; //read from the manifest
    char* bundleVersion; //read from the manifest

    celix_thread_mutex_t lock; //protects below and saving of bundle state properties
    bundle_revision_t* revision; //the current revision
    char* location;
};

static void celix_bundleArchive_storeBundleStateProperties(bundle_archive_pt archive) {
    celix_properties_t* bundleStateProperties = celix_properties_create();
    if (bundleStateProperties != NULL) {
        celixThreadMutex_lock(&archive->lock);
        //set/update bundle cache state properties
        celix_properties_setLong(bundleStateProperties, CELIX_BUNDLE_ARCHIVE_BUNDLE_ID_PROPERTY_NAME, archive->id);
        celix_properties_set(bundleStateProperties, CELIX_BUNDLE_ARCHIVE_LOCATION_PROPERTY_NAME, archive->location);
        celix_properties_set(bundleStateProperties, CELIX_BUNDLE_ARCHIVE_SYMBOLIC_NAME_PROPERTY_NAME,
                             archive->bundleSymbolicName);
        celix_properties_set(bundleStateProperties, CELIX_BUNDLE_ARCHIVE_VERSION_PROPERTY_NAME, archive->bundleVersion);

        //save bundle cache state properties
        celix_properties_store(bundleStateProperties, archive->savedBundleStatePropertiesPath,
                               "Bundle State Properties");
        celixThreadMutex_unlock(&archive->lock);
    }
    celix_properties_destroy(bundleStateProperties);
}

celix_status_t celix_bundleArchive_extractBundle(
        bundle_archive_t* archive,
        const char* revisionRoot,
        const char* bundleUrl) {
    celix_status_t status = CELIX_SUCCESS;
    bool extractBundle = true;

    //get revision mod time;
    struct timespec revisionMod;
    status = celix_bundleArchive_getLastModifiedInternal(archive, &revisionMod);
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
    const char* error;
    status = celix_utils_deleteDirectory(revisionRoot, &error);
    if (status != CELIX_SUCCESS) {
        fw_logCode(archive->fw->logger, CELIX_LOG_LEVEL_ERROR, status, "Failed to remove existing bundle archive revision directory '%s': %s", revisionRoot, error);
        return status;
    }

    status = celix_framework_utils_extractBundle(archive->fw, bundleUrl, revisionRoot);
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
celix_status_t celix_bundleArchive_createCacheDirectory(bundle_archive_pt archive, manifest_pt* manifestOut) {
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
    int rc = asprintf(&archive->storeRoot, "%s/%s", archive->archiveRoot, CELIX_BUNDLE_ARCHIVE_STORE_DIRECTORY_NAME);
    if (rc < 0) {
        fw_log(archive->fw->logger, CELIX_LOG_LEVEL_ERROR, "Failed to initialize archive. Failed to create bundle store dir.");
        return CELIX_ENOMEM;
    }
    status = celix_utils_createDirectory(archive->storeRoot, false, &errorStr);
    if (status != CELIX_SUCCESS) {
        fw_log(archive->fw->logger, CELIX_LOG_LEVEL_ERROR, "Failed to initialize archive. Failed to create bundle store dir: %s", errorStr);
        return status;
    }

    //create bundle revision directory
    rc = asprintf(&archive->resourceCacheRoot, "%s/%s", archive->archiveRoot, CELIX_BUNDLE_ARCHIVE_RESOURCE_CACHE_NAME);
    if (rc < 0) {
        fw_log(archive->fw->logger, CELIX_LOG_LEVEL_ERROR, "Failed to initialize archive. Failed to create bundle revision dir.");
        return CELIX_ENOMEM;
    }

    status = celix_utils_createDirectory(archive->resourceCacheRoot, false, &errorStr);
    if (status != CELIX_SUCCESS) {
        fw_log(archive->fw->logger, CELIX_LOG_LEVEL_ERROR, "Failed to initialize archive. Failed to create bundle revision dir: %s", errorStr);
        return status;
    }

    //extract bundle zip to revision directory
    status = celix_bundleArchive_extractBundle(archive, archive->resourceCacheRoot, archive->location);
    if (status != CELIX_SUCCESS) {
        fw_log(archive->fw->logger, CELIX_LOG_LEVEL_ERROR, "Failed to initialize archive. Failed to extract bundle.");
        return status;
    }

    //read manifest from extracted bundle zip
    *manifestOut = NULL;
    char pathBuffer[512];
    char* manifestPath = celix_utils_writeOrCreateString(pathBuffer, sizeof(pathBuffer), "%s/%s", archive->resourceCacheRoot, CELIX_BUNDLE_MANIFEST_REL_PATH);
    status = manifest_createFromFile(manifestPath, manifestOut);
    celix_utils_freeStringIfNotEqual(pathBuffer, manifestPath);
    if (status != CELIX_SUCCESS) {
        fw_log(archive->fw->logger, CELIX_LOG_LEVEL_ERROR, "Failed to initialize archive. Cannot read manifest.");
        return status;
    }

    //populate bundle symbolic name and version from manifest
    archive->bundleSymbolicName = celix_utils_strdup(manifest_getValue(*manifestOut, OSGI_FRAMEWORK_BUNDLE_SYMBOLICNAME));
    if (archive->bundleSymbolicName == NULL) {
        fw_log(archive->fw->logger, CELIX_LOG_LEVEL_ERROR, "Failed to initialize archive. Cannot read bundle symbolic name.");
        manifest_destroy(*manifestOut);
        return CELIX_BUNDLE_EXCEPTION;
    }
    archive->bundleVersion = celix_utils_strdup(manifest_getValue(*manifestOut, OSGI_FRAMEWORK_BUNDLE_VERSION));
    if (archive->bundleVersion == NULL) {
        fw_log(archive->fw->logger, CELIX_LOG_LEVEL_ERROR, "Failed to initialize archive. Cannot read bundle version.");
        manifest_destroy(*manifestOut);
        return CELIX_BUNDLE_EXCEPTION;
    }

    return status;
}
/**
 * Create a bundle archive for the given root, id, location and revision nr.
 *
 * Also create the bundle cache dir and if will reuse a existing bundle resource cache dir if the provided
 * bundle zip location is older then the existing bundle resource cache dir.
 */
celix_status_t celix_bundleArchive_createArchiveInternal(celix_framework_t* fw, const char* archiveRoot, long id, const char *location, bundle_archive_pt* bundle_archive) {
    celix_status_t status = CELIX_SUCCESS;
    bundle_archive_pt archive = calloc(1, sizeof(*archive));

    if (archive) {
        archive->fw = fw;
        archive->id = id;
        archive->isSystemBundle = id == CELIX_FRAMEWORK_BUNDLE_ID;
        celixThreadMutex_create(&archive->lock, NULL);
    }

    if (archive == NULL) {
        status = CELIX_ENOMEM;
        fw_logCode(fw->logger, CELIX_LOG_LEVEL_ERROR, status, "Could not create archive. Out of memory.");
        bundleArchive_destroy(archive);
        return status;
    }

    int rc;
    if (archive->isSystemBundle) {
        archive->resourceCacheRoot = getcwd(NULL, 0);
        archive->storeRoot = getcwd(NULL, 0);
    } else {
        // assert(location != NULL)
        archive->location = celix_utils_strdup(location);
        archive->archiveRoot = celix_utils_strdup(archiveRoot);
        rc = asprintf(&archive->savedBundleStatePropertiesPath, "%s/%s", archiveRoot,
                      CELIX_BUNDLE_ARCHIVE_STATE_PROPERTIES_FILE_NAME);
        if (rc < 0 || archive->location == NULL || archive->savedBundleStatePropertiesPath == NULL
                || archive->archiveRoot == NULL) {
            status = CELIX_ENOMEM;
            fw_logCode(fw->logger, CELIX_LOG_LEVEL_ERROR, status, "Could not create archive. Out of memory.");
            bundleArchive_destroy(archive);
            return status;
        }
    }

    manifest_pt manifest = NULL;
    if (archive->isSystemBundle) {
        status = manifest_create(&manifest);
    } else {
        status = celix_bundleArchive_createCacheDirectory(archive, &manifest);
    }
    if (status != CELIX_SUCCESS) {
        fw_logCode(fw->logger, CELIX_LOG_LEVEL_ERROR, status, "Could not create archive. Failed to initialize archive or create manifest.");
        bundleArchive_destroy(archive);
        return status;
    }

    if (archive->isSystemBundle) {
        status = bundleRevision_create(fw, archive->archiveRoot, NULL, manifest, &archive->revision);
    } else {
        status = bundleRevision_create(fw, archive->archiveRoot, archive->location, manifest, &archive->revision);
    }
    if (status != CELIX_SUCCESS) {
        fw_logCode(fw->logger, CELIX_LOG_LEVEL_ERROR, status, "Could not create archive. Could not create bundle revision.");
        bundleArchive_destroy(archive);
        return status;
    }

    if (!archive->isSystemBundle) {
        celix_bundleArchive_storeBundleStateProperties(archive);
    }

    *bundle_archive = archive;
    return status;
}

celix_status_t bundleArchive_create(celix_framework_t* fw, const char *archiveRoot, long id, const char *location, bundle_archive_pt *bundle_archive) {
    return celix_bundleArchive_createArchiveInternal(fw, archiveRoot, id, location, bundle_archive);
}

celix_status_t bundleArchive_destroy(bundle_archive_pt archive) {
	if (archive != NULL) {
        free(archive->location);
        free(archive->savedBundleStatePropertiesPath);
        free(archive->archiveRoot);
        free(archive->resourceCacheRoot);
        free(archive->storeRoot);
        free(archive->bundleSymbolicName);
        free(archive->bundleVersion);
        bundleRevision_destroy(archive->revision);
        celixThreadMutex_destroy(&archive->lock);
        free(archive);
	}
	return CELIX_SUCCESS;
}

celix_status_t bundleArchive_getId(bundle_archive_pt archive, long *id) {
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
    celixThreadMutex_lock(&archive->lock);
    *location = archive->location;
    celixThreadMutex_unlock(&archive->lock);
    return CELIX_SUCCESS;
}

celix_status_t bundleArchive_getArchiveRoot(bundle_archive_pt archive, const char **archiveRoot) {
    *archiveRoot = archive->archiveRoot;
    return CELIX_SUCCESS;
}

celix_status_t bundleArchive_getCurrentRevisionNumber(bundle_archive_pt archive, long *revisionNumber) {
    *revisionNumber = 1; //NOTE bundle revision is deprecated
    return CELIX_SUCCESS;
}

celix_status_t bundleArchive_getCurrentRevision(bundle_archive_pt archive, bundle_revision_pt *revision) {
    celixThreadMutex_lock(&archive->lock);
    *revision = archive->revision;
    celixThreadMutex_unlock(&archive->lock);
    return CELIX_SUCCESS;
}

celix_status_t bundleArchive_getRevision(bundle_archive_pt archive, long revNr __attribute__((unused)), bundle_revision_pt *revision) {
    return bundleArchive_getCurrentRevision(archive, revision);
}

celix_status_t bundleArchive_getPersistentState(bundle_archive_pt archive __attribute__((unused)), bundle_state_e *state) {
    fw_log(archive->fw->logger, CELIX_LOG_LEVEL_DEBUG, "Bundle archive persistent state no longer supported");
    *state = CELIX_BUNDLE_STATE_UNKNOWN;
    return CELIX_SUCCESS;
}

celix_status_t bundleArchive_setPersistentState(bundle_archive_pt archive __attribute__((unused)), bundle_state_e state  __attribute__((unused))) {
    fw_log(archive->fw->logger, CELIX_LOG_LEVEL_DEBUG, "Bundle archive persistent state no longer supported");
    return CELIX_SUCCESS;
}

celix_status_t bundleArchive_getRefreshCount(bundle_archive_pt archive __attribute__((unused)), long *refreshCount) {
    fw_log(archive->fw->logger, CELIX_LOG_LEVEL_DEBUG, "Bundle archive refresh count is no longer supported");
    *refreshCount = 0;
    return CELIX_SUCCESS;
}

celix_status_t bundleArchive_setRefreshCount(bundle_archive_pt archive __attribute__((unused))) {
    fw_log(archive->fw->logger, CELIX_LOG_LEVEL_DEBUG, "Bundle archive refresh count is no longer supported");
    return CELIX_SUCCESS;
}

celix_status_t bundleArchive_getLastModified(bundle_archive_pt archive, time_t *lastModified) {
    struct timespec mod;
    celix_status_t status = celix_bundleArchive_getLastModified(archive, &mod);
    if (status == CELIX_SUCCESS) {
        *lastModified = mod.tv_sec;
    }
    return status;
}

celix_status_t celix_bundleArchive_getLastModifiedInternal(bundle_archive_pt archive, struct timespec *lastModified) {
    //precondition: archive->lock is locked
    celix_status_t status = CELIX_SUCCESS;
    char manifestPathBuffer[CELIX_DEFAULT_STRING_CREATE_BUFFER_SIZE];
    char* manifestPath = celix_utils_writeOrCreateString(manifestPathBuffer, sizeof(manifestPathBuffer), "%s/%s", archive->resourceCacheRoot, CELIX_BUNDLE_MANIFEST_REL_PATH);
    status = celix_utils_getLastModified(manifestPath, lastModified);
    celix_utils_freeStringIfNotEqual(manifestPathBuffer, manifestPath);
    return status;
}

celix_status_t celix_bundleArchive_getLastModified(bundle_archive_pt archive, struct timespec* lastModified) {
    celix_status_t status;
    celixThreadMutex_lock(&archive->lock);
    status = celix_bundleArchive_getLastModifiedInternal(archive, lastModified);
    celixThreadMutex_unlock(&archive->lock);
    return status;
}

celix_status_t bundleArchive_setLastModified(bundle_archive_pt archive __attribute__((unused)), time_t lastModifiedTime  __attribute__((unused))) {
    celix_status_t status = CELIX_SUCCESS;
    char manifestPathBuffer[CELIX_DEFAULT_STRING_CREATE_BUFFER_SIZE];
    char* manifestPath = celix_utils_writeOrCreateString(manifestPathBuffer, sizeof(manifestPathBuffer), "%s/%s", archive->resourceCacheRoot, CELIX_BUNDLE_MANIFEST_REL_PATH);
    status = celix_utils_touch(manifestPath);
    celix_utils_freeStringIfNotEqual(manifestPathBuffer, manifestPath);
    return status;
}

celix_status_t bundleArchive_revise(bundle_archive_pt archive, const char * location __attribute__((unused)), const char *updatedBundleUrl) {
    celixThreadMutex_lock(&archive->lock);

    const char* updateUrl = archive->location;
    if (updatedBundleUrl != NULL && strcmp(updateUrl, updatedBundleUrl) != 0) {
        fw_log(archive->fw->logger, CELIX_LOG_LEVEL_INFO, "Updating bundle archive bundle url location to %s", updatedBundleUrl);
        updateUrl = updatedBundleUrl;
    }

    const char *reason = NULL;
    celix_status_t status = celix_bundleArchive_extractBundle(archive, archive->resourceCacheRoot, updateUrl);
    if (status == CELIX_SUCCESS) {
        bundle_revision_t* current = archive->revision;
        bundle_revision_t* revised = bundleRevision_revise(current, updateUrl);
        if (revised != NULL) {
            archive->revision = revised;
            bundleRevision_destroy(current);
        } else {
            status = CELIX_BUNDLE_EXCEPTION;
            reason = "bundle revision creation";
        }
    } else {
        reason = "bundle extraction";
    }
    if (status != CELIX_SUCCESS) {
        goto revise_finished;
    }
    if (archive->location != updateUrl) {
        free(archive->location);
        archive->location = celix_utils_strdup(updateUrl);
    }
revise_finished:
    celixThreadMutex_unlock(&archive->lock);
    framework_logIfError(archive->fw->logger, status, reason, "Cannot update bundle archive %s", updatedBundleUrl);
    return status;
}


celix_status_t bundleArchive_rollbackRevise(bundle_archive_pt archive, bool *rolledback) {
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
	celix_status_t status = CELIX_SUCCESS;

	status = bundleArchive_close(archive);
	if (status == CELIX_SUCCESS) {
		const char* err = NULL;
		status = celix_utils_deleteDirectory(archive->archiveRoot, &err);
		framework_logIfError(archive->fw->logger, status, NULL, "Failed to delete archive root '%s': %s", archive->archiveRoot, err);
	}

	framework_logIfError(archive->fw->logger, status, NULL, "Failed to close and delete archive");

	return status;
}

const char* celix_bundleArchive_getPersistentStoreRoot(bundle_archive_t* archive) {
    return archive->storeRoot;
}

const char* celix_bundleArchive_getCurrentRevisionRoot(bundle_archive_t* archive) {
    return archive->resourceCacheRoot;
}


