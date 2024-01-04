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

#include "bundle.h"
#include "celix_bundle.h"

#ifdef __cplusplus
extern "C" {
#endif

struct celix_bundle {
    bundle_context_pt context;
    char *symbolicName;
    char *name;
    char *group;
    char *description;
    struct celix_bundle_activator *activator;
    bundle_state_e state;
    void *handle;
    bundle_archive_pt archive;
    celix_array_list_t* modules;

    celix_framework_t *framework;
};

/**
 * Creates a bundle from the given archive.
 * @param[in] framework The framework.
 * @param[in] archive The archive.
 * @param[out] bundleOut The created bundle.
 * @return CELIX_SUCCESS if the bundle is created successfully.
 */
celix_status_t
celix_bundle_createFromArchive(celix_framework_t *framework, bundle_archive_pt archive, celix_bundle_t **bundleOut);

/**
 * @brief Get the bundle archive.
 *
 * @param[in] bundle The bundle.
 * @return The bundle archive.
 */
bundle_archive_t *celix_bundle_getArchive(const celix_bundle_t *bundle);

/**
 * Destroys the bundle.
 * @param[in] bundle The bundle to destroy.
 * @return CELIX_SUCCESS if the bundle is destroyed successfully.
 */
celix_status_t bundle_destroy(celix_bundle_t *bundle);

#ifdef __cplusplus
}
#endif

#endif /* BUNDLE_PRIVATE_H_ */
