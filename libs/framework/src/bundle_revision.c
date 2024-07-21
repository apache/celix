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

#include "celix_compiler.h"
#include "celix_utils.h"
#include "bundle_revision_private.h"
#include "framework_private.h"

celix_status_t celix_bundleRevision_create(celix_framework_t* fw, const char *root, const char *location, celix_bundle_manifest_t* manifest, bundle_revision_pt *bundle_revision) {
    celix_status_t status = CELIX_SUCCESS;
    bundle_revision_pt revision = calloc(1, sizeof(*revision));
    if (revision != NULL) {
        revision->fw = fw;
        if (root != NULL) {
            revision->root = celix_utils_strdup(root);
        }
        if (location != NULL) {
            revision->location = celix_utils_strdup(location);
        }
        revision->manifest = manifest;
    }

    if (revision == NULL || (root != NULL && revision->root == NULL) || (location != NULL && revision->location == NULL)) {
        status = CELIX_ENOMEM;
        fw_logCode(fw->logger, CELIX_LOG_LEVEL_ERROR, status, "Cannot create bundle revision, out of memory");
        if (revision != NULL) {
            bundleRevision_destroy(revision);
        } else {
            celix_bundleManifest_destroy(manifest);
        }
        return status;
    }

    *bundle_revision = revision;
    return status;
}

celix_status_t bundleRevision_destroy(bundle_revision_pt revision) {
    if (revision != NULL) {
        celix_bundleManifest_destroy(revision->manifest);
        free(revision->root);
        free(revision->location);
        free(revision);
    }
    return CELIX_SUCCESS;
}

celix_status_t bundleRevision_getManifest(const bundle_revision_t* revision, celix_bundle_manifest_t** manifest) {
    *manifest = revision->manifest;
    return CELIX_SUCCESS;
}

//LCOV_EXCL_START
bundle_revision_t* bundleRevision_revise(const bundle_revision_t* rev, const char* updatedBundleUrl) {
    fw_log(rev->fw->logger, CELIX_LOG_LEVEL_ERROR, "Revision revise unsupported.");
    return NULL;
}


celix_status_t bundleRevision_getNumber(const bundle_revision_t* revision, long *revisionNr) {
    *revisionNr = 1; //note revision nr is deprecated
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

celix_status_t bundleRevision_getHandles(const bundle_revision_t* revision CELIX_UNUSED, celix_array_list_t** handles) {
    //nop, usage deprecated
    if (handles) {
        *handles = celix_arrayList_create();
    }
    return CELIX_SUCCESS;
}
//LCOV_EXCL_STOP

