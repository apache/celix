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
#include <sys/errno.h>

#include "celix_constants.h"
#include "celix_log.h"
#include "celix_properties.h"
#include "celix_file_utils.h"
#include "celix_utils.h"
#include "celix_bundle_context.h"
#include "framework_private.h"
#include "bundle_archive_private.h"
#include "celix_string_hash_map.h"
#include "celix_build_assert.h"

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
    bool found = false;
    const char* cacheDir = celix_framework_getConfigProperty(fw,
        CELIX_FRAMEWORK_FRAMEWORK_CACHE_DIR,
        CELIX_FRAMEWORK_FRAMEWORK_CACHE_DIR_DEFAULT,
        &found);
    if (!found) {
        //falling back to old property
        cacheDir = celix_framework_getConfigProperty(fw,
            CELIX_FRAMEWORK_FRAMEWORK_STORAGE,
            CELIX_FRAMEWORK_FRAMEWORK_CACHE_DIR_DEFAULT,
            NULL);
    }
    return cacheDir;
}

static bool celix_bundleCache_useTmpDir(celix_framework_t* fw) {
    bool converted = false;
    bool useTmp = celix_framework_getConfigPropertyAsBool(fw,
        CELIX_FRAMEWORK_CACHE_USE_TMP_DIR,
        CELIX_FRAMEWORK_CACHE_USE_TMP_DIR_DEFAULT,
        &converted);
    if (!converted) {
        //falling back to old property
        useTmp = celix_framework_getConfigPropertyAsBool(fw,
            CELIX_FRAMEWORK_STORAGE_USE_TMP_DIR,
            CELIX_FRAMEWORK_CACHE_USE_TMP_DIR_DEFAULT,
            NULL);
    }
    return useTmp;
}

static bool celix_bundleCache_cleanOnCreate(celix_framework_t* fw) {
    bool converted = false;
    bool cleanOnCreate = celix_framework_getConfigPropertyAsBool(fw,
            CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE,
            CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE_DEFAULT,
            &converted);
    if (!converted) {
        //falling back to old property
        cleanOnCreate = celix_framework_getConfigPropertyAsBool(fw,
                CELIX_FRAMEWORK_FRAMEWORK_STORAGE_CLEAN_NAME,
                CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE_DEFAULT,
                NULL);
    }
    return cleanOnCreate;
}

celix_status_t celix_bundleCache_create(celix_framework_t* fw, celix_bundle_cache_t **out) {
    celix_status_t status = CELIX_SUCCESS;

    celix_bundle_cache_t *cache = calloc(1, sizeof(*cache));
    if (!cache) {
        status = CELIX_ENOMEM;
        fw_logCode(fw->logger, CELIX_LOG_LEVEL_ERROR, status, "Cannot create bundle cache, out of memory");
        return status;
    }

    bool useTmpDir = celix_bundleCache_useTmpDir(fw);
    cache->deleteOnCreate = celix_bundleCache_cleanOnCreate(fw);
    cache->deleteOnDestroy = useTmpDir; //if tmp dir is used, delete on destroy
    cache->locationToBundleIdLookupMap = celix_stringHashMap_create();
    celixThreadMutex_create(&cache->mutex, NULL);

    if (useTmpDir) {
        //Using /tmp dir for cache, so that multiple frameworks can be launched
        //instead of cacheDir = ".cache";
        const char *pg = bundleCache_progamName();
        if (pg == NULL) {
            pg = "";
        }

        asprintf(&cache->cacheDir, "/tmp/celix-cache-%s-%s", pg, celix_framework_getUUID(fw));
    } else {
        const char* cacheDir = celix_bundleCache_cacheDirPath(fw);
        cache->cacheDir = celix_utils_strdup(cacheDir);
    }

    if (cache->deleteOnCreate) {
        status = celix_bundleCache_deleteCacheDir(cache);
        if (status != CELIX_SUCCESS) {
            celix_bundleCache_destroy(cache);
            return status;
        }
    }

    const char* errorStr;
    status = celix_utils_createDirectory(cache->cacheDir, false, &errorStr);
    if (status != CELIX_SUCCESS) {
        fw_logCode(fw->logger, CELIX_LOG_LEVEL_ERROR, status, "Cannot create bundle cache directory %s, error %s", cache->cacheDir, errorStr);
        celix_bundleCache_destroy(cache);
        return status;
    }

    *out = cache;
	return status;
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
    celixThreadMutex_unlock(&cache->mutex);
    if (status != CELIX_SUCCESS) {
        fw_logCode(cache->fw->logger, CELIX_LOG_LEVEL_ERROR, status, "Cannot delete bundle cache directory %s: %s", cache->cacheDir, err);
    }
    return status;
}

