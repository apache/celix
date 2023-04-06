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
#include "celix_rcm_err_private.h"

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
        celix_rcmErr_push("Failed to create celix_requirement_t. Namespace is NULL or empty.");
        return NULL;
    }

    celix_requirement_t* req = malloc(sizeof(*req));
    if (req == NULL) {
        celix_rcmErr_push("Failed to allocate celix_requirement_t. Out of memory.");
        return NULL;
    }

    req->resource = resource;
    req->ns = celix_utils_strdup(ns);
    req->attributes = celix_properties_create();
    req->directives = celix_properties_create();
    if (req->ns == NULL || req->attributes == NULL || req->directives == NULL) {
        celix_rcmErr_push("Failed to allocate celix_requirement_t fields. Out of memory.");
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

    if (celix_properties_size(req1->attributes) != celix_properties_size(req2->attributes) ||
        celix_properties_size(req1->directives) != celix_properties_size(req2->directives)) {
        return false;
    }

    if (!celix_utils_stringEquals(req1->ns, req2->ns)) {
        return false;
    }

    //compare attributes
    bool equals = true;
    const char* visit;
    CELIX_PROPERTIES_FOR_EACH(req1->attributes, visit) {
        const char* val1 = celix_properties_get(req1->attributes, visit, NULL);
        const char* val2 = celix_properties_get(req2->attributes, visit, NULL);
        if (!celix_utils_stringEquals(val1, val2)) {
            equals = false;
            break;
        }
    }
    if (!equals) {
        return false;
    }

    //compare directives
    CELIX_PROPERTIES_FOR_EACH(req1->directives, visit) {
        const char* val1 = celix_properties_get(req1->directives, visit, NULL);
        const char* val2 = celix_properties_get(req2->directives, visit, NULL);
        if (!celix_utils_stringEquals(val1, val2)) {
            equals = false;
            break;
        }
    }
    return equals;
}

unsigned int celix_requirement_hashCode(const celix_requirement_t* req) {
    unsigned int hash = celix_utils_stringHash(req->ns);
    const char* visit;

    //FIXME order of attributes/directives is not taken into account
    CELIX_PROPERTIES_FOR_EACH(req->attributes, visit) {
        const char* val = celix_properties_get(req->attributes, visit, NULL);
        hash += celix_utils_stringHash(visit);
        hash += celix_utils_stringHash(val);
    }
    CELIX_PROPERTIES_FOR_EACH(req->directives, visit) {
        const char* val = celix_properties_get(req->directives, visit, NULL);
        hash += celix_utils_stringHash(visit);
        hash += celix_utils_stringHash(val);
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
    const char* visit;
    CELIX_PROPERTIES_FOR_EACH(directives, visit) {
        const char* val = celix_properties_get(directives, visit, NULL);
        celix_requirement_addDirective(req, visit, val);
    }
}

void celix_requirement_addAttributes(celix_requirement_t* req, const celix_properties_t* attributes) {
    const char* visit;
    CELIX_PROPERTIES_FOR_EACH(attributes, visit) {
        const char* val = celix_properties_get(attributes, visit, NULL);
        celix_requirement_addAttribute(req, visit, val);
    }
}
