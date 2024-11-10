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

#ifndef CELIX_MODULE_H_
#define CELIX_MODULE_H_

typedef struct celix_module celix_module_t;

#include <stdbool.h>

#include "celix_bundle_manifest.h"
#include "celix_framework_export.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create a new module based on the provided manifest.
 * @return A new module or NULL if the module could not be created. If NULL is returned, an error log is written to
 * celix err.
 */
celix_module_t* module_create(celix_bundle_t* bundle);

void module_destroy(celix_module_t* module);

/**
 * Define the cleanup function for a celix_bundleManifest_t, so that it can be used with celix_autoptr.
 */
CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(celix_module_t, module_destroy)

const celix_version_t* module_getVersion(celix_module_t* module);

celix_status_t module_getSymbolicName(celix_module_t* module, const char **symbolicName);

bool module_isResolved(celix_module_t* module);

void module_setResolved(celix_module_t* module);

celix_bundle_t *module_getBundle(celix_module_t* module);

celix_status_t module_getGroup(celix_module_t* module, const char **group);

celix_status_t module_getName(celix_module_t* module, const char **name);

celix_status_t module_getDescription(celix_module_t* module, const char **description);

#ifdef __cplusplus
}
#endif

#endif /* CELIX_MODULE_H_ */
