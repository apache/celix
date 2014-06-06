/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */
/*
 * bundle_cache.h
 *
 *  \date       Aug 8, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef BUNDLE_CACHE_H_
#define BUNDLE_CACHE_H_

/**
 * @defgroup BundleCache BundleCache
 * @ingroup framework
 * @{
 */

#include "properties.h"
#include "array_list.h"
#include "bundle_archive.h"
#include "celix_log.h"

/**
 * Type definition for the bundle_cache_pt abstract data type.
 */
typedef struct bundleCache *bundle_cache_pt;

/**
 * Creates the bundle cache using the supplied configuration map.
 *
 * @param configurationMap Set with properties to use for this cache
 * @param mp The memory pool to use for allocation the cache
 * @param bundle_cache Output parameter for the created cache
 * @return Status code indication failure or success:
 * 		- CELIX_SUCCESS when no errors are encountered.
 * 		- CELIX_ILLEGAL_ARGUMENT If <code>bundle_cache</code> not is null.
 * 		- CELIX_ENOMEM If allocating memory for <code>bundle_cache</code> failed.
 */
celix_status_t bundleCache_create(properties_pt configurationMap, framework_logger_pt logger, bundle_cache_pt *bundle_cache);

/**
 * Recreates and retrieves the list of archives for the given bundle cache.
 * Archives are recreated on the bundle cache memory pool, the list for the results is created on the suplied pool, and is owned by the caller.
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
celix_status_t bundleCache_getArchives(bundle_cache_pt cache, array_list_pt *archives);

/**
 * Creates a new archive for the given bundle (using the id and location). The archive is created on the supplied bundlePool.
 *
 * @param cache The cache to create an archive in
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
celix_status_t bundleCache_createArchive(bundle_cache_pt cache, long id, char * location, char *inputFile, bundle_archive_pt *archive);

/**
 * Deletes the entire bundle cache.
 *
 * @param cache the cache to delete
 * @return Status code indication failure or success:
 * 		- CELIX_SUCCESS when no errors are encountered.
 * 		- CELIX_ILLEGAL_ARGUMENT If the cache is invalid
 * 		- CELIX_FILE_IO_EXCEPTION If the cache cannot be opened or read.
 */
celix_status_t bundleCache_delete(bundle_cache_pt cache);

/**
 * @}
 */

#endif /* BUNDLE_CACHE_H_ */
