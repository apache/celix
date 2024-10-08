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


#ifndef BUNDLE_ARCHIVE_PRIVATE_H_
#define BUNDLE_ARCHIVE_PRIVATE_H_

#include <time.h>

#include "bundle_archive.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CELIX_BUNDLE_ARCHIVE_STATE_PROPERTIES_FILE_NAME "bundle_state.properties"

#define CELIX_BUNDLE_ARCHIVE_SYMBOLIC_NAME_PROPERTY_NAME "bundle.symbolic_name"
#define CELIX_BUNDLE_ARCHIVE_VERSION_PROPERTY_NAME "bundle.version"
#define CELIX_BUNDLE_ARCHIVE_BUNDLE_ID_PROPERTY_NAME "bundle.id"
#define CELIX_BUNDLE_ARCHIVE_LOCATION_PROPERTY_NAME "bundle.location"

#define CELIX_BUNDLE_ARCHIVE_RESOURCE_CACHE_NAME "resources"
#define CELIX_BUNDLE_ARCHIVE_STORE_DIRECTORY_NAME "storage"

#define CELIX_BUNDLE_MANIFEST_REL_PATH "META-INF/MANIFEST.MF"

/**
 * @brief Create bundle archive.
 * Create a bundle archive for the given root, id, location and revision nr.
 * Also create the bundle cache dir and if will reuse a existing bundle resource cache dir if the provided
 * bundle zip location is older then the existing bundle resource cache dir.
 */
celix_status_t celix_bundleArchive_create(celix_framework_t* fw, const char *archiveRoot, long id, const char *location, bundle_archive_pt *bundle_archive);

void celix_bundleArchive_destroy(bundle_archive_pt archive);

/**
 * @brief Returns the bundle id of the bundle archive.
 * @param archive The bundle archive.
 * @return The bundle id.
 */
long celix_bundleArchive_getId(bundle_archive_pt archive);

/**
 * @brief Returns the bundle symbolic name of the bundle archive.
 * @param archive The bundle archive.
 * @return The bundle symbolic name.
 */
const char* celix_bundleArchive_getSymbolicName(bundle_archive_pt archive);

/**
 * Returns the root of the bundle persistent store.
 */
const char* celix_bundleArchive_getPersistentStoreRoot(bundle_archive_t *archive);

 /**
  * Returns the root of the current revision.
  */
const char* celix_bundleArchive_getCurrentRevisionRoot(bundle_archive_pt archive);

/**
 * @brief Invalidate the whole bundle archive.
 */
void celix_bundleArchive_invalidate(bundle_archive_pt archive);

/**
 * @brief Invalidate the bundle archive's bundle cache.
 */
void celix_bundleArchive_invalidateCache(bundle_archive_pt archive);

/**
 * @brief Return if the bundle cache is valid.
 */
bool celix_bundleArchive_isCacheValid(bundle_archive_pt archive);

/**
 * @brief Remove all valid directories of the bundle archive.
 */
void celix_bundleArchive_removeInvalidDirs(bundle_archive_pt archive);

#ifdef __cplusplus
}
#endif

#endif /* BUNDLE_ARCHIVE_PRIVATE_H_ */
