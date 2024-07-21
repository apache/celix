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

#ifndef CELIX_MANIFEST_H_
#define CELIX_MANIFEST_H_

#include "celix_array_list.h"
#include "celix_cleanup.h"
#include "celix_errno.h"
#include "celix_properties_type.h"
#include "celix_version_type.h"
#include "celix_array_list_type.h"
#include "celix_bundle_manifest_type.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file celix_bundle_manifest.h
 * @brief Header file for celix_bundle_manifest_t.
 *
 *
 * The bundle manifest consists of a set of attributes, including a set of mandatory attributes.
 * A bundle manifest is used to describe the contents of a bundle.
 *
 * A bundle manifest must contain the following attributes:
 * - CELIX_BUNDLE_MANIFEST_VERSION, type celix_version_t, the version of the manifest.
 * - CELIX_BUNDLE_SYMBOLIC_NAME, type string, the symbolic name of the bundle.
 * - CELIX_BUNDLE_VERSION, type celix_version_t, the version of the bundle.
 * - CELIX_BUNDLE_NAME, type string, the name of the bundle.
 *
 * The bundle manifest may contain the following attributes:
 * - CELIX_BUNDLE_ACTIVATOR_LIBRARY, type string, the activator library of the bundle.
 * - CELIX_BUNDLE_PRIVATE_LIBRARIES, type string array, the private libraries of the bundle.
 * - CELIX_BUNDLE_GROUP, type string, the group of the bundle. Helps in grouping sets of bundles.
 *
 * And a manifest may contain any other attributes of any type, this can be retrieved using
 * celix_bundleManifest_getAttributes.
 *
 * A bundle symbolic name can only contain the following characters: [a-zA-Z0-9_-:]
 */

/**
 * Create a new manifest using the provided properties.
 *
 * If an error occurs, an error message is logged on celix_err and in case of an CELIX_ILLEGAL_ARGUMENT error, there
 * can be multiple error messages.
 *
 * @param[in] properties The properties to use for the manifest. Takes ownership of the properties.
 * @param[out] manifest The created manifest.
 * @return CELIX_SUCCESS if no errors occurred, ENOMEM if memory allocation failed and CELIX_ILLEGAL_ARGUMENT if the
 * provided attributes is incorrect. In case of an error, the provided attributes are destroyed.
 */
celix_status_t celix_bundleManifest_create(celix_properties_t* attributes, celix_bundle_manifest_t** manifest);

/**
 * Create a new manifest by reading the manifest from the provided file.
 *
 * If an error occurs, an error message is logged on celix_err.
 *
 * @param[in] filename The file to read the manifest from.
 * @param[out] manifest The created manifest.
 * @return CELIX_SUCCESS if no errors occurred, ENOMEM if memory allocation failed, CELIX_FILE_IO_EXCEPTION if the file
 * could not be read and CELIX_ILLEGAL_ARGUMENT if the manifest file is invalid.
 */
celix_status_t celix_bundleManifest_createFromFile(const char* filename, celix_bundle_manifest_t** manifest);

/**
 * @brief Create a new framework manifest.
 *
 * The framework manifest is a special manifest that contains the following manifest attributes:
 * - CELIX_BUNDLE_SYMBOLIC_NAME="celix.framework"
 * - CELIX_BUNDLE_NAME="Celix Framework"
 * - CELIX_BUNDLE_VERSION=CELIX_FRAMEWORK_VERSION
 * - CELIX_BUNDLE_MANIFEST_VERSION="2.0.0"
 * - CELIX_BUNDLE_GROUP="Celix"
 * - CELIX_BUNDLE_DESCRIPTION="Celix Framework"
 *
 * @param manifest The created framework manifest.
 * @return CELIX_SUCCESS if no errors occurred, ENOMEM if memory allocation failed. If an error occurs, an error message
 * is logged on celix_err.
 */
