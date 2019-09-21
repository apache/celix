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
 * bundle_revision_mock.c
 *
 *  \date       Feb 11, 2013
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include "CppUTestExt/MockSupport_c.h"

#include "bundle_revision.h"

celix_status_t bundleRevision_create(const char *root, const char *location, long revisionNr, const char *inputFile, bundle_revision_pt *bundle_revision) {
	mock_c()->actualCall("bundleRevision_create")
			->withStringParameters("root", root)
			->withStringParameters("location", location)
			->withLongIntParameters("revisionNr", revisionNr)
			->withStringParameters("inputFile", inputFile)
			->withOutputParameter("bundle_revision", bundle_revision);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleRevision_destroy(bundle_revision_pt revision) {
    mock_c()->actualCall("bundleRevision_destroy");
    return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleRevision_getNumber(bundle_revision_pt revision, long *revisionNr) {
	mock_c()->actualCall("bundleRevision_getNumber")
			->withPointerParameters("revision", revision)
			->withOutputParameter("revisionNr", revisionNr);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleRevision_getLocation(bundle_revision_pt revision, const char **location) {
	mock_c()->actualCall("bundleRevision_getLocation")
			->withPointerParameters("revision", revision)
			->withOutputParameter("location", location);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleRevision_getRoot(bundle_revision_pt revision, const char **root) {
	mock_c()->actualCall("bundleRevision_getRoot")
			->withPointerParameters("revision", revision)
			->withOutputParameter("root", root);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleRevision_getManifest(bundle_revision_pt revision, manifest_pt *manifest) {
    mock_c()->actualCall("bundleRevision_getManifest")
        ->withPointerParameters("revision", revision)
        ->withOutputParameter("manifest", (void **) manifest);
    return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleRevision_getHandles(bundle_revision_pt revision, array_list_pt *handles) {
    mock_c()->actualCall("bundleRevision_getHandles")
        ->withPointerParameters("revision", revision)
        ->withOutputParameter("handles", (void **) handles);
    return mock_c()->returnValue().value.intValue;
}

