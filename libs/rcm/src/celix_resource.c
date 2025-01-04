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

#include <stdlib.h>

#include "celix_string_hash_map.h"
#include "celix_err.h"
#include "celix_resource.h"
#include "celix_capability.h"
#include "celix_requirement.h"

struct celix_resource {
    celix_array_list_t* allCapabilities;
    celix_array_list_t* allRequirements;
    celix_string_hash_map_t* capabilitiesByNamespace; //note weak ref to capabilities in allCapabilities
    celix_string_hash_map_t* requirementsByNamespace; //note weak ref to requirements in allRequirements
};

static void celix_resource_destroyCapability(void* capability) {
    celix_capability_destroy((celix_capability_t*)capability);
}

static void celix_resource_destroyRequirement(void* requirement) {
    celix_requirement_destroy((celix_requirement_t*)requirement);
}

static void celix_resource_freeReqOrCapList(void* list) {
    celix_arrayList_destroy((celix_array_list_t*)list);
}

celix_resource_t* celix_resource_create() {
    celix_resource_t* res = malloc(sizeof(*res));
    if (res == NULL) {
        celix_err_push("Failed to allocate celix_resource_t. Out of memory.");
        return NULL;
    }

    celix_array_list_create_options_t opts = CELIX_EMPTY_ARRAY_LIST_CREATE_OPTIONS;
    opts.simpleRemovedCallback = celix_resource_destroyCapability;
    res->allCapabilities = celix_arrayList_createWithOptions(&opts);

    opts.simpleRemovedCallback = celix_resource_destroyRequirement;
    res->allRequirements = celix_arrayList_createWithOptions(&opts);

    celix_string_hash_map_create_options_t mapOpts = CELIX_EMPTY_STRING_HASH_MAP_CREATE_OPTIONS;
    mapOpts.simpleRemovedCallback = celix_resource_freeReqOrCapList;
    res->capabilitiesByNamespace = celix_stringHashMap_createWithOptions(&mapOpts);
    res->requirementsByNamespace = celix_stringHashMap_createWithOptions(&mapOpts);

    if (    res->allCapabilities == NULL ||
            res->allRequirements == NULL ||
            res->capabilitiesByNamespace == NULL ||
            res->requirementsByNamespace == NULL) {
        celix_err_push("Failed to allocate celix_resource_t fields. Out of memory.");
        celix_resource_destroy(res);
        return NULL;
    }
    return res;
}

void celix_resource_destroy(celix_resource_t* resource) {
    if (resource != NULL) {
        celix_arrayList_destroy(resource->allCapabilities);
        celix_arrayList_destroy(resource->allRequirements);
        celix_stringHashMap_destroy(resource->capabilitiesByNamespace);
        celix_stringHashMap_destroy(resource->requirementsByNamespace);
        free(resource);
    }
}

const celix_array_list_t* celix_resource_getCapabilities(const celix_resource_t* res, const char* ns) {
    if (ns != NULL) {
        return celix_stringHashMap_get(res->capabilitiesByNamespace, ns);
    }
    return res->allCapabilities;
}

const celix_array_list_t* celix_resource_getRequirements(const celix_resource_t* res, const char* ns) {
    if (ns != NULL) {
        return celix_stringHashMap_get(res->requirementsByNamespace, ns);
    }
    return res->allRequirements;
}

celix_status_t celix_resource_addCapability(celix_resource_t* res, celix_capability_t* cap) {
    if (celix_capability_getResource(cap) != res) {
        celix_err_pushf(
                "Capability is not added to the correct resource. (Capability is added to resource %p, but resource %p is expected.)",
                celix_capability_getResource(cap), res);
        celix_capability_destroy(cap);
        return CELIX_ILLEGAL_ARGUMENT;
    }

    const char* ns = celix_capability_getNamespace(cap);
    celix_array_list_t* caps = celix_stringHashMap_get(res->capabilitiesByNamespace, ns);
    if (caps == NULL) {
        caps = celix_arrayList_createPointerArray();
        if (caps == NULL) {
            goto err_handling;
        }
        celix_stringHashMap_put(res->capabilitiesByNamespace, ns, caps);
    }
    CELIX_GOTO_IF_ERR(celix_arrayList_add(caps, cap), err_handling);
    CELIX_GOTO_IF_ERR(celix_arrayList_add(res->allCapabilities, cap), err_handling);
    return CELIX_SUCCESS;
err_handling:
    celix_err_push("Failed to add capability to resource. Out of memory.");
    if (caps != NULL) {
        celix_arrayList_remove(caps, cap);
    }
    celix_capability_destroy(cap);
    return ENOMEM;
}

celix_status_t celix_resource_addRequirement(celix_resource_t* res, celix_requirement_t* req) {
    if (celix_requirement_getResource(req) != res) {
        celix_err_pushf(
                "Requirement is not added to the correct resource. (Requirement is added to resource %p, but resource %p is expected.)",
                celix_requirement_getResource(req), res);
        celix_requirement_destroy(req);
        return CELIX_ILLEGAL_ARGUMENT;
    }

    const char* ns = celix_requirement_getNamespace(req);
    celix_array_list_t* reqs = celix_stringHashMap_get(res->requirementsByNamespace, ns);
    if (reqs == NULL) {
        reqs = celix_arrayList_createPointerArray();
        if (reqs == NULL) {
            goto err_handling;
        }
        celix_stringHashMap_put(res->requirementsByNamespace, ns, reqs);
    }
    CELIX_GOTO_IF_ERR(celix_arrayList_add(reqs, req), err_handling);
    CELIX_GOTO_IF_ERR(celix_arrayList_add(res->allRequirements, req), err_handling);
    return CELIX_SUCCESS;
err_handling:
    celix_err_push("Failed to add requirement to resource. Out of memory.");
    if (reqs != NULL) {
        celix_arrayList_remove(reqs, req);
    }
    celix_requirement_destroy(req);
    return ENOMEM;
}
