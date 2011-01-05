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
 * module.c
 *
 *  Created on: Jul 19, 2010
 *      Author: alexanderb
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "module.h"
#include "manifest_parser.h"
#include "linked_list_iterator.h"
#include "capability.h"
#include "wire.h"

struct module {
	LINKED_LIST capabilities;
	LINKED_LIST requirements;
	LINKED_LIST wires;

	VERSION version;
	char * symbolicName;
	bool resolved;

	MANIFEST headerMap;
	char * id;

	struct bundle * bundle;
};

MODULE module_create(MANIFEST headerMap, char * moduleId, BUNDLE bundle) {
	MODULE module = (MODULE) malloc(sizeof(*module));
	module->headerMap = headerMap;
	module->id = moduleId;
	module->bundle = bundle;

	MANIFEST_PARSER mp = manifestParser_createManifestParser(module, headerMap);
	module->symbolicName = mp->bundleSymbolicName;
	module->version = mp->bundleVersion;
	module->capabilities = mp->capabilities;
	module->requirements = mp->requirements;

	module->wires = NULL;

	return module;
}

MODULE module_createFrameworkModule() {
	MODULE module = (MODULE) malloc(sizeof(*module));
	module->id = "0";
	module->symbolicName = strdup("framework");
	module->version = version_createVersion(1, 0, 0, "");
	module->capabilities = linkedList_create();
	module->requirements = linkedList_create();
	return module;
}

WIRE module_getWire(MODULE module, char * serviceName) {
	if (module->wires != NULL) {
		LINKED_LIST_ITERATOR iterator = linkedListIterator_create(module->wires, 0);
		while (linkedListIterator_hasNext(iterator)) {
			WIRE wire = linkedListIterator_next(iterator);
			if (strcasecmp(capability_getServiceName(wire_getCapability(wire)), serviceName) == 0) {
				return wire;
			}
		}
	}
	return NULL;
}

VERSION module_getVersion(MODULE module) {
	return module->version;
}

char * module_getSymbolicName(MODULE module) {
	return module->symbolicName;
}

char * module_getId(MODULE module) {
	return module->id;
}

LINKED_LIST module_getWires(MODULE module) {
	return module->wires;
}

void module_setWires(MODULE module, LINKED_LIST wires) {
	module->wires = wires;
}

bool module_isResolved(MODULE module) {
	return module->resolved;
}

void module_setResolved(MODULE module) {
	module->resolved = true;
}

BUNDLE module_getBundle(MODULE module) {
	return module->bundle;
}

LINKED_LIST module_getRequirements(MODULE module) {
	return module->requirements;
}

LINKED_LIST module_getCapabilities(MODULE module) {
	return module->capabilities;
}
