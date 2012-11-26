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
 * bundle_archive.h
 *
 *  \date       Aug 8, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef BUNDLE_ARCHIVE_H_
#define BUNDLE_ARCHIVE_H_

#include <apr_general.h>

#include "bundle_revision.h"
#include "bundle_state.h"
#include "celix_errno.h"
#include "celixbool.h"

typedef struct bundleArchive * bundle_archive_t;

celix_status_t bundleArchive_create(char * archiveRoot, long id, char * location, char *inputFile, apr_pool_t *mp, bundle_archive_t *bundle_archive);
celix_status_t bundleArchive_createSystemBundleArchive(apr_pool_t *mp, bundle_archive_t *bundle_archive);
celix_status_t bundleArchive_recreate(char * archiveRoot, apr_pool_t *mp, bundle_archive_t *bundle_archive);
celix_status_t bundleArchive_getId(bundle_archive_t archive, long *id);
celix_status_t bundleArchive_getLocation(bundle_archive_t archive, char **location);
celix_status_t bundleArchive_getArchiveRoot(bundle_archive_t archive, char **archiveRoot);

celix_status_t bundleArchive_revise(bundle_archive_t archive, char * location, char *inputFile);
celix_status_t bundleArchive_rollbackRevise(bundle_archive_t archive, bool *rolledback);
celix_status_t bundleArchive_getRevision(bundle_archive_t archive, long revNr, BUNDLE_REVISION *revision);
celix_status_t bundleArchive_getCurrentRevision(bundle_archive_t archive, BUNDLE_REVISION *revision);
celix_status_t bundleArchive_getCurrentRevisionNumber(bundle_archive_t archive, long *revisionNumber);

celix_status_t bundleArchive_getRefreshCount(bundle_archive_t archive, long *refreshCount);
celix_status_t bundleArchive_setRefreshCount(bundle_archive_t archive);

celix_status_t bundleArchive_close(bundle_archive_t archive);
celix_status_t bundleArchive_closeAndDelete(bundle_archive_t archive);

celix_status_t bundleArchive_setLastModified(bundle_archive_t archive, time_t lastModifiedTime);
celix_status_t bundleArchive_getLastModified(bundle_archive_t archive, time_t *lastModified);
celix_status_t bundleArchive_setPersistentState(bundle_archive_t archive, BUNDLE_STATE state);
celix_status_t bundleArchive_getPersistentState(bundle_archive_t archive, BUNDLE_STATE *state);

#endif /* BUNDLE_ARCHIVE_H_ */
