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


#include "bundle_archive.h"
#include "celix_framework.h"
#include "celix_array_list.h"

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
 * @param fwUUID the framework uuid. Can be used in cache directory naming if a tmp dir used.
 * @param config The config properties to use for this cache. The configuration is checked for cache behaviour configuration:
 *  - OSGI_FRAMEWORK_FRAMEWORK_STORAGE
 *  - OSGI_FRAMEWORK_STORAGE_USE_TMP_DIR
 * @param out The bundle cache output param. Cannot be NULL.
 * @return CELIX_SUCCESS if bundle cache is successfully created.
 */
celix_status_t
celix_bundleCache_create(celix_framework_t* fw, celix_bundle_cache_t **out);

/**
 * @brief Frees the bundle_cache memory allocated in celix_bundleCache_create
 *
 * @param bundle_cache parameter for the created cache
 * @return Status code indication failure or success:
 *      - CELIX_SUCCESS when no errors are encountered.
 */
celix_status_t celix_bundleCache_destroy(celix_bundle_cache_t *cache);

/**
 * @brief Recreates and retrieves the list of archives for the given bundle cache.
 *
 * Archives are recreated on the bundle cache memory pool, the list for the results is created on the supplied pool, and is owned by the caller.
 *
 * @param cache The cache to recreate archives out
 * @param pool The pool on which the list of archives is created
 * @param archives List with recreated archives
 * @return Status code indication failure or success:
 * 		- CELIX_SUCCESS when no errors are encountered.
 * 		- CELIX_ILLEGAL_ARGUMENT If <code>archives</code> not is null.
 * 		- CELIX_ENOMEM If allocating memory for <code>archives</code> failed.
 * 		- CELIX_FILE_IO_EXCEPTION If the cache cannot be opened or read.
 */
celix_status_t celix_bundleCache_getArchives(celix_bundle_cache_t *cache, celix_array_list_t **archives);

/**
 * @brief Creates a new archive for the given bundle (using the id and location). The archive is created on the supplied bundlePool.
 *
 * @param cache The Celix framework to create an archive in
 * @param bundlePool The pool to use for the archive creation
 * @param id The id of the bundle
 * @param location The location identifier of the bundle
 * @param inputFile Input identifier to read the bundle data from
 * @param archive The archive to create
 *
 * @return Status code indication failure or success:
 * 		- CELIX_SUCCESS when no errors are encountered.
 * 		- CELIX_ILLEGAL_ARGUMENT If <code>bundle_archive</code> not is null.
 * 		- CELIX_ENOMEM If allocating memory for <code>bundle_archive</code> failed.
 */
celix_status_t
celix_bundleCache_createArchive(celix_framework_t *fw, long id, const char *location, const char *inputFile,
                                bundle_archive_pt *archive);

/**
 * @brief Deletes the entire bundle cache.
 *
 * @param cache the cache to delete
 * @return Status code indication failure or success:
 * 		- CELIX_SUCCESS when no errors are encountered.
 * 		- CELIX_ILLEGAL_ARGUMENT If the cache is invalid
 * 		- CELIX_FILE_IO_EXCEPTION If the cache cannot be opened or read.
 */
celix_status_t celix_bundleCache_delete(celix_bundle_cache_t *cache);

#ifdef __cplusplus
}
#endif

#endif /* CELIX_BUNDLE_CACHE_H_ */
