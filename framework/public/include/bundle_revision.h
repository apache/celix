/*
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

#include "celix_errno.h"
#include "manifest.h"
#include "celix_log.h"
#include "array_list.h"

/**
 * Typedef for bundle_revision_pt.
 *
 * A bundle revision represents the content of a bundle. A revision is associated with a bundle archive.
 * An archive can have multiple revisions, each update of a bundle results in a new one.
 *
 * In a revision the content of a bundle (ZIP file) is extracted to a specified location inside the archive.
 */
typedef struct bundleRevision * bundle_revision_pt;

/**
 * Creates a new revision for the given inputFile or location.
 * The location parameter is used to identify the bundle, in case of an update or download, the inputFile
 *  parameter can be used to point to the actual data. In the OSGi specification this is the inputstream.
 *
 * @param pool The pool on which this revision has to be allocated.
 * @param root The root for this revision in which the bundle is extracted and state is stored.
 * @param location The location associated with the revision
 * @param revisionNr The number of the revision
 * @param inputFile The (optional) location of the file to use as input for this revision
 * @param[out] bundle_revision The output parameter for the created revision.
 *
 * @return Status code indication failure or success:
 * 		- CELIX_SUCCESS when no errors are encountered.
 * 		- CELIX_ENOMEM If allocating memory for <code>bundle_revision</code> failed.
 */
celix_status_t bundleRevision_create(framework_logger_pt logger, char *root, char *location, long revisionNr, char *inputFile, bundle_revision_pt *bundle_revision);

celix_status_t bundleRevision_destroy(bundle_revision_pt revision);

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
celix_status_t bundleRevision_getLocation(bundle_revision_pt revision, char **location);

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
celix_status_t bundleRevision_getRoot(bundle_revision_pt revision, char **root);

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
 * @param[out] handles array_list_pt containing the handles.
 *
 * @return Status code indication failure or success:
 *      - CELIX_SUCCESS when no errors are encountered.
 *      - CELIX_ILLEGAL_ARGUMENT If <code>revision</code> is illegal.
 */
celix_status_t bundleRevision_getHandles(bundle_revision_pt revision, array_list_pt *handles);

#endif /* BUNDLE_REVISION_H_ */

/**
 * @}
 */
