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
/**
 * celix_bundle_archive.h
 *
 *  \date       Aug 8, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef BUNDLE_ARCHIVE_H_
#define BUNDLE_ARCHIVE_H_

#include "celix_types.h"
#include <stdbool.h>
#include <stdlib.h>

#include "celix_bundle_state.h"
#include "celix_errno.h"
#include "celix_framework_export.h"
#include "celix_bundle_manifest_type.h"
#include "celix_cleanup.h"

#define CELIX_BUNDLE_ARCHIVE_STATE_PROPERTIES_FILE_NAME "bundle_state.properties"

#define CELIX_BUNDLE_ARCHIVE_SYMBOLIC_NAME_PROPERTY_NAME "bundle.symbolic_name"
#define CELIX_BUNDLE_ARCHIVE_VERSION_PROPERTY_NAME "bundle.version"
#define CELIX_BUNDLE_ARCHIVE_BUNDLE_ID_PROPERTY_NAME "bundle.id"
#define CELIX_BUNDLE_ARCHIVE_LOCATION_PROPERTY_NAME "bundle.location"

#define CELIX_BUNDLE_ARCHIVE_RESOURCE_CACHE_NAME "resources"
#define CELIX_BUNDLE_ARCHIVE_STORE_DIRECTORY_NAME "storage"

#define CELIX_BUNDLE_MANIFEST_REL_PATH "META-INF/MANIFEST.json"

#ifdef __cplusplus
extern "C" {
#endif

 /**
 * @brief Create bundle archive.
 * Create a bundle archive for the given root, id, location and revision nr.
 * Also create the bundle cache dir and if will reuse a existing bundle resource cache dir if the provided
 * bundle zip location is older then the existing bundle resource cache dir.
 */
celix_status_t celix_bundleArchive_create(celix_framework_t* fw, const char *archiveRoot, long id, const char *location, celix_bundle_archive_t** bundle_archive);

void celix_bundleArchive_destroy(celix_bundle_archive_t* archive);

 /**
 * Define the cleanup function for a bundle_archive_t, so that it can be used with celix_autoptr.
 */
 CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(celix_bundle_archive_t, celix_bundleArchive_destroy);

/**
 * @brief Returns the bundle id of the bundle archive.
 * @param archive The bundle archive.
 * @return The bundle id.
 */
long celix_bundleArchive_getId(const celix_bundle_archive_t* archive);

/**
 * @brief Return the manifest for the bundle archive. All bundle archives have a manifest.
 */
celix_bundle_manifest_t* celix_bundleArchive_getManifest(const celix_bundle_archive_t* archive);

/**
 * @brief Return the bundle symbolic name (from the manifest)
 */
const char* celix_bundleArchive_getSymbolicName(const celix_bundle_archive_t* archive);

/**
 * Returns the root of the bundle persistent store.
 */
const char* celix_bundleArchive_getPersistentStoreRoot(const celix_bundle_archive_t *archive);

 /**
  * Returns the root of the current revision.
  */
const char* celix_bundleArchive_getCurrentRevisionRoot(const celix_bundle_archive_t* archive);

/**
 * @brief Invalidate the whole bundle archive.
 */
void celix_bundleArchive_invalidate(celix_bundle_archive_t* archive);

/**
 * @brief Invalidate the bundle archive's bundle cache.
 */
void celix_bundleArchive_invalidateCache(celix_bundle_archive_t* archive);

/**
 * @brief Return if the bundle cache is valid.
 */
bool celix_bundleArchive_isCacheValid(const celix_bundle_archive_t* archive);

/**
 * @brief Remove all valid directories of the bundle archive.
 */
void celix_bundleArchive_removeInvalidDirs(celix_bundle_archive_t* archive);

 /**
 * @brief Return the location of the bundle archive.b
 * @param[in] archive The bundle archive.
 * @return The location of the bundle archive. The location is valid as long as the archive is valid.
 */
const char* celix_bundleArchive_getLocation(const celix_bundle_archive_t* archive);

/**
 * @brief Return the archive root for this bundle archive.
 * @param[in] archive The bundle archive.
 * @return The archive root of the bundle archive. The archive root is valid as long as the archive is valid.
 */
const char* celix_bundleArchive_getArchiveRoot(const celix_bundle_archive_t* archive);

/**
 * @brief Return the last modified time of the bundle archive.
 *
 * The last modified time is based on the last modified time of the bundle archive cache directory.
 *
 * If the bundle archive cache directory does not exist, lastModified will be set to 0.
 *
 * @param[in] archive The bundle archive.
 * @param[out] lastModified The last modified time of the bundle archive.
 * @return Status code indication failure or success:
 *      - CELIX_SUCCESS when no errors are encountered.
 *      - CELIX_FILE_IO_EXCEPTION if the last modified time could not be retrieved.
 *        Check errno for more specific error information.when root of the archive is not a directory.
 *      - CELIX_ENOMEM not enough memory for manifest file path.
 */
celix_status_t celix_bundleArchive_getLastModified(const celix_bundle_archive_t* archive, struct timespec* lastModified);

#ifdef __cplusplus
}
#endif

#endif /* BUNDLE_ARCHIVE_H_ */
