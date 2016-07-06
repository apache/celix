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
 * bundle_archive_mock.c
 *
 *  \date       Feb 11, 2013
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include "CppUTestExt/MockSupport_c.h"

#include "bundle_archive.h"

celix_status_t bundleArchive_create(const char * archiveRoot, long id, const char * location, const char *inputFile, bundle_archive_pt *bundle_archive) {
	mock_c()->actualCall("bundleArchive_create")
			->withStringParameters("archiveRoot", archiveRoot)
			->withIntParameters("id", id)
			->withStringParameters("location", location)
			->withStringParameters("inputFile", inputFile)
			->withOutputParameter("bundle_archive", (void **) bundle_archive);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleArchive_createSystemBundleArchive(bundle_archive_pt *bundle_archive) {
	mock_c()->actualCall("bundleArchive_createSystemBundleArchive")
			->withOutputParameter("bundle_archive", (void **) bundle_archive);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleArchive_recreate(const char * archiveRoot, bundle_archive_pt *bundle_archive) {
	mock_c()->actualCall("bundleArchive_recreate")
			->withStringParameters("archiveRoot", archiveRoot)
			->withOutputParameter("bundle_archive", (void **) bundle_archive);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleArchive_destroy(bundle_archive_pt archive) {
    mock_c()->actualCall("bundleArchive_destroy");
    return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleArchive_getId(bundle_archive_pt archive, long *id) {
	mock_c()->actualCall("bundleArchive_getId")
			->withPointerParameters("archive", archive)
			->withOutputParameter("id", id);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleArchive_getLocation(bundle_archive_pt archive, const char **location) {
	mock_c()->actualCall("bundleArchive_getLocation")
			->withPointerParameters("archive", archive)
			->withOutputParameter("location", location);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleArchive_getArchiveRoot(bundle_archive_pt archive, const char **archiveRoot) {
	mock_c()->actualCall("bundleArchive_getArchiveRoot")
			->withPointerParameters("archive", archive)
			->withOutputParameter("archiveRoot", archiveRoot);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleArchive_revise(bundle_archive_pt archive, const char * location, const char *inputFile) {
	mock_c()->actualCall("bundleArchive_revise")
			->withPointerParameters("archive", archive)
			->withStringParameters("location", location)
			->withStringParameters("inputFile", inputFile);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleArchive_rollbackRevise(bundle_archive_pt archive, bool *rolledback) {
	mock_c()->actualCall("bundleArchive_rollbackRevise")
			->withPointerParameters("archive", archive)
			->withOutputParameter("rolledback", rolledback);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleArchive_getRevision(bundle_archive_pt archive, long revNr, bundle_revision_pt *revision) {
	mock_c()->actualCall("bundleArchive_getRevision")
			->withPointerParameters("archive", archive)
			->withLongIntParameters("revNr", revNr)
			->withOutputParameter("revision", revision);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleArchive_getCurrentRevision(bundle_archive_pt archive, bundle_revision_pt *revision) {
	mock_c()->actualCall("bundleArchive_getCurrentRevision")
        ->withPointerParameters("archive", archive)
        ->withOutputParameter("revision", (void **) revision);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleArchive_getCurrentRevisionNumber(bundle_archive_pt archive, long *revisionNumber) {
	mock_c()->actualCall("bundleArchive_getCurrentRevisionNumber")
			->withPointerParameters("archive", archive)
			->withOutputParameter("revisionNumber", revisionNumber);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleArchive_getRefreshCount(bundle_archive_pt archive, long *refreshCount) {
	mock_c()->actualCall("bundleArchive_getRefreshCount")
			->withPointerParameters("archive", archive)
			->withOutputParameter("refreshCount", refreshCount);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleArchive_setRefreshCount(bundle_archive_pt archive) {
	mock_c()->actualCall("bundleArchive_setRefreshCount");
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleArchive_close(bundle_archive_pt archive) {
	mock_c()->actualCall("bundleArchive_close")
			->withPointerParameters("archive", archive);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleArchive_closeAndDelete(bundle_archive_pt archive) {
	mock_c()->actualCall("bundleArchive_closeAndDelete");
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleArchive_setLastModified(bundle_archive_pt archive, time_t lastModifiedTime) {
	mock_c()->actualCall("bundleArchive_setLastModified");
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleArchive_getLastModified(bundle_archive_pt archive, time_t *lastModified) {
	mock_c()->actualCall("bundleArchive_getLastModified")
			->withPointerParameters("archive", archive)
			->withOutputParameter("lastModified", lastModified);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleArchive_setPersistentState(bundle_archive_pt archive, bundle_state_e state) {
	mock_c()->actualCall("bundleArchive_setPersistentState")
			->withPointerParameters("archive", archive)
			->withIntParameters("state", state);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleArchive_getPersistentState(bundle_archive_pt archive, bundle_state_e *state) {
	mock_c()->actualCall("bundleArchive_getPersistentState")
			->withPointerParameters("archive", archive)
			->withOutputParameter("state", state);
	return mock_c()->returnValue().value.intValue;
}

