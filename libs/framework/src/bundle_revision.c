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

struct celix_bundle_revision {
    celix_framework_t* fw;
    char* root;
    char* location;
    celix_bundle_manifest_t* manifest;
};

celix_status_t celix_bundleRevision_create(celix_framework_t* fw, const char *root, const char *location, celix_bundle_manifest_t* manifest,
                                           celix_bundle_revision_t** bundle_revision) {
    celix_status_t status = CELIX_SUCCESS;
    celix_bundle_revision_t* revision = calloc(1, sizeof(*revision));
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
            celix_bundleRevision_destroy(revision);
        } else {
            celix_bundleManifest_destroy(manifest);
        }
        return status;
    }

    *bundle_revision = revision;
    return status;
}

celix_status_t celix_bundleRevision_destroy(celix_bundle_revision_t* revision) {
    if (revision != NULL) {
        celix_bundleManifest_destroy(revision->manifest);
        free(revision->root);
        free(revision->location);
        free(revision);
    }
    return CELIX_SUCCESS;
}

celix_bundle_manifest_t* celix_bundleRevision_getManifest(celix_bundle_revision_t* revision) {
    return revision->manifest;
}