celix_status_t celix_bundleCache_createArchive(celix_framework_t* fw, long id, const char *location, bundle_archive_t** archiveOut) {
	celix_status_t status = CELIX_SUCCESS;
    bundle_archive_t* archive = NULL;

    char archiveRootBuffer[CELIX_DEFAULT_STRING_CREATE_BUFFER_SIZE];
    char *archiveRoot = celix_utils_writeOrCreateString(archiveRootBuffer, sizeof(archiveRootBuffer), CELIX_BUNDLE_ARCHIVE_ROOT_FORMAT, fw->cache->cacheDir, id);
    if (archiveRoot) {
        celixThreadMutex_lock(&fw->cache->mutex);
		status = bundleArchive_create(fw, archiveRoot, id, location, &archive);
        celixThreadMutex_unlock(&fw->cache->mutex);
        if (status != CELIX_SUCCESS) {
            celix_utils_freeStringIfNeeded(archiveRootBuffer, archiveRoot);
            return status;
        }
	} else {
        status = CELIX_ENOMEM;
        fw_logCode(fw->logger, CELIX_LOG_LEVEL_ERROR, status, "Failed to create archive. Out of memory.");
        return status;
    }

    //add bundle id and location to lookup maps
    celixThreadMutex_lock(&fw->cache->mutex);
    celix_stringHashMap_put(fw->cache->locationToBundleIdLookupMap, location, (void*)id);
    celixThreadMutex_unlock(&fw->cache->mutex);

    *archiveOut = archive;
	return status;
}

celix_status_t celix_bundleCache_createSystemArchive(celix_framework_t* fw, bundle_archive_pt* archive) {
    return celix_bundleCache_createArchive(fw, CELIX_FRAMEWORK_BUNDLE_ID, NULL, archive);
}

/**
 * Update location->bundle id lookup map.
 * Assumes that bundle cache dir are not removed, so only adding not removing entries.
 */
static void celix_bundleCache_updateIdForLocationLookupMap(celix_framework_t* fw) {
    celixThreadMutex_lock(&fw->cache->mutex);
    DIR* dir = opendir(fw->cache->cacheDir);
    if (dir == NULL) {
        fw_logCode(fw->logger, CELIX_LOG_LEVEL_ERROR, CELIX_BUNDLE_EXCEPTION, "Cannot open bundle cache directory %s", fw->cache->cacheDir);
        return;
    }
    char archiveRootBuffer[CELIX_DEFAULT_STRING_CREATE_BUFFER_SIZE];
    struct dirent* dent = NULL;
    while ((dent = readdir(dir)) != NULL) {
        if (strncmp(dent->d_name, "bundle", 6) != 0) {
            continue;
        }
        char *bundleStateProperties = celix_utils_writeOrCreateString(archiveRootBuffer, sizeof(archiveRootBuffer),
                                                                      "%s/%s/%s", fw->cache->cacheDir, dent->d_name,
                                                                      CELIX_BUNDLE_ARCHIVE_STATE_PROPERTIES_FILE_NAME);
        if (celix_utils_fileExists(bundleStateProperties)) {
            celix_properties_t *props = celix_properties_load(bundleStateProperties);
            const char *visitLoc = celix_properties_get(props, CELIX_BUNDLE_ARCHIVE_LOCATION_PROPERTY_NAME, NULL);
            long bndId = celix_properties_getAsLong(props, CELIX_BUNDLE_ARCHIVE_BUNDLE_ID_PROPERTY_NAME, -1);
            if (visitLoc != NULL && bndId >= 0) {
                fw_log(fw->logger, CELIX_LOG_LEVEL_TRACE, "Adding location %s -> bnd id %li to lookup map",
                       visitLoc, bndId);
                celix_stringHashMap_putLong(fw->cache->locationToBundleIdLookupMap, visitLoc, bndId);
            }
            celix_properties_destroy(props);
        }
    }
    closedir(dir);
    celixThreadMutex_unlock(&fw->cache->mutex);
}

long celix_bundleCache_findBundleIdForLocation(celix_framework_t *fw, const char *location) {
    long bndId = -1;
    celixThreadMutex_lock(&fw->cache->mutex);
    if (celix_stringHashMap_hasKey(fw->cache->locationToBundleIdLookupMap, location)) {
        bndId = celix_stringHashMap_getLong(fw->cache->locationToBundleIdLookupMap, location, -1);
    }
    celixThreadMutex_unlock(&fw->cache->mutex);
    if (bndId == -1) {
        celix_bundleCache_updateIdForLocationLookupMap(fw);
        celixThreadMutex_lock(&fw->cache->mutex);
        bndId = celix_stringHashMap_getLong(fw->cache->locationToBundleIdLookupMap, location, -1);
        celixThreadMutex_unlock(&fw->cache->mutex);
    }
    return bndId;
}

bool celix_bundleCache_isBundleIdAlreadyUsed(celix_framework_t *fw, long bndId) {
    bool found = false;
    celixThreadMutex_lock(&fw->cache->mutex);
    CELIX_STRING_HASH_MAP_ITERATE(fw->cache->locationToBundleIdLookupMap, iter) {
        if (iter.value.longValue == bndId) {
            found = true;
            break;
        }
    }
    celixThreadMutex_unlock(&fw->cache->mutex);
    return found;
}