celix_status_t celix_bundleManifest_createFrameworkManifest(celix_bundle_manifest_t** manifest);

/**
 * @brief Destroy the provided manifest.
 */
void celix_bundleManifest_destroy(celix_bundle_manifest_t* manifest);

/**
 * Define the cleanup function for a celix_bundleManifest_t, so that it can be used with celix_autoptr.
 */
CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(celix_bundle_manifest_t, celix_bundleManifest_destroy)

/**
 * @brief Get the manifest attributes.
 * The manifest attributes are valid as long as the manifest is valid.
 *
 * @param[in] manifest The bundle manifest to get the manifest version from. Cannot be NULL.
 * @return The manifest attributes. Will never be NULL.
 */
const celix_properties_t* celix_bundleManifest_getAttributes(celix_bundle_manifest_t* manifest);

/**
 * @brief Get the manifest version. Returned value is valid as long as the manifest is valid.
 *
 * @param[in] manifest The bundle manifest to get the manifest version from. Cannot be NULL.
 * @return The bundle name. Will never be NULL.
 */
const char* celix_bundleManifest_getBundleName(celix_bundle_manifest_t* manifest);

/**
 * @brief Get the manifest version. Returned value is valid as long as the manifest is valid.
 *
 * @param[in] manifest The bundle manifest to get the manifest version from. Cannot be NULL.
 * @return The bundle symbolic name. Will never be NULL.
 */
const char* celix_bundleManifest_getBundleSymbolicName(celix_bundle_manifest_t* manifest);

/**
 * @brief Get the bundle version. Returned value is valid as long as the manifest is valid.
 *
 * @param[in] manifest The bundle manifest to get the manifest version from. Cannot be NULL.
 * @return The bundle version. Will never be NULL.
 */
const celix_version_t* celix_bundleManifest_getBundleVersion(celix_bundle_manifest_t* manifest);

/**
 * @brief Get the bundle version. Returned value is valid as long as the manifest is valid.
 *
 * @param[in] manifest The bundle manifest to get the manifest version from. Cannot be NULL.
 * @return The manifest version. Will never be NULL.
 */
const celix_version_t* celix_bundleManifest_getManifestVersion(celix_bundle_manifest_t* manifest);

/**
 * @brief Get the bundle activator library. Returned value is valid as long as the manifest is valid.
 *
 * @param[in] manifest The bundle manifest to get the manifest version from. Cannot be NULL.
 * @return The bundle activator library. Will be NULL if the manifest does not contain the attribute.
 */
const char* celix_bundleManifest_getBundleActivatorLibrary(celix_bundle_manifest_t* manifest);

/**
 * @brief Get the bundle private libraries. Returned value is valid as long as the manifest is valid.
 *
 * @param[in] manifest The bundle manifest to get the manifest version from. Cannot be NULL.
 * @return The bundle private libraries as a celix_array_list_t* with strings. Will be NULL if the manifest does not
 * contain the attribute.
 */
const celix_array_list_t* celix_bundleManifest_getBundlePrivateLibraries(celix_bundle_manifest_t* manifest);

/**
 * @brief Get the bundle description. Returned value is valid as long as the manifest is valid.
 *
 * @param[in] manifest The bundle manifest to get the manifest version from. Cannot be NULL.
 * @return The bundle description. Will be NULL if the manifest does not contain the attribute.
 */
const char* celix_bundleManifest_getBundleDescription(celix_bundle_manifest_t* manifest);

/**
 * @brief Get the bundle group. Returned value is valid as long as the manifest is valid.
 *
 * @param[in] manifest The bundle manifest to get the manifest version from. Cannot be NULL.
 * @return The bundle group. Will be NULL if the manifest does not contain the attribute.
 */
const char* celix_bundleManifest_getBundleGroup(celix_bundle_manifest_t* manifest);


#ifdef __cplusplus
}
#endif

#endif /* CELIX_MANIFEST_H_ */
