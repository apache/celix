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

#include "celix_bundle_manifest.h"

#include "celix_cleanup.h"
#include "celix_errno.h"
#include "celix_array_list_type.h"
#include "celix_err.h"
#include "celix_properties.h"
#include "celix_properties_type.h"
#include "celix_version.h"
#include "celix_framework_version.h"
#include "celix_version_type.h"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#define CELIX_BUNDLE_SYMBOLIC_NAME_ALLOWED_SPECIAL_CHARS "-_:."
#define CELIX_FRAMEWORK_MANIFEST_VERSION "2.0.0"

struct celix_bundle_manifest {
    celix_properties_t* attributes;
};

/**
 * @brief Set and validate the provided manifest by checking if all mandatory attributes are present and of the correct
 * type and checking if the optional attributes, when present, are of the correct type.
 */
static celix_status_t celix_bundleManifest_checkAttributes(celix_bundle_manifest_t* manifest);

celix_status_t celix_bundleManifest_create(celix_properties_t* attributes, celix_bundle_manifest_t** manifestOut) {
    if (!attributes) {
        return CELIX_ILLEGAL_ARGUMENT;
    }

    celix_autoptr(celix_bundle_manifest_t) manifest = calloc(1, sizeof(*manifest));
    if (!manifest) {
        celix_properties_destroy(attributes);
        celix_err_push("Failed to allocate memory for manifest");
        return ENOMEM;
    }
    manifest->attributes = attributes;

    celix_status_t status = celix_bundleManifest_checkAttributes(manifest);
    if (status != CELIX_SUCCESS) {
        return status;
    }

    *manifestOut = celix_steal_ptr(manifest);
    return CELIX_SUCCESS;
}

celix_status_t celix_bundleManifest_createFromFile(const char* filename, celix_bundle_manifest_t** manifestOut) {
    celix_properties_t* properties = NULL;
    celix_status_t status = celix_properties_load(filename, 0, &properties);
    if (status != CELIX_SUCCESS) {
        return status;
    }
    return celix_bundleManifest_create(properties, manifestOut);
}

celix_status_t celix_bundleManifest_createFrameworkManifest(celix_bundle_manifest_t** manifest) {
    celix_autoptr(celix_properties_t) properties = celix_properties_create();
    if (!properties) {
        celix_err_push("Failed to create properties for framework manifest");
        return ENOMEM;
    }

    celix_version_t* fwVersion;
    celix_version_t* manifestVersion;
    celix_status_t status = celix_version_parse(CELIX_FRAMEWORK_MANIFEST_VERSION, &manifestVersion);
    status = CELIX_DO_IF(status,
                         celix_properties_assignVersion(properties, CELIX_BUNDLE_MANIFEST_VERSION, manifestVersion));
    status = CELIX_DO_IF(status,
                         celix_properties_set(properties, CELIX_BUNDLE_SYMBOLIC_NAME, "apache_celix_framework"));
    status = CELIX_DO_IF(status, celix_properties_set(properties, CELIX_BUNDLE_NAME, "Apache Celix Framework"));
    status = CELIX_DO_IF(status, celix_version_parse(CELIX_FRAMEWORK_VERSION, &fwVersion));
    status = CELIX_DO_IF(status, celix_properties_assignVersion(properties, CELIX_BUNDLE_VERSION, fwVersion));
    status = CELIX_DO_IF(status, celix_properties_set(properties, CELIX_BUNDLE_GROUP, "Celix/Framework"));
    status = CELIX_DO_IF(
        status,
        celix_properties_set(properties, CELIX_BUNDLE_DESCRIPTION, "The Apache Celix Framework System Bundle"));

    if (status != CELIX_SUCCESS) {
        celix_err_push("Failed to set properties for framework manifest");
        return status;
    }

    return celix_bundleManifest_create(celix_steal_ptr(properties), manifest);
}

void celix_bundleManifest_destroy(celix_bundle_manifest_t* manifest) {
    if (manifest) {
        celix_properties_destroy(manifest->attributes);
        free(manifest);
    }
}

const celix_properties_t* celix_bundleManifest_getAttributes(const celix_bundle_manifest_t* manifest) {
    return manifest->attributes;
}

