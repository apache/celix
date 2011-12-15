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
#include <apr_strings.h>

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
	    printf("already resolved\n");
		return NULL;
	}

	HASH_MAP candidatesMap = hashMap_create(NULL, NULL, NULL, NULL);

	if (resolver_populateCandidatesMap(candidatesMap, root) != 0) {
		HASH_MAP_ITERATOR iter = hashMapIterator_create(candidatesMap);
		while (hashMapIterator_hasNext(iter)) {
			HASH_MAP_ENTRY entry = hashMapIterator_nextEntry(iter);
			MODULE key = hashMapEntry_getKey(entry);
			LINKED_LIST value = hashMapEntry_getValue(entry);
			hashMapIterator_remove(iter);
			if (value != NULL) {
				LINKED_LIST_ITERATOR candSetIter = linkedListIterator_create(value, 0);
				while (linkedListIterator_hasNext(candSetIter)) {
					CANDIDATE_SET set = linkedListIterator_next(candSetIter);
					free(set);
					linkedListIterator_remove(candSetIter);
				}
				linkedListIterator_destroy(candSetIter);
			}
		}
		hashMapIterator_destroy(iter);
		hashMap_destroy(candidatesMap, false, false);
		return NULL;
	}

	HASH_MAP wireMap = hashMap_create(NULL,NULL,NULL,NULL);
	HASH_MAP resolved = resolver_populateWireMap(candidatesMap, root, wireMap);
	HASH_MAP_ITERATOR iter = hashMapIterator_create(candidatesMap);
	while (hashMapIterator_hasNext(iter)) {
		HASH_MAP_ENTRY entry = hashMapIterator_nextEntry(iter);
		MODULE key = hashMapEntry_getKey(entry);
		LINKED_LIST value = hashMapEntry_getValue(entry);
		hashMapIterator_remove(iter);
		if (value != NULL) {
			LINKED_LIST_ITERATOR candSetIter = linkedListIterator_create(value, 0);
			while (linkedListIterator_hasNext(candSetIter)) {
				CANDIDATE_SET set = linkedListIterator_next(candSetIter);
				free(set);
			}
			linkedListIterator_destroy(candSetIter);
		}
	}
	hashMapIterator_destroy(iter);
	hashMap_destroy(candidatesMap, false, false);
	return resolved;
}

