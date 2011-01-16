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
 * resolver.c
 *
 *  Created on: Jul 13, 2010
 *      Author: alexanderb
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "resolver.h"
#include "linked_list_iterator.h"

struct capabilityList {
	char * serviceName;
	LINKED_LIST capabilities;
};

typedef struct capabilityList * CAPABILITY_LIST;

struct candidateSet {
	MODULE module;
	REQUIREMENT requirement;
	LINKED_LIST candidates;
};

typedef struct candidateSet * CANDIDATE_SET;

// List containing MODULEs
LINKED_LIST m_modules = NULL;
// List containing CAPABILITY_LISTs
LINKED_LIST m_unresolvedServices = NULL;
// List containing CAPABILITY_LISTs
LINKED_LIST m_resolvedServices = NULL;

int resolver_populateCandidatesMap(HASH_MAP candidatesMap, MODULE targetModule);
CAPABILITY_LIST resolver_getCapabilityList(LINKED_LIST list, char * name);
void resolver_removeInvalidCandidate(MODULE module, HASH_MAP candidates, LINKED_LIST invalid);
HASH_MAP resolver_populateWireMap(HASH_MAP candidates, MODULE importer, HASH_MAP wireMap);

HASH_MAP resolver_resolve(MODULE root) {
	if (module_isResolved(root)) {
		return NULL;
	}

	HASH_MAP candidatesMap = hashMap_create(NULL, NULL, NULL, NULL);

	if (resolver_populateCandidatesMap(candidatesMap, root) != 0) {
		return NULL;
	}

	HASH_MAP wireMap = hashMap_create(NULL,NULL,NULL,NULL);
	return resolver_populateWireMap(candidatesMap, root, wireMap);
}

int resolver_populateCandidatesMap(HASH_MAP candidatesMap, MODULE targetModule) {
	if (hashMap_containsKey(candidatesMap, targetModule)) {
		return 0;
	}

	hashMap_put(candidatesMap, targetModule, NULL);

	LINKED_LIST candSetList = linkedList_create();

	int i = 0;
	for (i = 0; i < linkedList_size(module_getRequirements(targetModule)); i++) {
		REQUIREMENT req = (REQUIREMENT) linkedList_get(module_getRequirements(targetModule), i);
		CAPABILITY_LIST capList = resolver_getCapabilityList(m_resolvedServices, requirement_getTargetName(req));

		LINKED_LIST candidates = linkedList_create();

		int c;
		for (c = 0; (capList != NULL) && (c < linkedList_size(capList->capabilities)); c++) {
			CAPABILITY cap = (CAPABILITY) linkedList_get(capList->capabilities, c);
			if (requirement_isSatisfied(req, cap)) {
				linkedList_addElement(candidates, cap);
			}
		}
		capList = resolver_getCapabilityList(m_unresolvedServices, requirement_getTargetName(req));
		for (c = 0; (capList != NULL) && (c < linkedList_size(capList->capabilities)); c++) {
			CAPABILITY cap = (CAPABILITY) linkedList_get(capList->capabilities, c);
			if (requirement_isSatisfied(req, cap)) {
				linkedList_addElement(candidates, cap);
			}
		}

		if (linkedList_size(candidates) > 0) {
			LINKED_LIST_ITERATOR iterator;
			for (iterator = linkedListIterator_create(candidates, 0); linkedListIterator_hasNext(iterator); ) {
				CAPABILITY candidate = (CAPABILITY) linkedListIterator_next(iterator);
				if (!module_isResolved(capability_getModule(candidate))) {
					if (resolver_populateCandidatesMap(candidatesMap, capability_getModule(candidate)) != 0) {
						linkedListIterator_remove(iterator);
					}
				}
			}
		}

		if (linkedList_size(candidates) == 0) {
			LINKED_LIST invalid = linkedList_create();
			resolver_removeInvalidCandidate(targetModule, candidatesMap, invalid);
			printf("Unable to resolve: %s, %s\n", module_getSymbolicName(targetModule), requirement_getTargetName(req));
			return -1;
		} else if (linkedList_size(candidates) > 0) {
			CANDIDATE_SET cs = (CANDIDATE_SET) malloc(sizeof(*cs));
			cs->candidates = candidates;
			cs->module = targetModule;
			cs->requirement = req;
			linkedList_addElement(candSetList, cs);
		}

	}
	hashMap_put(candidatesMap, targetModule, candSetList);
	return 0;
}

void resolver_removeInvalidCandidate(MODULE invalidModule, HASH_MAP candidates, LINKED_LIST invalid) {
	hashMap_remove(candidates, invalidModule);

	HASH_MAP_ITERATOR iterator;
	for (iterator = hashMapIterator_create(candidates); hashMapIterator_hasNext(iterator); ) {
		HASH_MAP_ENTRY entry = hashMapIterator_nextEntry(iterator);
		MODULE module = (MODULE) hashMapEntry_getKey(entry);
		LINKED_LIST candSetList = (LINKED_LIST) hashMapEntry_getValue(entry);
		if (candSetList != NULL) {
			LINKED_LIST_ITERATOR itCandSetList;
			for (itCandSetList = linkedListIterator_create(candSetList, 0); linkedListIterator_hasNext(itCandSetList); ) {
				CANDIDATE_SET set = (CANDIDATE_SET) linkedListIterator_next(itCandSetList);
				LINKED_LIST_ITERATOR candIter;
				for (candIter = linkedListIterator_create(set->candidates, 0); linkedListIterator_hasNext(candIter); ) {
					CAPABILITY candCap = (CAPABILITY) linkedListIterator_next(candIter);
					if (capability_getModule(candCap) == invalidModule) {
						linkedListIterator_remove(candIter);
						if (linkedList_size(set->candidates) == 0) {
							linkedListIterator_remove(itCandSetList);
							if (module != invalidModule && linkedList_contains(invalid, module)) {
								linkedList_addElement(invalid, module);
							}
						}
						break;
					}
				}
			}
		}
	}
	free(iterator);

	if (linkedList_size(invalid) > 0) {
		while (!linkedList_isEmpty(invalid)) {
			MODULE m = (MODULE) linkedList_removeIndex(invalid, 0);
			resolver_removeInvalidCandidate(m, candidates, invalid);
		}
	}
}