static celix_status_t celix_bundleManifest_checkMandatoryAttributes(celix_bundle_manifest_t* manifest) {
    const char* symbolicName = celix_properties_get(manifest->attributes, CELIX_BUNDLE_SYMBOLIC_NAME, NULL);
    const char* bundleName = celix_properties_get(manifest->attributes, CELIX_BUNDLE_NAME, NULL);
    const celix_version_t* manifestVersion = celix_properties_getVersion(
        manifest->attributes,
        CELIX_BUNDLE_MANIFEST_VERSION);
    const celix_version_t* bundleVersion = celix_properties_getVersion(manifest->attributes, CELIX_BUNDLE_VERSION);

    celix_status_t status = CELIX_SUCCESS;
    if (!bundleName) {
        celix_err_push(CELIX_BUNDLE_NAME " is missing");
        status = CELIX_INVALID_SYNTAX;
    }
    if (!symbolicName) {
        celix_err_push(CELIX_BUNDLE_SYMBOLIC_NAME " is missing");
        status = CELIX_INVALID_SYNTAX;
    } else {
        // check if bundle symbolic name only contains the following characters: [a-zA-Z0-9_-:]
        for (size_t i = 0; symbolicName[i] != '\0'; ++i) {
            if (!isalnum(symbolicName[i]) &&
                strchr(CELIX_BUNDLE_SYMBOLIC_NAME_ALLOWED_SPECIAL_CHARS, symbolicName[i]) == NULL) {
                celix_err_pushf(
                    CELIX_BUNDLE_SYMBOLIC_NAME " '%s' contains invalid character '%c'", symbolicName, symbolicName[i]);
                status = CELIX_INVALID_SYNTAX;
                break;
            }
        }
    }
    if (!manifestVersion) {
        celix_err_push(CELIX_BUNDLE_MANIFEST_VERSION " is missing or not a version");
        status = CELIX_INVALID_SYNTAX;
    }
    if (!bundleVersion) {
        celix_err_push(CELIX_BUNDLE_VERSION " is missing or not a version");
        status = CELIX_INVALID_SYNTAX;
    }

    if (manifestVersion && celix_version_compareToMajorMinor(manifestVersion, 2, 0) != 0) {
        celix_err_push(CELIX_BUNDLE_MANIFEST_VERSION " is not 2.0.*");
        status = CELIX_INVALID_SYNTAX;
    }

    return status;
}

static celix_status_t celix_bundleManifest_checkOptionalAttributes(celix_bundle_manifest_t* manifest) {
    if (celix_properties_hasKey(manifest->attributes, CELIX_BUNDLE_PRIVATE_LIBRARIES)) {
        //if a private libraries manifest entry exist, this should be a string array list.
        const celix_array_list_t* libs = celix_properties_getStringArrayList(
            manifest->attributes,
            CELIX_BUNDLE_PRIVATE_LIBRARIES);
        if (!libs) {
            celix_err_push(CELIX_BUNDLE_PRIVATE_LIBRARIES " exists, but is not a array of strings");
            return CELIX_INVALID_SYNTAX;
        }
    }

    return CELIX_SUCCESS;
}

static celix_status_t celix_bundleManifest_checkAttributes(celix_bundle_manifest_t* manifest) {
    const celix_status_t mStatus = celix_bundleManifest_checkMandatoryAttributes(manifest);
    const celix_status_t oStatus = celix_bundleManifest_checkOptionalAttributes(manifest);
    return mStatus != CELIX_SUCCESS ? mStatus : oStatus;
}

const char* celix_bundleManifest_getBundleName(const celix_bundle_manifest_t* manifest) {
    return celix_properties_getString(manifest->attributes, CELIX_BUNDLE_NAME);
}

const char* celix_bundleManifest_getBundleSymbolicName(const celix_bundle_manifest_t* manifest) {
    return celix_properties_getString(manifest->attributes, CELIX_BUNDLE_SYMBOLIC_NAME);
}

const celix_version_t* celix_bundleManifest_getBundleVersion(const celix_bundle_manifest_t* manifest) {
    return celix_properties_getVersion(manifest->attributes, CELIX_BUNDLE_VERSION);
}

const celix_version_t* celix_bundleManifest_getManifestVersion(const celix_bundle_manifest_t* manifest) {
    return celix_properties_getVersion(manifest->attributes, CELIX_BUNDLE_MANIFEST_VERSION);
}

const char* celix_bundleManifest_getBundleActivatorLibrary(const celix_bundle_manifest_t* manifest) {
    return celix_properties_getString(manifest->attributes, CELIX_BUNDLE_ACTIVATOR_LIBRARY);
}

const celix_array_list_t* celix_bundleManifest_getBundlePrivateLibraries(const celix_bundle_manifest_t* manifest) {
    return celix_properties_getStringArrayList(manifest->attributes, CELIX_BUNDLE_PRIVATE_LIBRARIES);
}

const char* celix_bundleManifest_getBundleDescription(const celix_bundle_manifest_t* manifest) {
    return celix_properties_getString(manifest->attributes, CELIX_BUNDLE_DESCRIPTION);
}

const char* celix_bundleManifest_getBundleGroup(const celix_bundle_manifest_t* manifest) {
    return celix_properties_getString(manifest->attributes, CELIX_BUNDLE_GROUP);
}
