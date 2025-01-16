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

#include "celix_bundle_cache.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>

#include "celix_constants.h"
#include "celix_log.h"
#include "celix_properties.h"
#include "celix_file_utils.h"
#include "celix_stdlib_cleanup.h"
#include "celix_utils.h"
#include "celix_bundle_context.h"
#include "framework_private.h"
#include "bundle_archive_private.h"
#include "celix_string_hash_map.h"
#include "celix_stdio_cleanup.h"

//for Celix 3.0 update to a different bundle root scheme
//#define CELIX_BUNDLE_ARCHIVE_ROOT_FORMAT "%s/bundle_%li"

#define CELIX_BUNDLE_ARCHIVE_ROOT_FORMAT "%s/bundle%li"

#define FW_LOG(level, ...) \
    celix_framework_log(cache->fw->logger, (level), __FUNCTION__ , __FILE__, __LINE__, __VA_ARGS__)

struct celix_bundle_cache {
    celix_framework_t* fw;
    char* cacheDir;
    bool deleteOnDestroy;
    bool deleteOnCreate;

    celix_thread_mutex_t mutex; //protects below and access to the cache dir
    celix_string_hash_map_t* locationToBundleIdLookupMap; //key = location, value = bundle id.
    bool locationToBundleIdLookupMapLoaded; //true if the locationToBundleIdLookupMap is loaded from disk
};

static const char* bundleCache_progamName() {
#if defined(__APPLE__) || defined(__FreeBSD__)
    return getprogname();
#elif defined(_GNU_SOURCE)
    return program_invocation_short_name;
#else
    return "";
#endif
}

static const char* celix_bundleCache_cacheDirPath(celix_framework_t* fw) {
    return celix_framework_getConfigProperty(fw,
                                                             CELIX_FRAMEWORK_CACHE_DIR,
                                                             CELIX_FRAMEWORK_CACHE_DIR_DEFAULT,
                                                             NULL);
}

static bool celix_bundleCache_useTmpDir(celix_framework_t* fw) {
    return celix_framework_getConfigPropertyAsBool(fw,
                                                          CELIX_FRAMEWORK_CACHE_USE_TMP_DIR,
                                                          CELIX_FRAMEWORK_CACHE_USE_TMP_DIR_DEFAULT,
                                                          NULL);
}

static bool celix_bundleCache_cleanOnCreate(celix_framework_t* fw) {
    return celix_framework_getConfigPropertyAsBool(fw,
                                                                 CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE,
                                                                 CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE_DEFAULT,
                                                                 NULL);
}

celix_status_t celix_bundleCache_create(celix_framework_t* fw, celix_bundle_cache_t** out) {
    celix_status_t status = CELIX_SUCCESS;

    celix_autofree celix_bundle_cache_t* cache = calloc(1, sizeof(*cache));
    if (!cache) {
        return CELIX_ENOMEM;
    }

    cache->fw = fw;
    bool useTmpDir = celix_bundleCache_useTmpDir(fw);
    cache->deleteOnCreate = celix_bundleCache_cleanOnCreate(fw);
    cache->deleteOnDestroy = useTmpDir; //if tmp dir is used, delete on destroy
    celix_autoptr(celix_string_hash_map_t) locationToBundleIdLookupMap =
        cache->locationToBundleIdLookupMap = celix_stringHashMap_create();
    if (NULL == cache->locationToBundleIdLookupMap) {
        return CELIX_ENOMEM;
    }
    status = celixThreadMutex_create(&cache->mutex, NULL);
    if (status != CELIX_SUCCESS) {
        return status;  // Mutex creation failed, return the error code
    }

    if (useTmpDir) {
        const char* pg = bundleCache_progamName();
        if (pg == NULL) {
            pg = "";
        }
        if (asprintf(&cache->cacheDir, "/tmp/celix-cache-%s-%s", pg, celix_framework_getUUID(fw)) < 0) {
            return CELIX_ENOMEM;  // asprintf failure
        }
    } else {
        const char* cacheDir = celix_bundleCache_cacheDirPath(fw);
        cache->cacheDir = celix_utils_strdup(cacheDir);
    }

    if (NULL == cache->cacheDir) {
        return CELIX_ENOMEM;
    }

    celix_autofree char* cacheDir = cache->cacheDir;

    if (cache->deleteOnCreate) {
        status = celix_bundleCache_deleteCacheDir(cache);
        if (status != CELIX_SUCCESS) {
            fw_logCode(fw->logger, CELIX_LOG_LEVEL_ERROR, status, "Error deleting cache directory: %s", cache->cacheDir);
            return status;
        }
    }

    const char* errorStr;
    status = celix_utils_createDirectory(cache->cacheDir, false, &errorStr);
    if (status != CELIX_SUCCESS) {
        fw_logCode(fw->logger, CELIX_LOG_LEVEL_ERROR, status, "Cannot create bundle cache directory %s, error %s",
                   cache->cacheDir, errorStr);
        return status;
    }

    cache->locationToBundleIdLookupMapLoaded = false;

    *out = cache;  // Cache is returned, no need to steal pointers here.

    return CELIX_SUCCESS;
}