static celix_status_t celix_bundleCache_createBundleArchivesForSpaceSeparatedList(celix_framework_t *fw, long* bndId, const char* list, bool logProgress) {
    celix_status_t status = CELIX_SUCCESS;
    char delims[] = " ";
    char *savePtr = NULL;
    char zipFileListBuffer[CELIX_DEFAULT_STRING_CREATE_BUFFER_SIZE];
    char* zipFileList = celix_utils_writeOrCreateString(zipFileListBuffer, sizeof(zipFileListBuffer), "%s", list);
    if (zipFileList) {
        char *location = strtok_r(zipFileList, delims, &savePtr);
        while (location != NULL) {
            bundle_archive_t* archive = NULL;
            status = celix_bundleCache_createArchive(fw, (*bndId)++, location, &archive);
            if (status != CELIX_SUCCESS) {
                fw_logCode(fw->logger, CELIX_LOG_LEVEL_ERROR, CELIX_BUNDLE_EXCEPTION, "Cannot create bundle archive for %s", location);
                break;
            } else {
                celix_log_level_e lvl = logProgress ? CELIX_LOG_LEVEL_INFO : CELIX_LOG_LEVEL_DEBUG;
                fw_log(fw->logger, lvl, "Created bundle cache '%s' for bundle archive %s (bndId=%li).",
                       celix_bundleArchive_getCurrentRevisionRoot(archive),
                       celix_bundleArchive_getSymbolicName(archive), celix_bundleArchive_getId(archive));
                bundleArchive_destroy(archive);
            }
            location = strtok_r(NULL, delims, &savePtr);
        }
    } else {
        status = CELIX_ENOMEM;
        fw_logCode(fw->logger, CELIX_LOG_LEVEL_ERROR, status, "Failed to create zip file list.");
    }
    celix_utils_freeStringIfNeeded(zipFileListBuffer, zipFileList);
    return status;
}

//using tmp cache and that bundle cache is not deleted during bundle cache destroy.
celix_status_t celix_bundleCache_createBundleArchivesCache(celix_framework_t *fw, bool logProgress) {
    celix_status_t status = CELIX_SUCCESS;

    const char* const cosgiKeys[] = {"cosgi.auto.start.0","cosgi.auto.start.1","cosgi.auto.start.2","cosgi.auto.start.3","cosgi.auto.start.4","cosgi.auto.start.5","cosgi.auto.start.6", NULL};
    const char* const celixKeys[] = {CELIX_AUTO_START_0, CELIX_AUTO_START_1, CELIX_AUTO_START_2, CELIX_AUTO_START_3, CELIX_AUTO_START_4, CELIX_AUTO_START_5, CELIX_AUTO_START_6, NULL};
    CELIX_BUILD_ASSERT(sizeof(*cosgiKeys) == sizeof(*celixKeys));
    long bndId = CELIX_FRAMEWORK_BUNDLE_ID+1; //note cleaning cache, so starting bundle id at 1

    const char* errorStr = NULL;
    status = celix_utils_deleteDirectory(fw->cache->cacheDir, &errorStr);
    if (status != CELIX_SUCCESS) {
        fw_logCode(fw->logger, CELIX_LOG_LEVEL_ERROR, status, "Failed to delete bundle cache directory %s: %s", fw->cache->cacheDir, errorStr);
        return status;
    } else {
        celix_log_level_e lvl = logProgress ? CELIX_LOG_LEVEL_INFO : CELIX_LOG_LEVEL_DEBUG;
        fw_log(fw->logger, lvl, "Deleted bundle cache directory %s", fw->cache->cacheDir);
    }

    for (int i = 0; celixKeys[i] != NULL; ++i) {
        const char *autoStart = celix_framework_getConfigProperty(fw, celixKeys[i], NULL, NULL);
        if (autoStart == NULL) {
            autoStart = celix_framework_getConfigProperty(fw, cosgiKeys[i], NULL, NULL);
        }
        if (autoStart) {
            status = celix_bundleCache_createBundleArchivesForSpaceSeparatedList(fw, &bndId, autoStart, logProgress);
            if (status != CELIX_SUCCESS) {
                fw_logCode(fw->logger, CELIX_LOG_LEVEL_ERROR, status, "Failed to create bundle archives for auto start list %s", autoStart);
                return status;
            }
        }
    }
    const char* autoInstall = celix_framework_getConfigProperty(fw, CELIX_AUTO_INSTALL, NULL, NULL);
    if (autoInstall) {
        status = celix_bundleCache_createBundleArchivesForSpaceSeparatedList(fw, &bndId, autoInstall, logProgress);
        if (status != CELIX_SUCCESS) {
            fw_logCode(fw->logger, CELIX_LOG_LEVEL_ERROR, status, "Failed to create bundle archives for auto start list %s", autoInstall);
            return status;
        }
    }
    return status;
}
