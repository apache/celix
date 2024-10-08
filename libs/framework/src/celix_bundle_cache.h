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

#ifndef CELIX_BUNDLE_CACHE_H_
#define CELIX_BUNDLE_CACHE_H_

#include <stdbool.h>

#include "celix_array_list.h"
#include "celix_framework.h"
#include "celix_long_hash_map.h"

#include "bundle_archive.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Type definition for the celix_bundle_cache_t* abstract data type.
 */
typedef struct celix_bundle_cache celix_bundle_cache_t;

/**
 * @brief Creates the bundle cache using the supplied configuration map.
 *
 * @param fw The Celix framework to create bundle cache for.
 * Its UUID can be used in cache directory naming if a tmp dir used.
 * Its configuration is checked for cache behaviour configuration:
 *  - CELIX_FRAMEWORK_FRAMEWORK_STORAGE
 *  - CELIX_FRAMEWORK_CACHE_USE_TMP_DIR
 * @param out The bundle cache output param. Cannot be NULL.
 * @return CELIX_SUCCESS if bundle cache is successfully created.
 */
celix_status_t
celix_bundleCache_create(celix_framework_t* fw, celix_bundle_cache_t** out);

/**
 * @brief Destroy the bundle cache, releasing all resources allocated in celix_bundleCache_create
 *
 * @param cache The bundle cache to destroy.
 * @return Status code indication failure or success:
 *      - CELIX_SUCCESS when no errors are encountered.
 */
celix_status_t celix_bundleCache_destroy(celix_bundle_cache_t* cache);

/**
 * @brief Creates a new archive for the given bundle (using the id and location).
 *
 * @param cache The bundle cache to create an archive in.
 * @param id The id of the bundle
 * @param location The location identifier of the bundle
 * @param archive The archive to create
 *
 * @return Status code indication failure or success:
 * 		- CELIX_SUCCESS when no errors are encountered.
 * 		- CELIX_ILLEGAL_ARGUMENT If <code>bundle_archive</code> not is null.
 * 		- CELIX_ENOMEM If allocating memory for <code>bundle_archive</code> failed.
 */
celix_status_t
celix_bundleCache_createArchive(celix_bundle_cache_t* cache, long id, const char* location, bundle_archive_pt* archive);

/**
 * @@brief Creates a new system archive for framework bundle.
 * @param[in] fw The Celix framework to create an archive in
 * @param[out] archive  The archive to create
 * @return Status code indication failure or success:
 *         - CELIX_SUCCESS when no errors are encountered.
 *         - CELIX_ILLEGAL_ARGUMENT If <code>bundle_archive</code> not is null.
 *         - CELIX_ENOMEM If allocating memory for <code>bundle_archive</code> failed.
 *         - CELIX_FILE_IO_EXCEPTION If the cache cannot be opened or read.
 *         - CELIX_BUNDLE_EXCEPTION If the bundle cannot be created.
 */
celix_status_t celix_bundleCache_createSystemArchive(celix_framework_t* fw, bundle_archive_pt* archive);

/**
 * @brief Destroy the archive from the cache.
 * It releases all resources allocated in celix_bundleCache_createArchive and deletes invalid directories if any.
 * @param [in] cache The bundle cache to destroy archive from.
 * @param [in] archive The archive to destroy.
 */
void celix_bundleCache_destroyArchive(celix_bundle_cache_t* cache, bundle_archive_pt archive);

/**
 * @brief Deletes the entire bundle cache.
 *
 * @param cache the cache to delete
 * @return Status code indication failure or success:
 * 		- CELIX_SUCCESS when no errors are encountered.
 * 		- CELIX_ILLEGAL_ARGUMENT If the cache is invalid
 * 		- CELIX_FILE_IO_EXCEPTION If the cache cannot be opened or read.
 */
celix_status_t celix_bundleCache_deleteCacheDir(celix_bundle_cache_t* cache);

/**
 * @brief Find if the there is already a bundle cache for the provided bundle zip location and if this is true
 * return the bundle id for the bundle cache entry.
 *
 * @param[in]  cache The cache.
 * @param[in] location The location of the bundle zip to find the id for.
 * @param[out] outBndId The bundle id for the bundle cache entry.
 * @return CELIX_SUCCESS if the bundle id is found and CELIX_FILE_IO_EXCEPTION if the cache cannot be opened or read.
 */
celix_status_t
celix_bundleCache_findBundleIdForLocation(celix_bundle_cache_t* cache, const char* location, long* outBndId);

/**
 * @brief Find if the there is already a bundle cache for the provided bundle id.
 * @param cache The bundle cache.
 * @param bndId  The bundle id to find the bundle cache for.
 * @return Whether the bundle id is already used in a bundle cache entry.
 */
bool celix_bundleCache_isBundleIdAlreadyUsed(celix_bundle_cache_t* cache, long bndId);

/**
 * Clean existing cache dir and create the bundle archives cache for all the bundles configured for starting and
 * installed.
 *
 * For bundle ids, the first bundle will have CELIX_FRAMEWORK_BUNDLE_ID+1 and the next CELIX_FRAMEWORK_BUNDLE_ID+2 etc.
 *
 *  CELIX_AUTO_START_0, CELIX_AUTO_START_1, CELIX_AUTO_START_2, CELIX_AUTO_START_3, CELIX_AUTO_START_4,
 *  CELIX_AUTO_START_5, CELIX_AUTO_START_6 and lastly CELIX_AUTO_INSTALL.
 *
 * @param[in] fw The framework to create the archives for.
 * @param[in] printProgress Whether report progress of bundle archive creation.
 * @return Status code indication failure or success.
 */
celix_status_t celix_bundleCache_createBundleArchivesCache(celix_framework_t* fw, bool printProgress);

#ifdef __cplusplus
}
#endif

#endif /* CELIX_BUNDLE_CACHE_H_ */
