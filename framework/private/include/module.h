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
 *  Created on: Jul 12, 2010
 *      Author: alexanderb
 */

#ifndef MODULE_H_
#define MODULE_H_

#include <stdbool.h>

#include "linkedlist.h"
#include "headers.h"
#include "manifest.h"

MODULE module_create(MANIFEST headerMap, char * moduleId, BUNDLE bundle);
MODULE module_createFrameworkModule();

unsigned int module_hash(void * module);
int module_equals(void * module, void * compare);

WIRE module_getWire(MODULE module, char * serviceName);

VERSION module_getVersion(MODULE module);
char * module_getSymbolicName(MODULE module);
char * module_getId(MODULE module);
LINKED_LIST module_getWires(MODULE module);
void module_setWires(MODULE module, LINKED_LIST wires);
bool module_isResolved(MODULE module);
void module_setResolved(MODULE module);
BUNDLE module_getBundle(MODULE module);

LINKED_LIST module_getRequirements(MODULE module);
LINKED_LIST module_getCapabilities(MODULE module);

ARRAY_LIST module_getDependentImporters(MODULE module);
void module_addDependentImporter(MODULE module, MODULE importer);
void module_removeDependentImporter(MODULE module, MODULE importer);

ARRAY_LIST module_getDependentRequirers(MODULE module);
void module_addDependentRequirer(MODULE module, MODULE requirer);
void module_removeDependentRequirer(MODULE module, MODULE requirer);

ARRAY_LIST module_getDependents(MODULE module);

#endif /* MODULE_H_ */
