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

#include "celix_errno.h"
#include "celix_properties_type.h"
#include "celix_cleanup.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file celix_manifest.h
 * @brief Header file for celix_manifest_t.
 *
 *
 * The manifest consists of a set of attributes, including a set of mandatory attributes.
 * A manifest is used to describe the contents of a bundle.
 *
 * A manifest must contain the following attributes:
 * - CELIX_MANIFEST_VERSION, type celix_version_t, the version of the manifest.
 * - CELIX_BUNDLE_SYMBOLIC_NAME, type string, the symbolic name of the bundle.
 * - CELIX_BUNDLE_VERSION, type celix_version_t, the version of the bundle.
 * - CELIX_BUNDLE_NAME, type string, the name of the bundle.
 *
 * The manifest may contain the following attributes:
 * - CELIX_BUNDLE_ACTIVATOR_LIBRARY, type string, the activator library of the bundle.
 * - CELIX_BUNDLE_PRIVATE_LIBRARIES, type string array, the private libraries of the bundle.
 * - CELIX_BUNDLE_GROUP, type string, the group of the bundle. Helps in grouping sets of bundles.
 *
 * And a manifest may contain any other attributes of any type, this can be retrieved using
 * celix_manifest_getAttributes.
 *
 * A bundle symbolic name can only contain the following characters: [a-zA-Z0-9_-:]
 */

/**
 * @brief The definition of the celix_manifest_t* abstract data type.
 */
typedef struct celix_manifest celix_manifest_t;

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
celix_status_t celix_manifest_create(celix_properties_t* attributes, celix_manifest_t** manifest);

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
celix_status_t celix_manifest_createFromFile(const char* filename, celix_manifest_t** manifest);

/**
 * @brief Destroy the provided manifest.
 */
void celix_manifest_destroy(celix_manifest_t* manifest);

/**
 * Define the cleanup function for a celix_manifest_t, so that it can be used with celix_autoptr.
 */
CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(celix_manifest_t, celix_manifest_destroy)

/**
 * @brief Get the manifest attributes.
 * The manifest attributes are valid as long as the manifest is valid.
 */
const celix_properties_t* celix_manifest_getAttributes(celix_manifest_t* manifest);

//TODO getters for mandatory and optional attributes

#ifdef __cplusplus
}
#endif

#endif /* CELIX_MANIFEST_H_ */
