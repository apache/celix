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

#include <stdlib.h>

#include "celix_utils.h"
#include "bundle_revision_private.h"
#include "framework_private.h"

celix_status_t bundleRevision_create(celix_framework_t* fw, const char *root, const char *location, long revisionNr, manifest_pt manifest, bundle_revision_pt *bundle_revision) {
    celix_status_t status = CELIX_SUCCESS;
	bundle_revision_pt revision = calloc(1, sizeof(*revision));
    if (revision != NULL) {
        revision->fw = fw;
        revision->libraryHandles = celix_arrayList_create();
        revision->revisionNr = revisionNr;
        revision->manifest = manifest;
    }

    if (revision == NULL || revision->libraryHandles == NULL) {
        status = CELIX_ENOMEM;
        fw_logCode(fw->logger, CELIX_LOG_LEVEL_ERROR, status, "Cannot create bundle revision, out of memory");
        manifest_destroy(manifest);
        if (revision != NULL) {
            revision->manifest = NULL;
            bundleRevision_destroy(revision);
        }
        return status;
    }

    bool isSystemRevision = root == NULL && location == NULL;
    if (!isSystemRevision) {
        revision->location = celix_utils_strdup(location);
        revision->root = celix_utils_strdup(root);
        if (revision->location == NULL || revision->root == NULL) {
            status = CELIX_ENOMEM;
            fw_logCode(fw->logger, CELIX_LOG_LEVEL_ERROR, status, "Cannot create bundle revision, out of memory");
            bundleRevision_destroy(revision);
            return status;
        }
    }

    *bundle_revision = revision;
	return status;
}

bundle_revision_t* bundleRevision_revise(const bundle_revision_t* rev, const char* updatedBundleUrl) {
    bundle_revision_pt newRev = NULL;
    manifest_pt clonedManifest = manifest_clone(rev->manifest);
    bundleRevision_create(rev->fw, rev->root, updatedBundleUrl, rev->revisionNr+1, clonedManifest, &newRev);
    return newRev;
}

celix_status_t bundleRevision_destroy(bundle_revision_pt revision) {
    if (revision != NULL) {
        celix_arrayList_destroy(revision->libraryHandles);
        manifest_destroy(revision->manifest);
        free(revision->root);
        free(revision->location);
        free(revision);
    }
	return CELIX_SUCCESS;
}

celix_status_t bundleRevision_getNumber(const bundle_revision_t* revision, long *revisionNr) {
    *revisionNr = revision->revisionNr;
    return CELIX_SUCCESS;
}

celix_status_t bundleRevision_getLocation(const bundle_revision_t* revision, const char **location) {
    *location = revision->location;
    return CELIX_SUCCESS;
}

celix_status_t bundleRevision_getRoot(const bundle_revision_t* revision, const char **root) {
    *root = revision->root;
    return CELIX_SUCCESS;
}

celix_status_t bundleRevision_getManifest(const bundle_revision_t* revision, manifest_pt* manifest) {
    *manifest = revision->manifest;
    return CELIX_SUCCESS;
}

celix_status_t bundleRevision_getHandles(const bundle_revision_t* revision __attribute__((unused)), celix_array_list_t** handles) {
    //nop, usage deprecated
    if (handles) {
        *handles = celix_arrayList_create();
    }
    return CELIX_SUCCESS;
}