int resolver_populateCandidatesMap(HASH_MAP candidatesMap, MODULE targetModule) {
    LINKED_LIST candSetList;
	apr_pool_t *candSetList_pool;
    LINKED_LIST candidates;
    apr_pool_t *candidates_pool;
    LINKED_LIST invalid;
    apr_pool_t *invalid_pool;
    int i;
    int c;
    REQUIREMENT req;
    CAPABILITY_LIST capList;

	if (hashMap_containsKey(candidatesMap, targetModule)) {
		return 0;
	}

	hashMap_put(candidatesMap, targetModule, NULL);

	if (apr_pool_create(&candSetList_pool, module_getBundle(targetModule)->memoryPool) == APR_SUCCESS) {
	    if (linkedList_create(candSetList_pool, &candSetList) == CELIX_SUCCESS) {
            for (i = 0; i < linkedList_size(module_getRequirements(targetModule)); i++) {
                req = (REQUIREMENT) linkedList_get(module_getRequirements(targetModule), i);
                capList = resolver_getCapabilityList(m_resolvedServices, requirement_getTargetName(req));

                if (apr_pool_create(&candidates_pool, module_getBundle(targetModule)->memoryPool) == APR_SUCCESS) {
                    if (linkedList_create(candidates_pool, &candidates) == CELIX_SUCCESS) {
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
                            LINKED_LIST_ITERATOR iterator = NULL;
                            for (iterator = linkedListIterator_create(candidates, 0); linkedListIterator_hasNext(iterator); ) {
                                CAPABILITY candidate = (CAPABILITY) linkedListIterator_next(iterator);
                            	MODULE module = NULL;
                            	capability_getModule(candidate, &module);
                                if (!module_isResolved(module)) {
                                    if (resolver_populateCandidatesMap(candidatesMap, module) != 0) {
                                        linkedListIterator_remove(iterator);
                                    }
                                }
                            }
                            linkedListIterator_destroy(iterator);
                        }

                        if (linkedList_size(candidates) == 0) {
                            if (apr_pool_create(&invalid_pool, module_getBundle(targetModule)->memoryPool) == APR_SUCCESS) {
                                if (linkedList_create(invalid_pool, &invalid) == CELIX_SUCCESS) {
                                    resolver_removeInvalidCandidate(targetModule, candidatesMap, invalid);
                                    apr_pool_destroy(invalid_pool);
                                    apr_pool_destroy(candidates_pool);
                                    char *name = NULL;
                                    module_getSymbolicName(targetModule, &name);

                                    printf("Unable to resolve: %s, %s\n", name, requirement_getTargetName(req));
                                }
                            }
                            return -1;
                        } else if (linkedList_size(candidates) > 0) {
                            CANDIDATE_SET cs = (CANDIDATE_SET) malloc(sizeof(*cs));
                            cs->candidates = candidates;
                            cs->module = targetModule;
                            cs->requirement = req;
                            linkedList_addElement(candSetList, cs);
                        }

                    }
                }
            }
            hashMap_put(candidatesMap, targetModule, candSetList);
	    }
	}
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
					MODULE module = NULL;
					capability_getModule(candCap, &module);
					if (module == invalidModule) {
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
    apr_pool_t *modules_pool;
    apr_pool_t *unresolvedServices_pool;
    apr_pool_t *resolvedServices_pool;
    int i;
    CAPABILITY cap;
    CAPABILITY_LIST list;
    apr_pool_t *capabilities_pool;

	if (m_modules == NULL) {
	    if (apr_pool_create(&modules_pool, module_getBundle(module)->memoryPool) == APR_SUCCESS) {
	        if (apr_pool_create(&unresolvedServices_pool, module_getBundle(module)->memoryPool) == APR_SUCCESS) {
	            if (apr_pool_create(&resolvedServices_pool, module_getBundle(module)->memoryPool) == APR_SUCCESS) {
	                linkedList_create(modules_pool, &m_modules);
	                linkedList_create(unresolvedServices_pool, &m_unresolvedServices);
	                linkedList_create(resolvedServices_pool, &m_resolvedServices);
	            }
	        }
	    }
	}

	if (m_modules != NULL && m_unresolvedServices != NULL) {
        linkedList_addElement(m_modules, module);

        for (i = 0; i < linkedList_size(module_getCapabilities(module)); i++) {
            cap = (CAPABILITY) linkedList_get(module_getCapabilities(module), i);
            char *serviceName = NULL;
			capability_getServiceName(cap, &serviceName);
            list = resolver_getCapabilityList(m_unresolvedServices, serviceName);
            if (list == NULL) {
                if (apr_pool_create(&capabilities_pool, module_getBundle(module)->memoryPool) == APR_SUCCESS) {
                    list = (CAPABILITY_LIST) apr_palloc(capabilities_pool, sizeof(*list));
                    if (list != NULL) {
                        list->serviceName = apr_pstrdup(capabilities_pool, serviceName);
                        if (linkedList_create(capabilities_pool, &list->capabilities) == APR_SUCCESS) {
                            linkedList_addElement(m_unresolvedServices, list);
                        }
                    }
                }
            }
            linkedList_addElement(list->capabilities, cap);
        }
	}
}

void resolver_removeModule(MODULE module) {
    linkedList_removeElement(m_modules, module);
    LINKED_LIST caps = module_getCapabilities(module);
    if (caps != NULL)
    {
        int i = 0;
        for (i = 0; i < linkedList_size(caps); i++) {
            CAPABILITY cap = (CAPABILITY) linkedList_get(caps, i);
            char *serviceName = NULL;
			capability_getServiceName(cap, &serviceName);
            CAPABILITY_LIST list = resolver_getCapabilityList(m_unresolvedServices, serviceName);
            if (list != NULL) {
                linkedList_removeElement(list->capabilities, cap);
            }
            list = resolver_getCapabilityList(m_resolvedServices, serviceName);
            if (list != NULL) {
                linkedList_removeElement(list->capabilities, cap);
            }
        }
    }
}