celix_status_t celix_bundleCache_destroy(celix_bundle_cache_t* cache) {
    celix_status_t status = CELIX_SUCCESS;
    if (cache->deleteOnDestroy) {
        status = celix_bundleCache_deleteCacheDir(cache);
    }
    free(cache->cacheDir);
    celix_stringHashMap_destroy(cache->locationToBundleIdLookupMap);
    celixThreadMutex_destroy(&cache->mutex);
    free(cache);
    return status;
}

celix_status_t celix_bundleCache_deleteCacheDir(celix_bundle_cache_t* cache) {
    const char* err = NULL;
    celixThreadMutex_lock(&cache->mutex);
    celix_status_t status = celix_utils_deleteDirectory(cache->cacheDir, &err);
    if (status == CELIX_SUCCESS) {
        celix_stringHashMap_clear(cache->locationToBundleIdLookupMap);
    }
    celixThreadMutex_unlock(&cache->mutex);
    if (status != CELIX_SUCCESS) {
        fw_logCode(cache->fw->logger, CELIX_LOG_LEVEL_ERROR, status, "Cannot delete bundle cache directory %s: %s",
                   cache->cacheDir, err);
    }
    return status;
}

celix_status_t
celix_bundleCache_createArchive(celix_bundle_cache_t* cache, long id, const char* location, bundle_archive_t** archiveOut) {
    celix_status_t status = CELIX_SUCCESS;
    bundle_archive_t* archive = NULL;

    char archiveRootBuffer[CELIX_DEFAULT_STRING_CREATE_BUFFER_SIZE];
    char* archiveRoot = celix_utils_writeOrCreateString(archiveRootBuffer, sizeof(archiveRootBuffer),
                                                        CELIX_BUNDLE_ARCHIVE_ROOT_FORMAT, cache->cacheDir, id);
    if (archiveRoot) {
        celixThreadMutex_lock(&cache->mutex);
        status = celix_bundleArchive_create(cache->fw, archiveRoot, id, location, &archive);
        if (status == CELIX_SUCCESS) {
            celix_stringHashMap_put(cache->locationToBundleIdLookupMap, location, (void*) id);
        }
        celixThreadMutex_unlock(&cache->mutex);
        celix_utils_freeStringIfNotEqual(archiveRootBuffer, archiveRoot);
    } else {
        status = CELIX_ENOMEM;
    }
    if (status != CELIX_SUCCESS) {
        fw_logCode(cache->fw->logger, CELIX_LOG_LEVEL_ERROR, status, "Failed to create archive.");
        return status;
    }
    *archiveOut = archive;
    return status;
}

celix_status_t celix_bundleCache_createSystemArchive(celix_framework_t* fw, bundle_archive_pt* archive) {
    return celix_bundleCache_createArchive(fw->cache, CELIX_FRAMEWORK_BUNDLE_ID, NULL, archive);
}

void celix_bundleCache_destroyArchive(celix_bundle_cache_t* cache, bundle_archive_pt archive) {
    celixThreadMutex_lock(&cache->mutex);
    if (!celix_bundleArchive_isCacheValid(archive)) {
        const char* loc = NULL;
        (void) bundleArchive_getLocation(archive, &loc);
        (void) celix_stringHashMap_remove(cache->locationToBundleIdLookupMap, loc);
    }
    (void)celix_bundleArchive_removeInvalidDirs(archive);
    celixThreadMutex_unlock(&cache->mutex);
    celix_bundleArchive_destroy(archive);
}

