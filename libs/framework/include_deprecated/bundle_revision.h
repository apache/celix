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

#ifndef BUNDLE_REVISION_H_
#define BUNDLE_REVISION_H_

#include <stdio.h>

#include "celix_types.h"

#include "celix_errno.h"
#include "manifest.h"
#include "celix_log.h"
#include "celix_array_list.h"
#include "celix_framework_export.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 *
 * A bundle revision represents the content of a bundle. A revision is associated with a bundle archive.
 * An archive can have multiple revisions, each update of a bundle results in a new one.
 *
 * In a revision the content of a bundle (ZIP file) is extracted to a specified location inside the archive.
 *
 * @note The bundle revision is immutable and thread safe.
 */

/**
 * Retrieves the revision number of the given revision.
 *
 * @param revision The revision to get the number for.
 * @param[out] revisionNr The revision number.
 *
 * @return Status code indication failure or success:
 * 		- CELIX_SUCCESS when no errors are encountered.
 * 		- CELIX_ILLEGAL_ARGUMENT If <code>revision</code> is illegal.
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t bundleRevision_getNumber(const bundle_revision_t* revision, long *revisionNr) __attribute__((deprecated));

/**
 * Retrieves the location of the given revision.
 *
 * @param revision The revision to get the location for.
 * @param[out] location The location.
 *
 * @return Status code indication failure or success:
 * 		- CELIX_SUCCESS when no errors are encountered.
 * 		- CELIX_ILLEGAL_ARGUMENT If <code>revision</code> is illegal.
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t bundleRevision_getLocation(const bundle_revision_t* revision, const char **location);

/**
 * Retrieves the root of the given revision.
 *
 * @param revision The revision to get the location for.
 * @param[out] root The root.
 *
 * @return Status code indication failure or success:
 * 		- CELIX_SUCCESS when no errors are encountered.
 * 		- CELIX_ILLEGAL_ARGUMENT If <code>revision</code> is illegal.
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t bundleRevision_getRoot(const bundle_revision_t* revision, const char **root);

/**
 * Retrieves the manifest of the given revision.
 *
 * @param revision The revision to get the manifest for.
 * @param[out] manifest The manifest.
 *
 * @return Status code indication failure or success:
 * 		- CELIX_SUCCESS when no errors are encountered.
 * 		- CELIX_ILLEGAL_ARGUMENT If <code>revision</code> is illegal.
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t bundleRevision_getManifest(const bundle_revision_t* revision, manifest_pt *manifest);

/**
 * Originally retrieved the handles of the installed libraries for this revision.
 * Currently deprecated and will not return any handles.
 * @return CELIX_SUCCESS and an empty list.
 */
CELIX_FRAMEWORK_EXPORT celix_status_t bundleRevision_getHandles(const bundle_revision_t* revision, celix_array_list_t **handles)
    __attribute__((deprecated("Deprecated. Library handles are no now handled by the bundle module.")));

#ifdef __cplusplus
}
#endif

#endif /* BUNDLE_REVISION_H_ */

/**
 * @}
 */
