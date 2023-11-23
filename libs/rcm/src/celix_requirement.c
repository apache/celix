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

#include "celix_utils.h"
#include "celix_requirement.h"
#include "celix_err.h"

#define CELIX_REQUIREMENT_DIRECTIVE_FILTER "filter"

struct celix_requirement {
    const celix_resource_t* resource; //weak ref
    char* ns;
    celix_properties_t* attributes;
    celix_properties_t* directives;
};

celix_requirement_t* celix_requirement_create(
        celix_resource_t* resource,
        const char* ns,
        const char* filter) {

    if (celix_utils_isStringNullOrEmpty(ns)) {
        celix_err_push("Failed to create celix_requirement_t. Namespace is NULL or empty.");
        return NULL;
    }

    celix_requirement_t* req = malloc(sizeof(*req));
    if (req == NULL) {
        celix_err_push("Failed to allocate celix_requirement_t. Out of memory.");
        return NULL;
    }

    req->resource = resource;
    req->ns = celix_utils_strdup(ns);
    req->attributes = celix_properties_create();
    req->directives = celix_properties_create();
    if (req->ns == NULL || req->attributes == NULL || req->directives == NULL) {
        celix_err_push("Failed to allocate celix_requirement_t fields. Out of memory.");
        celix_requirement_destroy(req);
        return NULL;
    }

    if (filter != NULL) {
        celix_properties_set(req->directives, CELIX_REQUIREMENT_DIRECTIVE_FILTER, filter);
    }

    return req;
}

void celix_requirement_destroy(celix_requirement_t* requirement) {
    if (requirement != NULL) {
        free(requirement->ns);
        celix_properties_destroy(requirement->attributes);
        celix_properties_destroy(requirement->directives);
        free(requirement);
    }
}

bool celix_requirement_equals(const celix_requirement_t* req1, const celix_requirement_t* req2) {
    if (req1 == req2) {
        return true;
    }

    return  celix_utils_stringEquals(req1->ns, req2->ns) &&
            celix_properties_equals(req1->directives, req2->directives) &&
            celix_properties_equals(req1->attributes, req2->attributes);
}

unsigned int celix_requirement_hashCode(const celix_requirement_t* req) {
    unsigned int hash = celix_utils_stringHash(req->ns);
    CELIX_PROPERTIES_ITERATE(req->attributes, visit) {
        hash += celix_utils_stringHash(visit.key);
        hash += celix_utils_stringHash(visit.entry.value);
    }
    CELIX_PROPERTIES_ITERATE(req->directives, visit) {
            hash += celix_utils_stringHash(visit.key);
            hash += celix_utils_stringHash(visit.entry.value);
    }
    return hash;
}

const celix_resource_t* celix_requirement_getResource(const celix_requirement_t* req) {
    return req->resource;
}

const char* celix_requirement_getNamespace(const celix_requirement_t* req) {
    return req->ns;
}

const char* celix_requirement_getFilter(const celix_requirement_t* req) {
    return celix_properties_get(req->directives, CELIX_REQUIREMENT_DIRECTIVE_FILTER, NULL);
}

const celix_properties_t* celix_requirement_getDirectives(const celix_requirement_t* req) {
    return req->directives;
}

const char* celix_requirement_getDirective(const celix_requirement_t* req, const char* key) {
    return celix_properties_get(req->directives, key, NULL);
}

const celix_properties_t* celix_requirement_getAttributes(const celix_requirement_t* req) {
    return req->attributes;
}

const char* celix_requirement_getAttribute(const celix_requirement_t* req, const char* key) {
    return celix_properties_get(req->attributes, key, NULL);
}

void celix_requirement_addDirective(celix_requirement_t* req, const char* key, const char* value) {
    celix_properties_set(req->directives, key, value);
}

void celix_requirement_addAttribute(celix_requirement_t* req, const char* key, const char* value) {
    celix_properties_set(req->attributes, key, value);
}

void celix_requirement_addDirectives(celix_requirement_t* req, const celix_properties_t* directives) {
    CELIX_PROPERTIES_ITERATE(directives, visit) {
        celix_requirement_addDirective(req, visit.key, visit.entry.value);
    }
}

void celix_requirement_addAttributes(celix_requirement_t* req, const celix_properties_t* attributes) {
    CELIX_PROPERTIES_ITERATE(attributes, visit) {
        celix_requirement_addAttribute(req, visit.key, visit.entry.value);
    }
}
