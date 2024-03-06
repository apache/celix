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
/**
 * module.h
 *
 *  \date       Jul 12, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef MODULE_H_
#define MODULE_H_

typedef struct module *module_pt;
typedef struct module celix_module_t;

#include <stdbool.h>

#include "manifest.h"
#include "bundle.h"
#include "celix_framework_export.h"

#ifdef __cplusplus
extern "C" {
#endif

CELIX_FRAMEWORK_DEPRECATED_EXPORT module_pt module_create(manifest_pt headerMap, const char *moduleId, celix_bundle_t *bundle);

CELIX_FRAMEWORK_DEPRECATED_EXPORT module_pt module_createFrameworkModule(celix_framework_t* fw, celix_bundle_t *bundle);

CELIX_FRAMEWORK_DEPRECATED_EXPORT void module_destroy(module_pt module);

CELIX_FRAMEWORK_DEPRECATED_EXPORT unsigned int module_hash(void *module);

CELIX_FRAMEWORK_DEPRECATED_EXPORT int module_equals(void *module, void *compare);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_version_t* module_getVersion(module_pt module);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t module_getSymbolicName(module_pt module, const char **symbolicName);

CELIX_FRAMEWORK_DEPRECATED_EXPORT char *module_getId(module_pt module);

CELIX_FRAMEWORK_DEPRECATED_EXPORT bool module_isResolved(module_pt module);

CELIX_FRAMEWORK_DEPRECATED_EXPORT void module_setResolved(module_pt module);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_bundle_t *module_getBundle(module_pt module);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_array_list_t *module_getDependentImporters(module_pt module);

CELIX_FRAMEWORK_DEPRECATED_EXPORT void module_addDependentImporter(module_pt module, module_pt importer);

CELIX_FRAMEWORK_DEPRECATED_EXPORT void module_removeDependentImporter(module_pt module, module_pt importer);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_array_list_t *module_getDependentRequirers(module_pt module);

CELIX_FRAMEWORK_DEPRECATED_EXPORT void module_addDependentRequirer(module_pt module, module_pt requirer);

CELIX_FRAMEWORK_DEPRECATED_EXPORT void module_removeDependentRequirer(module_pt module, module_pt requirer);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_array_list_t *module_getDependents(module_pt module);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t module_getGroup(module_pt module, const char **group);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t module_getName(module_pt module, const char **name);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t module_getDescription(module_pt module, const char **description);

#ifdef __cplusplus
}
#endif

#endif /* MODULE_H_ */
