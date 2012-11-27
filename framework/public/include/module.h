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

typedef struct module *MODULE;

#include "celixbool.h"
#include "linkedlist.h"
#include "manifest.h"
#include "version.h"
#include "array_list.h"
#include "bundle.h"
#include "framework_exports.h"

MODULE module_create(MANIFEST headerMap, char * moduleId, BUNDLE bundle);
MODULE module_createFrameworkModule(BUNDLE bundle);
void module_destroy(MODULE module);

FRAMEWORK_EXPORT unsigned int module_hash(void * module);
FRAMEWORK_EXPORT int module_equals(void * module, void * compare);

FRAMEWORK_EXPORT WIRE module_getWire(MODULE module, char * serviceName);

FRAMEWORK_EXPORT VERSION module_getVersion(MODULE module);
FRAMEWORK_EXPORT celix_status_t module_getSymbolicName(MODULE module, char **symbolicName);
FRAMEWORK_EXPORT char * module_getId(MODULE module);
FRAMEWORK_EXPORT LINKED_LIST module_getWires(MODULE module);
FRAMEWORK_EXPORT void module_setWires(MODULE module, LINKED_LIST wires);
FRAMEWORK_EXPORT bool module_isResolved(MODULE module);
FRAMEWORK_EXPORT void module_setResolved(MODULE module);
FRAMEWORK_EXPORT BUNDLE module_getBundle(MODULE module);

FRAMEWORK_EXPORT LINKED_LIST module_getRequirements(MODULE module);
FRAMEWORK_EXPORT LINKED_LIST module_getCapabilities(MODULE module);

FRAMEWORK_EXPORT ARRAY_LIST module_getDependentImporters(MODULE module);
FRAMEWORK_EXPORT void module_addDependentImporter(MODULE module, MODULE importer);
FRAMEWORK_EXPORT void module_removeDependentImporter(MODULE module, MODULE importer);

FRAMEWORK_EXPORT ARRAY_LIST module_getDependentRequirers(MODULE module);
FRAMEWORK_EXPORT void module_addDependentRequirer(MODULE module, MODULE requirer);
FRAMEWORK_EXPORT void module_removeDependentRequirer(MODULE module, MODULE requirer);

FRAMEWORK_EXPORT ARRAY_LIST module_getDependents(MODULE module);

#endif /* MODULE_H_ */
