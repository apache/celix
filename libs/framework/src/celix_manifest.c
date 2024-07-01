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

#include "celix_manifest.h"

#include "celix_err.h"
#include "celix_properties.h"
#include "celix_stdlib_cleanup.h"
#include "celix_utils.h"
#include "celix_version.h"

// Mandatory manifest attributes
#define CELIX_MANIFEST_VERSION "CELIX_MANIFEST_VERSION"
#define CELIX_BUNDLE_SYMBOLIC_NAME "CELIX_BUNDLE_SYMBOLIC_NAME"
#define CELIX_BUNDLE_VERSION "CELIX_BUNDLE_VERSION"
#define CELIX_BUNDLE_NAME "CELIX_BUNDLE_NAME"

// Optional manifest attributes
#define CELIX_BUNDLE_PRIVATE_LIBRARIES "CELIX_BUNDLE_PRIVATE_LIBRARIES"
#define CELIX_BUNDLE_ACTIVATOR_LIBRARY "CELIX_BUNDLE_ACTIVATOR_LIBRARY"
#define CELIX_BUNDLE_GROUP "CELIX_BUNDLE_GROUP"

struct celix_manifest{
    celix_properties_t* attributes;

    //Mandatory fields
    celix_version_t* manifestVersion;
    celix_version_t* bundleVersion;
    char* symbolicName;
    char* bundleName;

    //Optional fields
    char* bundleGroup;
    char* activatorLibrary;
    celix_array_list_t* privateLibraries;
};

/**
 * @brief Set and validate the provided manifest by checking if all mandatory attributes are present and of the correct
 * type and checking if the optional attributes, when present, are of the correct type.
 */
static celix_status_t celix_manifest_setAttributes(celix_manifest_t* manifest);

celix_status_t celix_manifest_create(celix_properties_t* attributes, celix_manifest_t** manifestOut) {
    if (!attributes) {
        return CELIX_ILLEGAL_ARGUMENT;
    }

    celix_autofree celix_manifest_t* manifest = calloc(1, sizeof(*manifest));
    if (!manifest) {
        celix_properties_destroy(attributes);
        celix_err_push("Failed to allocate memory for manifest");
        return ENOMEM;
    }
    manifest->attributes = attributes;

    CELIX_EPROP(celix_manifest_setAttributes(manifest));

    *manifestOut = celix_steal_ptr(manifest);
    return CELIX_SUCCESS;
}


celix_status_t celix_manifest_createFromFile(const char* filename, celix_manifest_t** manifestOut) {
    celix_properties_t* properties = NULL;
    celix_status_t status = celix_properties_load(filename, 0, &properties);
    if (status != CELIX_SUCCESS) {
        return status;
    }
    return celix_manifest_create(properties, manifestOut);
}

void celix_manifest_destroy(celix_manifest_t* manifest) {
    if (manifest) {
        celix_properties_destroy(manifest->attributes);

        free(manifest->symbolicName);
        free(manifest->bundleName);
        celix_version_destroy(manifest->manifestVersion);
        celix_version_destroy(manifest->bundleVersion);

        free(manifest->activatorLibrary);
        free(manifest->bundleGroup);
        celix_arrayList_destroy(manifest->privateLibraries);

        free(manifest);
    }
}

const celix_properties_t* celix_manifest_getAttributes(celix_manifest_t* manifest) {
    return manifest->attributes;
}

static celix_status_t celix_manifest_setMandatoryAttributes(celix_manifest_t* manifest) {
    const char* symbolicName = celix_properties_get(manifest->attributes, CELIX_BUNDLE_SYMBOLIC_NAME, NULL);
    const char* bundleName = celix_properties_get(manifest->attributes, CELIX_BUNDLE_NAME, NULL);

    celix_autoptr(celix_version_t) manifestVersion = NULL;
    celix_status_t getVersionStatus =
        celix_properties_getAsVersion(manifest->attributes, CELIX_MANIFEST_VERSION, NULL, &manifestVersion);
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
    }
    if (!manifestVersion) {
        celix_err_push(CELIX_MANIFEST_VERSION " is missing or not a version");
        status = CELIX_ILLEGAL_ARGUMENT;
    }
    if (!bundleVersion) {
        celix_err_push(CELIX_BUNDLE_VERSION " is missing or not a version");
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    if (manifestVersion && celix_version_compareToMajorMinor(manifestVersion, 2, 0) != 0) {
        celix_err_push(CELIX_MANIFEST_VERSION " is not 2.0.*");
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

static celix_status_t celix_manifest_setOptionalAttributes(celix_manifest_t* manifest) {
    celix_status_t status = CELIX_SUCCESS;

    const char* l = celix_properties_getAsString(manifest->attributes, CELIX_BUNDLE_ACTIVATOR_LIBRARY, NULL);
    celix_autofree char* activatorLib = NULL;
    if (l) {
        activatorLib = celix_utils_strdup(l);
        CELIX_ERR_RET_IF_NULL(activatorLib);
    }

    const char* g = celix_properties_getAsString(manifest->attributes, CELIX_BUNDLE_GROUP, NULL);
    celix_autofree char* bundleGroup = NULL;
    if (g) {
        bundleGroup = celix_utils_strdup(g);
        CELIX_ERR_RET_IF_NULL(bundleGroup);
    }

    celix_autoptr(celix_array_list_t) privateLibraries = NULL;
    celix_status_t getStatus = celix_properties_getAsStringArrayList(
        manifest->attributes, CELIX_BUNDLE_ACTIVATOR_LIBRARY, NULL, &privateLibraries);
    CELIX_ERR_RET_IF_ENOMEM(getStatus);
    if (celix_properties_hasKey(manifest->attributes, CELIX_BUNDLE_PRIVATE_LIBRARIES) && !privateLibraries) {
        celix_err_pushf(CELIX_BUNDLE_PRIVATE_LIBRARIES " is not a string array. Got: '%s'",
                        celix_properties_get(manifest->attributes, CELIX_BUNDLE_PRIVATE_LIBRARIES, NULL));
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    if (status == CELIX_SUCCESS) {
        manifest->activatorLibrary = celix_steal_ptr(activatorLib);
        manifest->bundleGroup = celix_steal_ptr(bundleGroup);
        manifest->privateLibraries = celix_steal_ptr(privateLibraries);
    }

    return status;
}

static celix_status_t celix_manifest_setAttributes(celix_manifest_t* manifest) {
    celix_status_t mStatus = celix_manifest_setMandatoryAttributes(manifest);
    celix_status_t oStatus = celix_manifest_setOptionalAttributes(manifest);
    return mStatus != CELIX_SUCCESS ? mStatus : oStatus;
}