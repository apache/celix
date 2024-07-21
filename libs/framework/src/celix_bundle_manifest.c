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
#include <ctype.h>
#include <string.h>

#include "celix_err.h"
#include "celix_properties.h"
#include "celix_stdlib_cleanup.h"
#include "celix_utils.h"
#include "celix_version.h"
#include "celix_framework_version.h"

// Mandatory manifest attributes
#define CELIX_BUNDLE_MANIFEST_VERSION "CELIX_BUNDLE_MANIFEST_VERSION"
#define CELIX_BUNDLE_SYMBOLIC_NAME "CELIX_BUNDLE_SYMBOLIC_NAME"
#define CELIX_BUNDLE_VERSION "CELIX_BUNDLE_VERSION"
#define CELIX_BUNDLE_NAME "CELIX_BUNDLE_NAME"

// Optional manifest attributes
#define CELIX_BUNDLE_ACTIVATOR_LIBRARY "CELIX_BUNDLE_ACTIVATOR_LIBRARY"
#define CELIX_BUNDLE_PRIVATE_LIBRARIES "CELIX_BUNDLE_PRIVATE_LIBRARIES"
#define CELIX_BUNDLE_DESCRIPTION "CELIX_BUNDLE_DESCRIPTION"
#define CELIX_BUNDLE_GROUP "CELIX_BUNDLE_GROUP"

#define CELIX_BUNDLE_SYMBOLIC_NAME_ALLOWED_SPECIAL_CHARS "-_:."
#define CELIX_FRAMEWORK_MANIFEST_VERSION "2.0.0"

struct celix_bundle_manifest {
    celix_properties_t* attributes;

    //Mandatory fields
    celix_version_t* manifestVersion;
    celix_version_t* bundleVersion;
    char* symbolicName;
    char* bundleName;

    //Optional fields
    char* bundleGroup;
    char* description;
    char* activatorLibrary;
    celix_array_list_t* privateLibraries;
};

/**
 * @brief Set and validate the provided manifest by checking if all mandatory attributes are present and of the correct
 * type and checking if the optional attributes, when present, are of the correct type.
 */
static celix_status_t celix_bundleManifest_setAttributes(celix_bundle_manifest_t* manifest);

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

    celix_status_t status = celix_bundleManifest_setAttributes(manifest);
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

    celix_status_t status =
        celix_properties_set(properties, CELIX_BUNDLE_MANIFEST_VERSION, CELIX_FRAMEWORK_MANIFEST_VERSION);
    status = CELIX_DO_IF(status, celix_properties_set(properties, CELIX_BUNDLE_SYMBOLIC_NAME, "apache_celix_framework"));
    status = CELIX_DO_IF(status, celix_properties_set(properties, CELIX_BUNDLE_NAME, "Apache Celix Framework"));
    status = CELIX_DO_IF(status, celix_properties_set(properties, CELIX_BUNDLE_VERSION, CELIX_FRAMEWORK_VERSION));
    status = CELIX_DO_IF(status, celix_properties_set(properties, CELIX_BUNDLE_GROUP, "Celix/Framework"));
    status = CELIX_DO_IF(
        status, celix_properties_set(properties, CELIX_BUNDLE_DESCRIPTION, "The Apache Celix Framework System Bundle"));

    if (status != CELIX_SUCCESS) {
        celix_err_push("Failed to set properties for framework manifest");
        return status;
    }

    return celix_bundleManifest_create(celix_steal_ptr(properties), manifest);
}

void celix_bundleManifest_destroy(celix_bundle_manifest_t* manifest) {
    if (manifest) {
        celix_properties_destroy(manifest->attributes);

        free(manifest->symbolicName);
        free(manifest->bundleName);
        celix_version_destroy(manifest->manifestVersion);
        celix_version_destroy(manifest->bundleVersion);

        free(manifest->activatorLibrary);
        free(manifest->bundleGroup);
        free(manifest->description);
        celix_arrayList_destroy(manifest->privateLibraries);

        free(manifest);
    }
}

const celix_properties_t* celix_bundleManifest_getAttributes(celix_bundle_manifest_t* manifest) {
    return manifest->attributes;
}

