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
#include "celix_capability.h"
#include "celix_err.h"

struct celix_capability {
    const celix_resource_t* resource; //weak ref
    char* ns;
    celix_properties_t* attributes;
    celix_properties_t* directives;
};

celix_capability_t* celix_capability_create(
        const celix_resource_t* resource,
        const char* ns) {

    if (celix_utils_isStringNullOrEmpty(ns)) {
        celix_err_push("Failed to create celix_capability_t. Namespace is NULL or empty.");
        return NULL;
    }

    celix_capability_t* cap = malloc(sizeof(*cap));
    if (cap == NULL) {
        celix_err_push("Failed to allocate celix_capability_t. Out of memory.");
        return NULL;
    }

    cap->resource = resource;
    cap->ns = celix_utils_strdup(ns);
    cap->attributes = celix_properties_create();
    cap->directives = celix_properties_create();
    if (cap->ns == NULL || cap->attributes == NULL || cap->directives == NULL) {
        celix_err_push("Failed to allocate celix_capability_t fields. Out of memory.");
        celix_capability_destroy(cap);
        return NULL;
    }

    return cap;
}

void celix_capability_destroy(celix_capability_t* capability) {
    if (capability != NULL) {
        free(capability->ns);
        celix_properties_destroy(capability->attributes);
        celix_properties_destroy(capability->directives);
        free(capability);
    }
}

bool celix_capability_equals(const celix_capability_t* cap1, const celix_capability_t* cap2) {
    if (cap1 == cap2) {
        return true;
    }

    return celix_utils_stringEquals(cap1->ns, cap2->ns) &&
           celix_properties_equals(cap1->attributes, cap2->attributes) &&
           celix_properties_equals(cap1->directives, cap2->directives);
}

unsigned int celix_capability_hashCode(const celix_capability_t* cap) {
    unsigned int hash = celix_utils_stringHash(cap->ns);
    CELIX_PROPERTIES_ITERATE(cap->attributes, visit) {
        hash += celix_utils_stringHash(visit.key);
        hash += celix_utils_stringHash(visit.entry.value);
    }
    CELIX_PROPERTIES_ITERATE(cap->directives, visit) {
        hash += celix_utils_stringHash(visit.key);
        hash += celix_utils_stringHash(visit.entry.value);
    }
    return hash;
}

const celix_resource_t* celix_capability_getResource(const celix_capability_t* cap) {
    return cap->resource;
}

const char* celix_capability_getNamespace(const celix_capability_t* cap) {
    return cap->ns;
}

const celix_properties_t* celix_capability_getAttributes(const celix_capability_t* cap) {
    return cap->attributes;
}

const celix_properties_t* celix_capability_getDirectives(const celix_capability_t* cap) {
    return cap->directives;
}

const char* celix_capability_getAttribute(const celix_capability_t* cap, const char* key) {
    return celix_properties_get(cap->attributes, key, NULL);
}

const char* celix_capability_getDirective(const celix_capability_t* cap, const char* key) {
    return celix_properties_get(cap->directives, key, NULL);
}

void celix_capability_addAttribute(celix_capability_t* cap, const char* key, const char* value) {
    celix_properties_set(cap->attributes, key, value);
}

void celix_capability_addAttributes(celix_capability_t* cap, const celix_properties_t* attributes) {
    CELIX_PROPERTIES_ITERATE(attributes, visit) {
        celix_properties_set(cap->attributes, visit.key, visit.entry.value);
    }
}

void celix_capability_addDirective(celix_capability_t* cap, const char* key, const char* value) {
    celix_properties_set(cap->directives, key, value);
}

void celix_capability_addDirectives(celix_capability_t* cap, const celix_properties_t* directives) {
    CELIX_PROPERTIES_ITERATE(directives, visit) {
        celix_properties_set(cap->directives, visit.key, visit.entry.value);
    }
}