/**
 * Update location->bundle id lookup map.
 */
static celix_status_t celix_bundleCache_updateIdForLocationLookupMap(celix_bundle_cache_t* cache) {
    celix_autoptr(DIR) dir = opendir(cache->cacheDir);
    if (dir == NULL) {
        fw_logCode(cache->fw->logger, CELIX_LOG_LEVEL_ERROR, CELIX_BUNDLE_EXCEPTION,
                   "Cannot open bundle cache directory %s", cache->cacheDir);
        return CELIX_FILE_IO_EXCEPTION;
    }
    char archiveRootBuffer[CELIX_DEFAULT_STRING_CREATE_BUFFER_SIZE];
    struct dirent* dent = NULL;
    while ((dent = readdir(dir)) != NULL) {
        if (strncmp(dent->d_name, "bundle", 6) != 0) {
            continue;
        }
        char* bundleStateProperties = celix_utils_writeOrCreateString(archiveRootBuffer, sizeof(archiveRootBuffer),
                                                                      "%s/%s/%s", cache->cacheDir, dent->d_name,
                                                                      CELIX_BUNDLE_ARCHIVE_STATE_PROPERTIES_FILE_NAME);
        celix_auto(celix_utils_string_guard_t) strGuard = celix_utils_stringGuard_init(archiveRootBuffer, bundleStateProperties);
        if (celix_utils_fileExists(bundleStateProperties)) {
            celix_autoptr(celix_properties_t) props = NULL;
            celix_status_t status = celix_properties_load(bundleStateProperties, 0, &props);
            if (status != CELIX_SUCCESS) {
                    fw_logCode(cache->fw->logger, CELIX_LOG_LEVEL_ERROR, status,
                               "Cannot load bundle state properties from %s", bundleStateProperties);
                    celix_framework_logTssErrors(cache->fw->logger, CELIX_LOG_LEVEL_ERROR);
                    return status;
            }
            const char* visitLoc = celix_properties_get(props, CELIX_BUNDLE_ARCHIVE_LOCATION_PROPERTY_NAME, NULL);
            long bndId = celix_properties_getAsLong(props, CELIX_BUNDLE_ARCHIVE_BUNDLE_ID_PROPERTY_NAME, -1);
            if (visitLoc != NULL && bndId >= 0) {
                fw_log(cache->fw->logger, CELIX_LOG_LEVEL_TRACE, "Adding location %s -> bnd id %li to lookup map",
                       visitLoc, bndId);
                celix_stringHashMap_putLong(cache->locationToBundleIdLookupMap, visitLoc, bndId);
            }
        }
    }
    return CELIX_SUCCESS;
}

celix_status_t
celix_bundleCache_findBundleIdForLocation(celix_bundle_cache_t* cache, const char* location, long* outBndId) {
    *outBndId = -1L;
    celix_auto(celix_mutex_lock_guard_t) lck = celixMutexLockGuard_init(&cache->mutex);
    if (!cache->locationToBundleIdLookupMapLoaded) {
        celix_status_t status = celix_bundleCache_updateIdForLocationLookupMap(cache);
        if (status != CELIX_SUCCESS) {
            return status;
        }
        cache->locationToBundleIdLookupMapLoaded = true;
    }
    if (celix_stringHashMap_hasKey(cache->locationToBundleIdLookupMap, location)) {
        *outBndId = celix_stringHashMap_getLong(cache->locationToBundleIdLookupMap, location, -1);
    }
    return CELIX_SUCCESS;
}

bool celix_bundleCache_isBundleIdAlreadyUsed(celix_bundle_cache_t* cache, long bndId) {
    bool found = false;
    celixThreadMutex_lock(&cache->mutex);
    CELIX_STRING_HASH_MAP_ITERATE(cache->locationToBundleIdLookupMap, iter) {
        if (iter.value.longValue == bndId) {
            found = true;
            break;
        }
    }
    celixThreadMutex_unlock(&cache->mutex);
    return found;
}