void resolver_addModule(MODULE module) {
	if (m_modules == NULL) {
		m_modules = linkedList_create();
		m_unresolvedServices = linkedList_create();
		m_resolvedServices = linkedList_create();
	}
	linkedList_addElement(m_modules, module);

	int i = 0;
	for (i = 0; i < linkedList_size(module_getCapabilities(module)); i++) {
		CAPABILITY cap = (CAPABILITY) linkedList_get(module_getCapabilities(module), i);
		CAPABILITY_LIST list = resolver_getCapabilityList(m_unresolvedServices, capability_getServiceName(cap));
		if (list == NULL) {
			list = (CAPABILITY_LIST) malloc(sizeof(*list));
			list->serviceName = strdup(capability_getServiceName(cap));
			list->capabilities = linkedList_create();

			linkedList_addElement(m_unresolvedServices, list);
		}
		linkedList_addElement(list->capabilities, cap);
	}
}

void resolver_moduleResolved(MODULE module) {
	if (module_isResolved(module)) {
		int capIdx;
		LINKED_LIST capsCopy = linkedList_create();
		for (capIdx = 0; (module_getCapabilities(module) != NULL) && (capIdx < linkedList_size(module_getCapabilities(module))); capIdx++) {
			CAPABILITY cap = (CAPABILITY) linkedList_get(module_getCapabilities(module), capIdx);
			CAPABILITY_LIST list = resolver_getCapabilityList(m_unresolvedServices, capability_getServiceName(cap));
			linkedList_removeElement(list->capabilities, cap);

			linkedList_addElement(capsCopy, cap);
		}

		LINKED_LIST wires = module_getWires(module);
		for (capIdx = 0; (capsCopy != NULL) && (capIdx < linkedList_size(capsCopy)); capIdx++) {
			CAPABILITY cap = linkedList_get(capsCopy, capIdx);

			int wireIdx = 0;
			for (wireIdx = 0; (wires != NULL) && (wireIdx < linkedList_size(wires)); wireIdx++) {
				WIRE wire = (WIRE) linkedList_get(wires, wireIdx);

				if (requirement_isSatisfied(wire_getRequirement(wire), cap)) {
					linkedList_set(capsCopy, capIdx, NULL);
					break;
				}
			}
		}

		for (capIdx = 0; (capsCopy != NULL) && (capIdx < linkedList_size(capsCopy)); capIdx++) {
			CAPABILITY cap = linkedList_get(capsCopy, capIdx);

			if (cap != NULL) {
				CAPABILITY_LIST list = resolver_getCapabilityList(m_resolvedServices, capability_getServiceName(cap));
				if (list == NULL) {
					list = (CAPABILITY_LIST) malloc(sizeof(*list));
					list->serviceName = strdup(capability_getServiceName(cap));
					list->capabilities = linkedList_create();

					linkedList_addElement(m_resolvedServices, list);
				}
				linkedList_addElement(list->capabilities, cap);
			}
		}
	}
}

CAPABILITY_LIST resolver_getCapabilityList(LINKED_LIST list, char * name) {
	LINKED_LIST_ITERATOR iterator = linkedListIterator_create(list, 0);
	while (linkedListIterator_hasNext(iterator)) {
		CAPABILITY_LIST services = (CAPABILITY_LIST) linkedListIterator_next(iterator);
		if (strcmp(services->serviceName, name) == 0) {
			return services;
		}
	}
	return NULL;
}

HASH_MAP resolver_populateWireMap(HASH_MAP candidates, MODULE importer, HASH_MAP wireMap) {
	if (module_isResolved(importer) || (hashMap_get(wireMap, importer))) {
		return wireMap;
	}

	LINKED_LIST candSetList = (LINKED_LIST) hashMap_get(candidates, importer);

	LINKED_LIST serviceWires = linkedList_create();
	LINKED_LIST emptyWires = linkedList_create();

	hashMap_put(wireMap, importer, emptyWires);

	int candSetIdx = 0;
	for (candSetIdx = 0; candSetIdx < linkedList_size(candSetList); candSetIdx++) {
		CANDIDATE_SET cs = (CANDIDATE_SET) linkedList_get(candSetList, candSetIdx);


		if (importer != capability_getModule(((CAPABILITY) linkedList_get(cs->candidates, 0)))) {
			WIRE wire = wire_create(importer, cs->requirement,
					capability_getModule(((CAPABILITY) linkedList_get(cs->candidates, 0))),
					((CAPABILITY) linkedList_get(cs->candidates, 0)));
			linkedList_addElement(serviceWires, wire);
		}

		wireMap = resolver_populateWireMap(candidates,
				capability_getModule(((CAPABILITY) linkedList_get(cs->candidates, 0))),
				wireMap);
	}

	//hashMap_remove(wireMap, importer);
	hashMap_put(wireMap, importer, serviceWires);

	return wireMap;
}