static celix_status_t celix_bundleManifest_setMandatoryAttributes(celix_bundle_manifest_t* manifest) {
    const char* symbolicName = celix_properties_get(manifest->attributes, CELIX_BUNDLE_SYMBOLIC_NAME, NULL);
    const char* bundleName = celix_properties_get(manifest->attributes, CELIX_BUNDLE_NAME, NULL);

    celix_autoptr(celix_version_t) manifestVersion = NULL;
    celix_status_t getVersionStatus =
        celix_properties_getAsVersion(manifest->attributes, CELIX_BUNDLE_MANIFEST_VERSION, NULL, &manifestVersion);
    CELIX_ERR_RET_IF_ENOMEM(getVersionStatus);

    celix_autoptr(celix_version_t) bundleVersion = NULL;
    getVersionStatus = celix_properties_getAsVersion(manifest->attributes, CELIX_BUNDLE_VERSION, NULL, &bundleVersion);
    CELIX_ERR_RET_IF_ENOMEM(getVersionStatus);

    celix_status_t status = CELIX_SUCCESS;
    if (!bundleName) {
        celix_err_push(CELIX_BUNDLE_NAME " is missing");
        status = CELIX_ILLEGAL_ARGUMENT;
    }
    if (!symbolicName) {
        celix_err_push(CELIX_BUNDLE_SYMBOLIC_NAME " is missing");
        status = CELIX_ILLEGAL_ARGUMENT;
    } else {
        // check if bundle symbolic name only contains the following characters: [a-zA-Z0-9_-:]
        for (size_t i = 0; symbolicName[i] != '\0'; ++i) {
            if (!isalnum(symbolicName[i]) &&
                strchr(CELIX_BUNDLE_SYMBOLIC_NAME_ALLOWED_SPECIAL_CHARS, symbolicName[i]) == NULL) {
                celix_err_pushf(
                    CELIX_BUNDLE_SYMBOLIC_NAME " '%s' contains invalid character '%c'", symbolicName, symbolicName[i]);
                status = CELIX_ILLEGAL_ARGUMENT;
                break;
            }
        }
    }
    if (!manifestVersion) {
        celix_err_push(CELIX_BUNDLE_MANIFEST_VERSION " is missing or not a version");
        status = CELIX_ILLEGAL_ARGUMENT;
    }
    if (!bundleVersion) {
        celix_err_push(CELIX_BUNDLE_VERSION " is missing or not a version");
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    if (manifestVersion && celix_version_compareToMajorMinor(manifestVersion, 2, 0) != 0) {
        celix_err_push(CELIX_BUNDLE_MANIFEST_VERSION " is not 2.0.*");
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    if (status == CELIX_SUCCESS) {
        manifest->symbolicName = celix_utils_strdup(symbolicName);
        CELIX_ERR_RET_IF_NULL(manifest->symbolicName);
        manifest->bundleName = celix_utils_strdup(bundleName);
        CELIX_ERR_RET_IF_NULL(manifest->bundleName);
        manifest->manifestVersion = celix_steal_ptr(manifestVersion);
        manifest->bundleVersion = celix_steal_ptr(bundleVersion);
    }

    return status;
}

static celix_status_t celix_bundleManifest_setOptionalAttributes(celix_bundle_manifest_t* manifest) {
    celix_status_t status = CELIX_SUCCESS;

    const char* lib = celix_properties_getAsString(manifest->attributes, CELIX_BUNDLE_ACTIVATOR_LIBRARY, NULL);
    celix_autofree char* activatorLib = NULL;
    if (lib) {
        activatorLib = celix_utils_strdup(lib);
        CELIX_ERR_RET_IF_NULL(activatorLib);
    }

    const char* group = celix_properties_getAsString(manifest->attributes, CELIX_BUNDLE_GROUP, NULL);
    celix_autofree char* bundleGroup = NULL;
    if (group) {
        bundleGroup = celix_utils_strdup(group);
        CELIX_ERR_RET_IF_NULL(bundleGroup);
    }

    const char* desc = celix_properties_getAsString(manifest->attributes, CELIX_BUNDLE_DESCRIPTION, NULL);
    celix_autofree char* description = NULL;
    if (desc) {
        description = celix_utils_strdup(desc);
        CELIX_ERR_RET_IF_NULL(description);
    }

    celix_autoptr(celix_array_list_t) privateLibraries = NULL;
    celix_status_t getStatus = celix_properties_getAsStringArrayList(
        manifest->attributes, CELIX_BUNDLE_PRIVATE_LIBRARIES, NULL, &privateLibraries);
    CELIX_ERR_RET_IF_ENOMEM(getStatus);
    if (celix_properties_hasKey(manifest->attributes, CELIX_BUNDLE_PRIVATE_LIBRARIES) && !privateLibraries) {
        celix_err_pushf(CELIX_BUNDLE_PRIVATE_LIBRARIES " is not a string array. Got: '%s'",
                        celix_properties_get(manifest->attributes, CELIX_BUNDLE_PRIVATE_LIBRARIES, NULL));
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    if (status == CELIX_SUCCESS) {
        manifest->activatorLibrary = celix_steal_ptr(activatorLib);
        manifest->bundleGroup = celix_steal_ptr(bundleGroup);
        manifest->description = celix_steal_ptr(description);
        manifest->privateLibraries = celix_steal_ptr(privateLibraries);
    }

    return status;
}

static celix_status_t celix_bundleManifest_setAttributes(celix_bundle_manifest_t* manifest) {
    celix_status_t mStatus = celix_bundleManifest_setMandatoryAttributes(manifest);
    celix_status_t oStatus = celix_bundleManifest_setOptionalAttributes(manifest);
    return mStatus != CELIX_SUCCESS ? mStatus : oStatus;
}

const char* celix_bundleManifest_getBundleName(celix_bundle_manifest_t* manifest) {
    return manifest->bundleName;
}

const char* celix_bundleManifest_getBundleSymbolicName(celix_bundle_manifest_t* manifest) {
    return manifest->symbolicName;
}

const celix_version_t* celix_bundleManifest_getBundleVersion(celix_bundle_manifest_t* manifest) {
    return manifest->bundleVersion;
}

const celix_version_t* celix_bundleManifest_getManifestVersion(celix_bundle_manifest_t* manifest) {
    return manifest->manifestVersion;
}

const char* celix_bundleManifest_getBundleActivatorLibrary(celix_bundle_manifest_t* manifest) {
    return manifest->activatorLibrary;
}

const celix_array_list_t* celix_bundleManifest_getBundlePrivateLibraries(celix_bundle_manifest_t* manifest) {
    return manifest->privateLibraries;
}

const char* celix_bundleManifest_getBundleDescription(celix_bundle_manifest_t* manifest) {
    return manifest->description;
}

const char* celix_bundleManifest_getBundleGroup(celix_bundle_manifest_t* manifest) {
    return manifest->bundleGroup;
}
