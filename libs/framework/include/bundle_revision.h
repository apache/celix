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
 *
 * @defgroup BundleRevision Bundle Revision
 * @ingroup framework
 * @{
 *
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \date      	April 12, 2011
 *  \copyright	Apache License, Version 2.0
 */

#ifndef BUNDLE_REVISION_H_
#define BUNDLE_REVISION_H_

#include <stdio.h>

#include "celix_types.h"

#include "celix_errno.h"
#include "manifest.h"
#include "celix_log.h"
#include "array_list.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 *
 * A bundle revision represents the content of a bundle. A revision is associated with a bundle archive.
 * An archive can have multiple revisions, each update of a bundle results in a new one.
 *
 * In a revision the content of a bundle (ZIP file) is extracted to a specified location inside the archive.
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
celix_status_t bundleRevision_getNumber(bundle_revision_pt revision, long *revisionNr);

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
celix_status_t bundleRevision_getLocation(bundle_revision_pt revision, const char **location);

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
celix_status_t bundleRevision_getRoot(bundle_revision_pt revision, const char **root);

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
celix_status_t bundleRevision_getManifest(bundle_revision_pt revision, manifest_pt *manifest);

/**
 * Retrieves the handles of the installed libraries for this revision.
 *
 * @param revision The revision to get the manifest for.
 * @param[out] handles celix_array_list_t *containing the handles.
 *
 * @return Status code indication failure or success:
 *      - CELIX_SUCCESS when no errors are encountered.
 *      - CELIX_ILLEGAL_ARGUMENT If <code>revision</code> is illegal.
 */
celix_status_t bundleRevision_getHandles(bundle_revision_pt revision, celix_array_list_t **handles);

#ifdef __cplusplus
}
#endif

#endif /* BUNDLE_REVISION_H_ */

/**
 * @}
 */
