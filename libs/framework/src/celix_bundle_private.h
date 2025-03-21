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

#ifndef BUNDLE_PRIVATE_H_
#define BUNDLE_PRIVATE_H_

#include "celix_bundle.h"
#include "celix_module.h"

#ifdef __cplusplus
extern "C" {
#endif

struct celix_bundle {
    bundle_context_pt context;
    struct celix_bundle_activator *activator;
    bundle_state_e state;
    void *handle;
    celix_bundle_archive_t* archive;
    celix_array_list_t* modules;

    celix_framework_t *framework;
};

typedef struct celix_bundle_activator celix_bundle_activator_t;



/**
 * Creates a bundle from the given archive.
 * @param[in] framework The framework.
 * @param[in] archive The archive.
 * @param[out] bundleOut The created bundle.
 * @return CELIX_SUCCESS if the bundle is created successfully.
 */
celix_status_t
celix_bundle_createFromArchive(celix_framework_t *framework, celix_bundle_archive_t* archive, celix_bundle_t **bundleOut);

/**
 * @brief Get the bundle archive.
 *
 * @param[in] bundle The bundle.
 * @return The bundle archive.
 */
celix_bundle_archive_t *celix_bundle_getArchive(const celix_bundle_t *bundle);

/**
 * Destroys the bundle.
 * @param[in] bundle The bundle to destroy.
 * @return CELIX_SUCCESS if the bundle is destroyed successfully.
 */
celix_status_t bundle_destroy(celix_bundle_t *bundle);

/**
 * @brief Get the bundle context.
 */
celix_bundle_context_t* celix_bundle_getContext(const celix_bundle_t *bundle);

/**
 * @brief Set the bundle context.
 */
void celix_bundle_setContext(celix_bundle_t *bundle, celix_bundle_context_t *context);


CELIX_FRAMEWORK_DEPRECATED celix_status_t bundle_isSystemBundle(const celix_bundle_t *bundle, bool *systemBundle);

CELIX_FRAMEWORK_DEPRECATED celix_status_t bundle_getArchive(const celix_bundle_t *bundle, celix_bundle_archive_t** archive);

CELIX_FRAMEWORK_DEPRECATED celix_status_t bundle_getCurrentModule(const celix_bundle_t *bundle, celix_module_t** module);

CELIX_FRAMEWORK_DEPRECATED void *bundle_getHandle(celix_bundle_t *bundle);

CELIX_FRAMEWORK_DEPRECATED void bundle_setHandle(celix_bundle_t *bundle, void *handle);

CELIX_FRAMEWORK_DEPRECATED celix_bundle_activator_t *bundle_getActivator(const celix_bundle_t *bundle);

CELIX_FRAMEWORK_DEPRECATED celix_status_t bundle_setActivator(celix_bundle_t *bundle, celix_bundle_activator_t *activator);

CELIX_FRAMEWORK_DEPRECATED celix_status_t bundle_setState(celix_bundle_t *bundle, bundle_state_e state);

CELIX_FRAMEWORK_DEPRECATED celix_status_t bundle_addModule(celix_bundle_t *bundle, celix_module_t* celix_module);

CELIX_FRAMEWORK_DEPRECATED celix_status_t bundle_getState(const celix_bundle_t *bundle, bundle_state_e *state);

CELIX_FRAMEWORK_DEPRECATED celix_status_t bundle_getBundleId(const celix_bundle_t *bundle, long *id);

CELIX_FRAMEWORK_DEPRECATED celix_status_t bundle_getFramework(const celix_bundle_t *bundle, celix_framework_t **framework);

#ifdef __cplusplus
}
#endif

#endif /* BUNDLE_PRIVATE_H_ */