static celix_status_t
celix_bundleCache_createBundleArchivesForSpaceSeparatedList(celix_framework_t* fw, long* bndId, const char* list,
                                                            bool logProgress) {
    celix_status_t status = CELIX_SUCCESS;
    char delims[] = " ";
    char* savePtr = NULL;
    char zipFileListBuffer[CELIX_DEFAULT_STRING_CREATE_BUFFER_SIZE];
    char* zipFileList = celix_utils_writeOrCreateString(zipFileListBuffer, sizeof(zipFileListBuffer), "%s", list);
    if (zipFileList) {
        char* location = strtok_r(zipFileList, delims, &savePtr);
        while (location != NULL) {
            bundle_archive_t* archive = NULL;
            status = celix_bundleCache_createArchive(fw->cache, (*bndId)++, location, &archive);
            if (status != CELIX_SUCCESS) {
                fw_logCode(fw->logger, CELIX_LOG_LEVEL_ERROR, status,
                           "Cannot create bundle archive for %s", location);
                break;
            } else {
                celix_log_level_e lvl = logProgress ? CELIX_LOG_LEVEL_INFO : CELIX_LOG_LEVEL_DEBUG;
                fw_log(fw->logger, lvl, "Created bundle cache '%s' for bundle archive %s (bndId=%li).",
                       celix_bundleArchive_getCurrentRevisionRoot(archive),
                       celix_bundleArchive_getSymbolicName(archive), celix_bundleArchive_getId(archive));
                celix_bundleArchive_destroy(archive);
            }
            location = strtok_r(NULL, delims, &savePtr);
        }
    } else {
        status = CELIX_ENOMEM;
        fw_logCode(fw->logger, CELIX_LOG_LEVEL_ERROR, status, "Failed to create zip file list.");
    }
    celix_utils_freeStringIfNotEqual(zipFileListBuffer, zipFileList);
    return status;
}

celix_status_t celix_bundleCache_createBundleArchivesCache(celix_framework_t* fw, bool logProgress) {
    celix_status_t status = CELIX_SUCCESS;

    const char* const celixKeys[] = {CELIX_AUTO_START_0, CELIX_AUTO_START_1, CELIX_AUTO_START_2, CELIX_AUTO_START_3,
                                     CELIX_AUTO_START_4, CELIX_AUTO_START_5, CELIX_AUTO_START_6, NULL};
    long bndId = CELIX_FRAMEWORK_BUNDLE_ID + 1; //note cleaning cache, so starting bundle id at 1

    const char* errorStr = NULL;
    status = celix_utils_deleteDirectory(fw->cache->cacheDir, &errorStr);
    if (status != CELIX_SUCCESS) {
        fw_logCode(fw->logger, CELIX_LOG_LEVEL_ERROR, status, "Failed to delete bundle cache directory %s: %s",
                   fw->cache->cacheDir, errorStr);
        return status;
    } else {
        celix_log_level_e lvl = logProgress ? CELIX_LOG_LEVEL_INFO : CELIX_LOG_LEVEL_DEBUG;
        fw_log(fw->logger, lvl, "Deleted bundle cache directory %s", fw->cache->cacheDir);
    }

    for (int i = 0; celixKeys[i] != NULL; ++i) {
        const char* autoStart = celix_framework_getConfigProperty(fw, celixKeys[i], NULL, NULL);
        if (autoStart) {
            status = celix_bundleCache_createBundleArchivesForSpaceSeparatedList(fw, &bndId, autoStart, logProgress);
            if (status != CELIX_SUCCESS) {
                fw_logCode(fw->logger, CELIX_LOG_LEVEL_ERROR, status,
                           "Failed to create bundle archives for auto start list %s", autoStart);
                return status;
            }
        }
    }
    const char* autoInstall = celix_framework_getConfigProperty(fw, CELIX_AUTO_INSTALL, NULL, NULL);
    if (autoInstall) {
        status = celix_bundleCache_createBundleArchivesForSpaceSeparatedList(fw, &bndId, autoInstall, logProgress);
        if (status != CELIX_SUCCESS) {
            fw_logCode(fw->logger, CELIX_LOG_LEVEL_ERROR, status,
                       "Failed to create bundle archives for auto install list %s", autoInstall);
            return status;
        }
    }
    return status;
}
