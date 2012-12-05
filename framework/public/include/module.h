/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */
/*
 * module.h
 *
 *  \date       Jul 12, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef MODULE_H_
#define MODULE_H_

typedef struct module *module_t;

#include "celixbool.h"
#include "linkedlist.h"
#include "manifest.h"
#include "version.h"
#include "array_list.h"
#include "bundle.h"
#include "framework_exports.h"

module_t module_create(MANIFEST headerMap, char * moduleId, bundle_t bundle);
module_t module_createFrameworkModule(bundle_t bundle);
void module_destroy(module_t module);

FRAMEWORK_EXPORT unsigned int module_hash(void * module);
FRAMEWORK_EXPORT int module_equals(void * module, void * compare);

FRAMEWORK_EXPORT wire_t module_getWire(module_t module, char * serviceName);

FRAMEWORK_EXPORT version_t module_getVersion(module_t module);
FRAMEWORK_EXPORT celix_status_t module_getSymbolicName(module_t module, char **symbolicName);
FRAMEWORK_EXPORT char * module_getId(module_t module);
FRAMEWORK_EXPORT linked_list_t module_getWires(module_t module);
FRAMEWORK_EXPORT void module_setWires(module_t module, linked_list_t wires);
FRAMEWORK_EXPORT bool module_isResolved(module_t module);
FRAMEWORK_EXPORT void module_setResolved(module_t module);
FRAMEWORK_EXPORT bundle_t module_getBundle(module_t module);

FRAMEWORK_EXPORT linked_list_t module_getRequirements(module_t module);
FRAMEWORK_EXPORT linked_list_t module_getCapabilities(module_t module);

FRAMEWORK_EXPORT array_list_t module_getDependentImporters(module_t module);
FRAMEWORK_EXPORT void module_addDependentImporter(module_t module, module_t importer);
FRAMEWORK_EXPORT void module_removeDependentImporter(module_t module, module_t importer);

FRAMEWORK_EXPORT array_list_t module_getDependentRequirers(module_t module);
FRAMEWORK_EXPORT void module_addDependentRequirer(module_t module, module_t requirer);
FRAMEWORK_EXPORT void module_removeDependentRequirer(module_t module, module_t requirer);

FRAMEWORK_EXPORT array_list_t module_getDependents(module_t module);

#endif /* MODULE_H_ */
