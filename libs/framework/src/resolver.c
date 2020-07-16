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
 * resolver.c
 *
 *  \date       Jul 13, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "resolver.h"
#include "linked_list_iterator.h"
#include "bundle.h"
#include "celix_log.h"

struct capabilityList {
    char * serviceName;
    linked_list_pt capabilities;
};

typedef struct capabilityList * capability_list_pt;

struct candidateSet {
    module_pt module;
    requirement_pt requirement;
    linked_list_pt candidates;
};

typedef struct candidateSet * candidate_set_pt;

// List containing module_ts
linked_list_pt m_modules = NULL;
// List containing capability_t_LISTs
linked_list_pt m_unresolvedServices = NULL;
// List containing capability_t_LISTs
linked_list_pt m_resolvedServices = NULL;

int resolver_populateCandidatesMap(hash_map_pt candidatesMap, module_pt targetModule);
capability_list_pt resolver_getCapabilityList(linked_list_pt list, const char* name);
void resolver_removeInvalidCandidate(module_pt module, hash_map_pt candidates, linked_list_pt invalid);
linked_list_pt resolver_populateWireMap(hash_map_pt candidates, module_pt importer, linked_list_pt wireMap);

linked_list_pt resolver_resolve(module_pt root) {
    hash_map_pt candidatesMap = NULL;
    linked_list_pt wireMap = NULL;
    linked_list_pt resolved = NULL;
    hash_map_iterator_pt iter = NULL;

    if (module_isResolved(root)) {
        return NULL;
    }

    candidatesMap = hashMap_create(NULL, NULL, NULL, NULL);

    if (resolver_populateCandidatesMap(candidatesMap, root) != 0) {
        iter = hashMapIterator_create(candidatesMap);
        while (hashMapIterator_hasNext(iter)) {
            hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
            linked_list_pt value = hashMapEntry_getValue(entry);
            hashMapIterator_remove(iter);
            if (value != NULL) {
                linked_list_iterator_pt candSetIter = linkedListIterator_create(value, 0);
                while (linkedListIterator_hasNext(candSetIter)) {
                    candidate_set_pt set = linkedListIterator_next(candSetIter);
                    linkedList_destroy(set->candidates);
                    free(set);
                    linkedListIterator_remove(candSetIter);
                }
                linkedListIterator_destroy(candSetIter);
                linkedList_destroy(value);
            }
        }
        hashMapIterator_destroy(iter);
        hashMap_destroy(candidatesMap, false, false);
        return NULL;
    }

    linkedList_create(&wireMap);
    resolved = resolver_populateWireMap(candidatesMap, root, wireMap);
    iter = hashMapIterator_create(candidatesMap);
    while (hashMapIterator_hasNext(iter)) {
        hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
        linked_list_pt value = hashMapEntry_getValue(entry);
        hashMapIterator_remove(iter);
        if (value != NULL) {
            linked_list_iterator_pt candSetIter = linkedListIterator_create(value, 0);
            while (linkedListIterator_hasNext(candSetIter)) {
                candidate_set_pt set = linkedListIterator_next(candSetIter);
                linkedList_destroy(set->candidates);
                free(set);
                linkedListIterator_remove(candSetIter);
            }
            linkedListIterator_destroy(candSetIter);
            linkedList_destroy(value);
        }
    }
    hashMapIterator_destroy(iter);
    hashMap_destroy(candidatesMap, false, false);
    return resolved;
}

