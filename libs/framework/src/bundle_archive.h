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
 * bundle_archive.h
 *
 *  \date       Aug 8, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef BUNDLE_ARCHIVE_H_
#define BUNDLE_ARCHIVE_H_

#include "celix_types.h"

#include <time.h>
#include <stdbool.h>
#include <stdlib.h>

#include "bundle_revision_private.h"
#include "celix_bundle_state.h"
#include "celix_errno.h"
#include "celix_log.h"

#include "bundle_context.h"
#include "celix_bundle_context.h"
#include "celix_framework_export.h"

#ifdef __cplusplus
extern "C" {
#endif

CELIX_FRAMEWORK_DEPRECATED celix_status_t bundleArchive_getId(bundle_archive_pt archive, long *id);

/**
 * Return the current location of the bundle.
 * @warning Not safe, because location can change during bundle revise.
 * @return
 */
CELIX_FRAMEWORK_DEPRECATED celix_status_t bundleArchive_getLocation(bundle_archive_pt archive, const char **location);

CELIX_FRAMEWORK_DEPRECATED celix_status_t bundleArchive_getArchiveRoot(bundle_archive_pt archive, const char **archiveRoot);

CELIX_FRAMEWORK_DEPRECATED celix_status_t
bundleArchive_revise(bundle_archive_pt archive, const char *location, const char *inputFile);

CELIX_FRAMEWORK_DEPRECATED celix_status_t bundleArchive_rollbackRevise(bundle_archive_pt archive, bool *rolledback);

CELIX_FRAMEWORK_DEPRECATED celix_status_t
bundleArchive_getRevision(bundle_archive_pt archive, long revNr, celix_bundle_revision_t** revision);

CELIX_FRAMEWORK_DEPRECATED celix_status_t
bundleArchive_getCurrentRevision(bundle_archive_pt archive, celix_bundle_revision_t** revision);

CELIX_FRAMEWORK_DEPRECATED celix_status_t bundleArchive_getCurrentRevisionNumber(bundle_archive_pt archive, long *revisionNumber);

CELIX_FRAMEWORK_DEPRECATED celix_status_t bundleArchive_getRefreshCount(bundle_archive_pt archive, long *refreshCount);

CELIX_FRAMEWORK_DEPRECATED celix_status_t bundleArchive_setRefreshCount(bundle_archive_pt archive);

CELIX_FRAMEWORK_DEPRECATED celix_status_t bundleArchive_close(bundle_archive_pt archive);

CELIX_FRAMEWORK_DEPRECATED celix_status_t bundleArchive_setLastModified(bundle_archive_pt archive, time_t lastModifiedTime);

CELIX_FRAMEWORK_DEPRECATED celix_status_t bundleArchive_getLastModified(bundle_archive_pt archive, time_t *lastModified);

CELIX_FRAMEWORK_DEPRECATED celix_status_t bundleArchive_setPersistentState(bundle_archive_pt archive, bundle_state_e state);

CELIX_FRAMEWORK_DEPRECATED celix_status_t bundleArchive_getPersistentState(bundle_archive_pt archive, bundle_state_e *state);

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
CELIX_FRAMEWORK_DEPRECATED celix_status_t celix_bundleArchive_getLastModified(bundle_archive_pt archive, struct timespec* lastModified);

/**
 * @brief Return the location of the bundle archive.b
 * @param[in] archive The bundle archive.
 * @return The location of the bundle archive. The caller is responsible for freeing the returned string.
 */
CELIX_FRAMEWORK_DEPRECATED char* celix_bundleArchive_getLocation(bundle_archive_pt archive);


#ifdef __cplusplus
}
#endif

#endif /* BUNDLE_ARCHIVE_H_ */