void resolver_moduleResolved(MODULE module) {
    int capIdx;
    LINKED_LIST capsCopy;
    apr_pool_t *capsCopy_pool;
    apr_pool_t *capabilities_pool;

	if (module_isResolved(module)) {
	    if (apr_pool_create(&capsCopy_pool, module_getBundle(module)->memoryPool) == APR_SUCCESS) {
	        if (linkedList_create(capsCopy_pool, &capsCopy) == APR_SUCCESS) {
                for (capIdx = 0; (module_getCapabilities(module) != NULL) && (capIdx < linkedList_size(module_getCapabilities(module))); capIdx++) {
                    CAPABILITY cap = (CAPABILITY) linkedList_get(module_getCapabilities(module), capIdx);
                    char *serviceName = NULL;
					capability_getServiceName(cap, &serviceName);
                    CAPABILITY_LIST list = resolver_getCapabilityList(m_unresolvedServices, serviceName);
                    linkedList_removeElement(list->capabilities, cap);

                    linkedList_addElement(capsCopy, cap);
                }

                LINKED_LIST wires = module_getWires(module);
                for (capIdx = 0; (capsCopy != NULL) && (capIdx < linkedList_size(capsCopy)); capIdx++) {
                    CAPABILITY cap = linkedList_get(capsCopy, capIdx);

                    int wireIdx = 0;
                    for (wireIdx = 0; (wires != NULL) && (wireIdx < linkedList_size(wires)); wireIdx++) {
                        WIRE wire = (WIRE) linkedList_get(wires, wireIdx);
                        REQUIREMENT req = NULL;
                        wire_getRequirement(wire, &req);
                        if (requirement_isSatisfied(req, cap)) {
                            linkedList_set(capsCopy, capIdx, NULL);
                            break;
                        }
                    }
                }

                for (capIdx = 0; (capsCopy != NULL) && (capIdx < linkedList_size(capsCopy)); capIdx++) {
                    CAPABILITY cap = linkedList_get(capsCopy, capIdx);

                    if (cap != NULL) {
                    	char *serviceName = NULL;
						capability_getServiceName(cap, &serviceName);

                        CAPABILITY_LIST list = resolver_getCapabilityList(m_resolvedServices, serviceName);
                        if (list == NULL) {
                            if (apr_pool_create(&capabilities_pool, module_getBundle(module)->memoryPool) == APR_SUCCESS) {
                                list = (CAPABILITY_LIST) apr_palloc(capabilities_pool, sizeof(*list));
                                if (list != NULL) {
                                    list->serviceName = apr_pstrdup(capabilities_pool, serviceName);
                                    if (linkedList_create(capabilities_pool, &list->capabilities) == APR_SUCCESS) {
                                        linkedList_addElement(m_resolvedServices, list);
                                    }
                                }
                            }
                        }
                        linkedList_addElement(list->capabilities, cap);
                    }
                }

                apr_pool_destroy(capsCopy_pool);
	        }
	    }
	}
}

CAPABILITY_LIST resolver_getCapabilityList(LINKED_LIST list, char * name) {
	CAPABILITY_LIST capabilityList = NULL;
	LINKED_LIST_ITERATOR iterator = linkedListIterator_create(list, 0);
	while (linkedListIterator_hasNext(iterator)) {
		CAPABILITY_LIST services = (CAPABILITY_LIST) linkedListIterator_next(iterator);
		if (strcmp(services->serviceName, name) == 0) {
			capabilityList = services;
			break;
		}
	}
	linkedListIterator_destroy(iterator);
	return capabilityList;
}

HASH_MAP resolver_populateWireMap(HASH_MAP candidates, MODULE importer, HASH_MAP wireMap) {
    LINKED_LIST serviceWires;
    apr_pool_t *serviceWires_pool;
    LINKED_LIST emptyWires;
    apr_pool_t *emptyWires_pool;

    if (candidates && importer && wireMap) {
        if (module_isResolved(importer) || (hashMap_get(wireMap, importer))) {
            return wireMap;
        }

        LINKED_LIST candSetList = (LINKED_LIST) hashMap_get(candidates, importer);

        if (apr_pool_create(&serviceWires_pool, module_getBundle(importer)->memoryPool) == APR_SUCCESS) {
            if (apr_pool_create(&emptyWires_pool, module_getBundle(importer)->memoryPool) == APR_SUCCESS) {
                if (linkedList_create(serviceWires_pool, &serviceWires) == APR_SUCCESS) {
                    if (linkedList_create(emptyWires_pool, &emptyWires) == APR_SUCCESS) {
                        hashMap_put(wireMap, importer, emptyWires);

                        int candSetIdx = 0;
                        for (candSetIdx = 0; candSetIdx < linkedList_size(candSetList); candSetIdx++) {
                            CANDIDATE_SET cs = (CANDIDATE_SET) linkedList_get(candSetList, candSetIdx);

                            MODULE module = NULL;
                            capability_getModule(((CAPABILITY) linkedList_get(cs->candidates, 0)), &module);
                            if (importer != module) {
                                WIRE wire = NULL;
                                wire_create(serviceWires_pool, importer, cs->requirement,
                                        module,
                                        ((CAPABILITY) linkedList_get(cs->candidates, 0)), &wire);
                                linkedList_addElement(serviceWires, wire);
                            }

                            wireMap = resolver_populateWireMap(candidates,
                                    module,
                                    wireMap);
                        }

                        hashMap_put(wireMap, importer, serviceWires);
                    }
                }
            }
        }
    }

    return wireMap;
}