int resolver_populateCandidatesMap(hash_map_pt candidatesMap, module_pt targetModule) {
    linked_list_pt candSetList;
    linked_list_pt candidates;
    linked_list_pt invalid;

    if (hashMap_containsKey(candidatesMap, targetModule)) {
        return 0;
    }

    hashMap_put(candidatesMap, targetModule, NULL);

    if (linkedList_create(&candSetList) == CELIX_SUCCESS) {
        int i;
        for (i = 0; i < linkedList_size(module_getRequirements(targetModule)); i++) {
            capability_list_pt capList;
            requirement_pt req;
            const char *targetName = NULL;
            req = (requirement_pt) linkedList_get(module_getRequirements(targetModule), i);
            requirement_getTargetName(req, &targetName);
            capList = resolver_getCapabilityList(m_resolvedServices, targetName);

            if (linkedList_create(&candidates) == CELIX_SUCCESS) {
                int c;
                for (c = 0; (capList != NULL) && (c < linkedList_size(capList->capabilities)); c++) {
                    capability_pt cap = (capability_pt) linkedList_get(capList->capabilities, c);
                    bool satisfied = false;
                    requirement_isSatisfied(req, cap, &satisfied);
                    if (satisfied) {
                        linkedList_addElement(candidates, cap);
                    }
                }
                capList = resolver_getCapabilityList(m_unresolvedServices, targetName);
                for (c = 0; (capList != NULL) && (c < linkedList_size(capList->capabilities)); c++) {
                    capability_pt cap = (capability_pt) linkedList_get(capList->capabilities, c);
                    bool satisfied = false;
                    requirement_isSatisfied(req, cap, &satisfied);
                    if (satisfied) {
                        linkedList_addElement(candidates, cap);
                    }
                }

                if (linkedList_size(candidates) > 0) {
                    linked_list_iterator_pt iterator = NULL;
                    for (iterator = linkedListIterator_create(candidates, 0); linkedListIterator_hasNext(iterator);) {
                        capability_pt candidate = (capability_pt) linkedListIterator_next(iterator);
                        module_pt module = NULL;
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
                    if (linkedList_create(&invalid) == CELIX_SUCCESS) {
                        const char *name = NULL;
                        resolver_removeInvalidCandidate(targetModule, candidatesMap, invalid);

                        module_getSymbolicName(targetModule, &name);

                        linkedList_destroy(invalid);
                        fw_log(celix_frameworkLogger_globalLogger(), CELIX_LOG_LEVEL_INFO, "Unable to resolve: %s, %s\n", name, targetName);
                    }
                    linkedList_destroy(candidates);
                    linkedList_destroy(candSetList);
                    return -1;
                } else if (linkedList_size(candidates) > 0) {
                    candidate_set_pt cs = (candidate_set_pt) malloc(sizeof(*cs));
                    cs->candidates = candidates;
                    cs->module = targetModule;
                    cs->requirement = req;
                    linkedList_addElement(candSetList, cs);
                }

            }
        }
        hashMap_put(candidatesMap, targetModule, candSetList);
    }
    return 0;
}

void resolver_removeInvalidCandidate(module_pt invalidModule, hash_map_pt candidates, linked_list_pt invalid) {
    hash_map_iterator_pt iterator;
    hashMap_remove(candidates, invalidModule);

    for (iterator = hashMapIterator_create(candidates); hashMapIterator_hasNext(iterator);) {
        hash_map_entry_pt entry = hashMapIterator_nextEntry(iterator);
        linked_list_pt candSetList = (linked_list_pt) hashMapEntry_getValue(entry);
        if (candSetList != NULL) {
            linked_list_iterator_pt itCandSetList;
            for (itCandSetList = linkedListIterator_create(candSetList, 0); linkedListIterator_hasNext(itCandSetList);) {
                candidate_set_pt set = (candidate_set_pt) linkedListIterator_next(itCandSetList);
                linked_list_iterator_pt candIter;
                for (candIter = linkedListIterator_create(set->candidates, 0); linkedListIterator_hasNext(candIter);) {
                    capability_pt candCap = (capability_pt) linkedListIterator_next(candIter);
                    module_pt module = NULL;
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
                linkedListIterator_destroy(candIter);
            }
            linkedListIterator_destroy(itCandSetList);
        }
    }
    hashMapIterator_destroy(iterator);

    if (linkedList_size(invalid) > 0) {
        while (!linkedList_isEmpty(invalid)) {
            module_pt m = (module_pt) linkedList_removeIndex(invalid, 0);
            resolver_removeInvalidCandidate(m, candidates, invalid);
        }
    }
}

void resolver_addModule(module_pt module) {

    if (m_modules == NULL) {
        linkedList_create(&m_modules);
        linkedList_create(&m_unresolvedServices);
        linkedList_create(&m_resolvedServices);
    }

    if (m_modules != NULL && m_unresolvedServices != NULL) {
        int i;

        linkedList_addElement(m_modules, module);

        for (i = 0; i < linkedList_size(module_getCapabilities(module)); i++) {
            const char *serviceName = NULL;
            capability_list_pt list = NULL;
            capability_pt cap;

            cap = (capability_pt) linkedList_get(module_getCapabilities(module), i);
            capability_getServiceName(cap, &serviceName);
            list = resolver_getCapabilityList(m_unresolvedServices, serviceName);
            if (list == NULL) {
                list = (capability_list_pt) malloc(sizeof(*list));
                if (list != NULL) {
                    list->serviceName = strdup(serviceName);
                    if (linkedList_create(&list->capabilities) == CELIX_SUCCESS) {
                        linkedList_addElement(m_unresolvedServices, list);
                    }
                    else{
                    	free(list->serviceName);
                    	free(list);
			list=NULL;
                    }
                }
            }
            if(list != NULL){
		linkedList_addElement(list->capabilities, cap);
            }
        }
    }
}

void resolver_removeModule(module_pt module) {
    if (m_modules == NULL) {
        return;
    }
    linked_list_pt caps = NULL;
    linkedList_removeElement(m_modules, module);
    caps = module_getCapabilities(module);
    if (caps != NULL) {
        int i = 0;
        for (i = 0; i < linkedList_size(caps); i++) {
            capability_pt cap = (capability_pt) linkedList_get(caps, i);
            const char *serviceName = NULL;
            capability_list_pt list;
            capability_getServiceName(cap, &serviceName);
            list = resolver_getCapabilityList(m_unresolvedServices, serviceName);
            if (list != NULL) {
                linkedList_removeElement(list->capabilities, cap);

                if (linkedList_isEmpty(list->capabilities)) {
                    linkedList_removeElement(m_unresolvedServices, list);
                    linkedList_destroy(list->capabilities);
                    free(list->serviceName);
                    free(list);
                }
            }
            list = resolver_getCapabilityList(m_resolvedServices, serviceName);
            if (list != NULL) {
                linkedList_removeElement(list->capabilities, cap);

                if (linkedList_isEmpty(list->capabilities)) {
                    linkedList_removeElement(m_resolvedServices, list);
                    linkedList_destroy(list->capabilities);
                    free(list->serviceName);
                    free(list);
                }
            }
        }
    }
    if (linkedList_isEmpty(m_modules)) {
        linkedList_destroy(m_modules);
        m_modules = NULL;

        if (!linkedList_isEmpty(m_unresolvedServices)) {
            // #TODO: Something is wrong, not all modules have been removed from the resolver
            fw_log(celix_frameworkLogger_globalLogger(), CELIX_LOG_LEVEL_ERROR, "Unexpected entries in unresolved module list");
        }
        linkedList_destroy(m_unresolvedServices);
        m_unresolvedServices = NULL;
        if (!linkedList_isEmpty(m_resolvedServices)) {
            // #TODO: Something is wrong, not all modules have been removed from the resolver
            fw_log(celix_frameworkLogger_globalLogger(), CELIX_LOG_LEVEL_ERROR, "Unexpected entries in resolved module list");
        }
        linkedList_destroy(m_resolvedServices);
        m_resolvedServices = NULL;
    }
}

void resolver_moduleResolved(module_pt module) {

    if (module_isResolved(module)) {
        linked_list_pt capsCopy = NULL;

        if (linkedList_create(&capsCopy) == CELIX_SUCCESS) {
            linked_list_pt wires = NULL;
            int capIdx;

            for (capIdx = 0; (module_getCapabilities(module) != NULL) && (capIdx < linkedList_size(module_getCapabilities(module))); capIdx++) {
                capability_pt cap = (capability_pt) linkedList_get(module_getCapabilities(module), capIdx);
                const char *serviceName = NULL;
                capability_list_pt list;
                capability_getServiceName(cap, &serviceName);
                list = resolver_getCapabilityList(m_unresolvedServices, serviceName);
                if(list != NULL){
			linkedList_removeElement(list->capabilities, cap);
                }

                linkedList_addElement(capsCopy, cap);
            }

            wires = module_getWires(module);
            for (capIdx = 0; (capsCopy != NULL) && (capIdx < linkedList_size(capsCopy)); capIdx++) {
                capability_pt cap = linkedList_get(capsCopy, capIdx);

                int wireIdx = 0;
                for (wireIdx = 0; (wires != NULL) && (wireIdx < linkedList_size(wires)); wireIdx++) {
                    wire_pt wire = (wire_pt) linkedList_get(wires, wireIdx);
                    requirement_pt req = NULL;
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
                capability_pt cap = linkedList_get(capsCopy, capIdx);

                if (cap != NULL) {
                    const char *serviceName = NULL;
                    capability_list_pt list = NULL;
                    capability_getServiceName(cap, &serviceName);

                    list = resolver_getCapabilityList(m_resolvedServices, serviceName);
                    if (list == NULL) {
                        list = (capability_list_pt) malloc(sizeof(*list));
                        if (list != NULL) {
                            list->serviceName = strdup(serviceName);
                            if (linkedList_create(&list->capabilities) == CELIX_SUCCESS) {
                                linkedList_addElement(m_resolvedServices, list);
                            }
                            else{
                            	free(list->serviceName);
                            	free(list);
				list=NULL;
                            }
                        }
                    }
                    if(list != NULL){
			linkedList_addElement(list->capabilities, cap);
                    }
                }
            }

            linkedList_destroy(capsCopy);
        }
    }
}

capability_list_pt resolver_getCapabilityList(linked_list_pt list, const char * name) {
    capability_list_pt capabilityList = NULL;
    linked_list_iterator_pt iterator = linkedListIterator_create(list, 0);
    while (linkedListIterator_hasNext(iterator)) {
        capability_list_pt services = (capability_list_pt) linkedListIterator_next(iterator);
        if (strcmp(services->serviceName, name) == 0) {
            capabilityList = services;
            break;
        }
    }
    linkedListIterator_destroy(iterator);
    return capabilityList;
}

linked_list_pt resolver_populateWireMap(hash_map_pt candidates, module_pt importer, linked_list_pt wireMap) {
    linked_list_pt serviceWires;

    if (candidates && importer && wireMap) {
        linked_list_pt candSetList = NULL;
        bool resolved = false;

        if (module_isResolved(importer)) {
            // already resolved
            resolved = true;
        }
        if (!resolved) {
            bool self = false;
            linked_list_iterator_pt wit = linkedListIterator_create(wireMap, 0);
            while (linkedListIterator_hasNext(wit)) {
                importer_wires_pt iw = linkedListIterator_next(wit);
                if (iw->importer == importer) {
                    // Do not resolve yourself
                    self = true;
                    break;
                }
            }
            linkedListIterator_destroy(wit);

            if (!self) {
                candSetList = (linked_list_pt) hashMap_get(candidates, importer);

                if (linkedList_create(&serviceWires) == CELIX_SUCCESS) {
//                    if (linkedList_create(&emptyWires) == CELIX_SUCCESS) {
                    int candSetIdx = 0;

                    // hashMap_put(wireMap, importer, emptyWires);

                    const char *mname = NULL;
                    module_getSymbolicName(importer, &mname);

                    importer_wires_pt importerWires = malloc(sizeof(*importerWires));
                    importerWires->importer = importer;
                    importerWires->wires = NULL;
                    linkedList_addElement(wireMap, importerWires);

                    for (candSetIdx = 0; candSetIdx < linkedList_size(candSetList); candSetIdx++) {
                        candidate_set_pt cs = (candidate_set_pt) linkedList_get(candSetList, candSetIdx);

                        module_pt module = NULL;
                        capability_getModule(((capability_pt) linkedList_get(cs->candidates, 0)), &module);
                        if (importer != module) {
                            wire_pt wire = NULL;
                            wire_create(importer, cs->requirement, module, ((capability_pt) linkedList_get(cs->candidates, 0)), &wire);
                            linkedList_addElement(serviceWires, wire);
                        }

                        wireMap = resolver_populateWireMap(candidates, module, wireMap);
                    }

                    importerWires->wires = serviceWires;
                    // hashMap_put(wireMap, importer, serviceWires);
//                    }
                }
            }
        }
    }

    return wireMap;
}
