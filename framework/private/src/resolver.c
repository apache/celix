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
 *  \date       Jul 13, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <apr_strings.h>

#include "resolver.h"
#include "linked_list_iterator.h"
#include "bundle.h"

struct capabilityList {
	char * serviceName;
	linked_list_t capabilities;
};

typedef struct capabilityList * capability_t_LIST;

struct candidateSet {
	module_t module;
	requirement_t requirement;
	linked_list_t candidates;
};

typedef struct candidateSet * CANDIDATE_SET;

// List containing module_ts
linked_list_t m_modules = NULL;
// List containing capability_t_LISTs
linked_list_t m_unresolvedServices = NULL;
// List containing capability_t_LISTs
linked_list_t m_resolvedServices = NULL;

int resolver_populateCandidatesMap(hash_map_t candidatesMap, module_t targetModule);
capability_t_LIST resolver_getCapabilityList(linked_list_t list, char * name);
void resolver_removeInvalidCandidate(module_t module, hash_map_t candidates, linked_list_t invalid);
hash_map_t resolver_populateWireMap(hash_map_t candidates, module_t importer, hash_map_t wireMap);

hash_map_t resolver_resolve(module_t root) {
	hash_map_t candidatesMap = NULL;
	hash_map_t wireMap = NULL;
	hash_map_t resolved = NULL;
	hash_map_iterator_t iter = NULL;

	if (module_isResolved(root)) {
	    printf("already resolved\n");
		return NULL;
	}

	candidatesMap = hashMap_create(NULL, NULL, NULL, NULL);

	if (resolver_populateCandidatesMap(candidatesMap, root) != 0) {
		hash_map_iterator_t iter = hashMapIterator_create(candidatesMap);
		while (hashMapIterator_hasNext(iter)) {
			hash_map_entry_t entry = hashMapIterator_nextEntry(iter);
			module_t key = hashMapEntry_getKey(entry);
			linked_list_t value = hashMapEntry_getValue(entry);
			hashMapIterator_remove(iter);
			if (value != NULL) {
				linked_list_iterator_t candSetIter = linkedListIterator_create(value, 0);
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

	wireMap = hashMap_create(NULL,NULL,NULL,NULL);
	resolved = resolver_populateWireMap(candidatesMap, root, wireMap);
	iter = hashMapIterator_create(candidatesMap);
	while (hashMapIterator_hasNext(iter)) {
		hash_map_entry_t entry = hashMapIterator_nextEntry(iter);
		module_t key = hashMapEntry_getKey(entry);
		linked_list_t value = hashMapEntry_getValue(entry);
		hashMapIterator_remove(iter);
		if (value != NULL) {
			linked_list_iterator_t candSetIter = linkedListIterator_create(value, 0);
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

int resolver_populateCandidatesMap(hash_map_t candidatesMap, module_t targetModule) {
    linked_list_t candSetList;
	apr_pool_t *candSetList_pool;
    linked_list_t candidates;
    apr_pool_t *candidates_pool;
    linked_list_t invalid;
    apr_pool_t *invalid_pool;
    int i;
    int c;
    requirement_t req;
    capability_t_LIST capList;
    apr_pool_t *bundlePool = NULL;
    bundle_t bundle = NULL;

    bundle = module_getBundle(targetModule);
    bundle_getMemoryPool(bundle, &bundlePool);


	if (hashMap_containsKey(candidatesMap, targetModule)) {
		return 0;
	}

	hashMap_put(candidatesMap, targetModule, NULL);

	if (apr_pool_create(&candSetList_pool, bundlePool) == APR_SUCCESS) {
	    if (linkedList_create(candSetList_pool, &candSetList) == CELIX_SUCCESS) {
            for (i = 0; i < linkedList_size(module_getRequirements(targetModule)); i++) {
            	char *targetName = NULL;
                req = (requirement_t) linkedList_get(module_getRequirements(targetModule), i);
                requirement_getTargetName(req, &targetName);
                capList = resolver_getCapabilityList(m_resolvedServices, targetName);

                if (apr_pool_create(&candidates_pool, bundlePool) == APR_SUCCESS) {
                    if (linkedList_create(candidates_pool, &candidates) == CELIX_SUCCESS) {
                        for (c = 0; (capList != NULL) && (c < linkedList_size(capList->capabilities)); c++) {
                            capability_t cap = (capability_t) linkedList_get(capList->capabilities, c);
                            bool satisfied = false;
                            requirement_isSatisfied(req, cap, &satisfied);
                            if (satisfied) {
                                linkedList_addElement(candidates, cap);
                            }
                        }
                        capList = resolver_getCapabilityList(m_unresolvedServices, targetName);
                        for (c = 0; (capList != NULL) && (c < linkedList_size(capList->capabilities)); c++) {
                            capability_t cap = (capability_t) linkedList_get(capList->capabilities, c);
                            bool satisfied = false;
                            requirement_isSatisfied(req, cap, &satisfied);
                            if (satisfied) {
                                linkedList_addElement(candidates, cap);
                            }
                        }

                        if (linkedList_size(candidates) > 0) {
                            linked_list_iterator_t iterator = NULL;
                            for (iterator = linkedListIterator_create(candidates, 0); linkedListIterator_hasNext(iterator); ) {
                                capability_t candidate = (capability_t) linkedListIterator_next(iterator);
                            	module_t module = NULL;
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
                            if (apr_pool_create(&invalid_pool, bundlePool) == APR_SUCCESS) {
                                if (linkedList_create(invalid_pool, &invalid) == CELIX_SUCCESS) {
									char *name = NULL;
                                    resolver_removeInvalidCandidate(targetModule, candidatesMap, invalid);
                                    apr_pool_destroy(invalid_pool);
                                    apr_pool_destroy(candidates_pool);
                                    
                                    module_getSymbolicName(targetModule, &name);

                                    printf("Unable to resolve: %s, %s\n", name, targetName);
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

void resolver_removeInvalidCandidate(module_t invalidModule, hash_map_t candidates, linked_list_t invalid) {
	hash_map_iterator_t iterator;
	hashMap_remove(candidates, invalidModule);
	
	for (iterator = hashMapIterator_create(candidates); hashMapIterator_hasNext(iterator); ) {
		hash_map_entry_t entry = hashMapIterator_nextEntry(iterator);
		module_t module = (module_t) hashMapEntry_getKey(entry);
		linked_list_t candSetList = (linked_list_t) hashMapEntry_getValue(entry);
		if (candSetList != NULL) {
			linked_list_iterator_t itCandSetList;
			for (itCandSetList = linkedListIterator_create(candSetList, 0); linkedListIterator_hasNext(itCandSetList); ) {
				CANDIDATE_SET set = (CANDIDATE_SET) linkedListIterator_next(itCandSetList);
				linked_list_iterator_t candIter;
				for (candIter = linkedListIterator_create(set->candidates, 0); linkedListIterator_hasNext(candIter); ) {
					capability_t candCap = (capability_t) linkedListIterator_next(candIter);
					module_t module = NULL;
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
			module_t m = (module_t) linkedList_removeIndex(invalid, 0);
			resolver_removeInvalidCandidate(m, candidates, invalid);
		}
	}
}

void resolver_addModule(module_t module) {
    apr_pool_t *modules_pool;
    apr_pool_t *unresolvedServices_pool;
    apr_pool_t *resolvedServices_pool;
    int i;
    capability_t cap;
    capability_t_LIST list;
    apr_pool_t *capabilities_pool;
    apr_pool_t *bundlePool = NULL;
	bundle_t bundle = NULL;

	bundle = module_getBundle(module);
	bundle_getMemoryPool(bundle, &bundlePool);

	if (m_modules == NULL) {
	    if (apr_pool_create(&modules_pool, bundlePool) == APR_SUCCESS) {
	        if (apr_pool_create(&unresolvedServices_pool, bundlePool) == APR_SUCCESS) {
	            if (apr_pool_create(&resolvedServices_pool, bundlePool) == APR_SUCCESS) {
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
			char *serviceName = NULL;
            cap = (capability_t) linkedList_get(module_getCapabilities(module), i);
            capability_getServiceName(cap, &serviceName);
            list = resolver_getCapabilityList(m_unresolvedServices, serviceName);
            if (list == NULL) {
                if (apr_pool_create(&capabilities_pool, bundlePool) == APR_SUCCESS) {
                    list = (capability_t_LIST) apr_palloc(capabilities_pool, sizeof(*list));
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

void resolver_removeModule(module_t module) {
    linked_list_t caps = NULL;
	linkedList_removeElement(m_modules, module);
    caps = module_getCapabilities(module);
    if (caps != NULL)
    {
        int i = 0;
        for (i = 0; i < linkedList_size(caps); i++) {
            capability_t cap = (capability_t) linkedList_get(caps, i);
            char *serviceName = NULL;
			capability_t_LIST list;
			capability_getServiceName(cap, &serviceName);
            list = resolver_getCapabilityList(m_unresolvedServices, serviceName);
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

void resolver_moduleResolved(module_t module) {
    int capIdx;
    linked_list_t capsCopy;
    apr_pool_t *capsCopy_pool;
    apr_pool_t *capabilities_pool;
    apr_pool_t *bundlePool = NULL;
	bundle_t bundle = NULL;

	bundle = module_getBundle(module);
	bundle_getMemoryPool(bundle, &bundlePool);

	if (module_isResolved(module)) {
	    if (apr_pool_create(&capsCopy_pool, bundlePool) == APR_SUCCESS) {
	        if (linkedList_create(capsCopy_pool, &capsCopy) == APR_SUCCESS) {
                linked_list_t wires = NULL;

				for (capIdx = 0; (module_getCapabilities(module) != NULL) && (capIdx < linkedList_size(module_getCapabilities(module))); capIdx++) {
                    capability_t cap = (capability_t) linkedList_get(module_getCapabilities(module), capIdx);
                    char *serviceName = NULL;
					capability_t_LIST list;
					capability_getServiceName(cap, &serviceName);
                    list = resolver_getCapabilityList(m_unresolvedServices, serviceName);
                    linkedList_removeElement(list->capabilities, cap);

                    linkedList_addElement(capsCopy, cap);
                }

                wires = module_getWires(module);
                for (capIdx = 0; (capsCopy != NULL) && (capIdx < linkedList_size(capsCopy)); capIdx++) {
                    capability_t cap = linkedList_get(capsCopy, capIdx);

                    int wireIdx = 0;
                    for (wireIdx = 0; (wires != NULL) && (wireIdx < linkedList_size(wires)); wireIdx++) {
                        wire_t wire = (wire_t) linkedList_get(wires, wireIdx);
                        requirement_t req = NULL;
                        bool satisfied = false;
                        wire_getRequirement(wire, &req);
						requirement_isSatisfied(req, cap, &satisfied);
                        if (satisfied) {
                            linkedList_set(capsCopy, capIdx, NULL);
                            break;
                        }
                    }
                }

                for (capIdx = 0; (capsCopy != NULL) && (capIdx < linkedList_size(capsCopy)); capIdx++) {
                    capability_t cap = linkedList_get(capsCopy, capIdx);

                    if (cap != NULL) {
                    	char *serviceName = NULL;
						capability_t_LIST list;
						capability_getServiceName(cap, &serviceName);

                        list = resolver_getCapabilityList(m_resolvedServices, serviceName);
                        if (list == NULL) {
                            if (apr_pool_create(&capabilities_pool, bundlePool) == APR_SUCCESS) {
                                list = (capability_t_LIST) apr_palloc(capabilities_pool, sizeof(*list));
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

capability_t_LIST resolver_getCapabilityList(linked_list_t list, char * name) {
	capability_t_LIST capabilityList = NULL;
	linked_list_iterator_t iterator = linkedListIterator_create(list, 0);
	while (linkedListIterator_hasNext(iterator)) {
		capability_t_LIST services = (capability_t_LIST) linkedListIterator_next(iterator);
		if (strcmp(services->serviceName, name) == 0) {
			capabilityList = services;
			break;
		}
	}
	linkedListIterator_destroy(iterator);
	return capabilityList;
}

hash_map_t resolver_populateWireMap(hash_map_t candidates, module_t importer, hash_map_t wireMap) {
    linked_list_t serviceWires;
    apr_pool_t *serviceWires_pool;
    linked_list_t emptyWires;
    apr_pool_t *emptyWires_pool;
    apr_pool_t *bundlePool = NULL;
	bundle_t bundle = NULL;

	bundle = module_getBundle(importer);
	bundle_getMemoryPool(bundle, &bundlePool);

    if (candidates && importer && wireMap) {
        linked_list_t candSetList = NULL;
		if (module_isResolved(importer) || (hashMap_get(wireMap, importer))) {
            return wireMap;
        }

        candSetList = (linked_list_t) hashMap_get(candidates, importer);

        if (apr_pool_create(&serviceWires_pool, bundlePool) == APR_SUCCESS) {
            if (apr_pool_create(&emptyWires_pool, bundlePool) == APR_SUCCESS) {
                if (linkedList_create(serviceWires_pool, &serviceWires) == APR_SUCCESS) {
                    if (linkedList_create(emptyWires_pool, &emptyWires) == APR_SUCCESS) {
                        int candSetIdx = 0;
						
						hashMap_put(wireMap, importer, emptyWires);
                        
                        for (candSetIdx = 0; candSetIdx < linkedList_size(candSetList); candSetIdx++) {
                            CANDIDATE_SET cs = (CANDIDATE_SET) linkedList_get(candSetList, candSetIdx);

                            module_t module = NULL;
                            capability_getModule(((capability_t) linkedList_get(cs->candidates, 0)), &module);
                            if (importer != module) {
                                wire_t wire = NULL;
                                wire_create(serviceWires_pool, importer, cs->requirement,
                                        module,
                                        ((capability_t) linkedList_get(cs->candidates, 0)), &wire);
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
