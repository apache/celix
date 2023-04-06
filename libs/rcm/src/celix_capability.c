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
#include "celix_rcm_err_private.h"

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
        celix_rcmErr_push("Failed to create celix_capability_t. Namespace is NULL or empty.");
        return NULL;
    }

    celix_capability_t* cap = malloc(sizeof(*cap));
    if (cap == NULL) {
        celix_rcmErr_push("Failed to allocate celix_capability_t. Out of memory.");
        return NULL;
    }

    cap->resource = resource;
    cap->ns = celix_utils_strdup(ns);
    cap->attributes = celix_properties_create();
    cap->directives = celix_properties_create();
    if (cap->ns == NULL || cap->attributes == NULL || cap->directives == NULL) {
        celix_rcmErr_push("Failed to allocate celix_capability_t fields. Out of memory.");
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

    if (celix_properties_size(cap1->attributes) != celix_properties_size(cap2->attributes) ||
        celix_properties_size(cap1->directives) != celix_properties_size(cap2->directives)) {
        return false;
    }

    if (!celix_utils_stringEquals(cap1->ns, cap2->ns)) {
        return false;
    }

    //compare attributes
    bool equals = true;
    const char* visit;
    CELIX_PROPERTIES_FOR_EACH(cap1->attributes, visit) {
        const char* value1 = celix_properties_get(cap1->attributes, visit, NULL);
        const char* value2 = celix_properties_get(cap2->attributes, visit, NULL);
        if (!celix_utils_stringEquals(value1, value2)) {
            equals = false;
            break;
        }
    }
    if (!equals) {
        return false;
    }
    CELIX_PROPERTIES_FOR_EACH(cap1->directives, visit) {
        const char* value1 = celix_properties_get(cap1->directives, visit, NULL);
        const char* value2 = celix_properties_get(cap2->directives, visit, NULL);
        if (!celix_utils_stringEquals(value1, value2)) {
            equals = false;
            break;
        }
    }
    return equals;
}

unsigned int celix_capability_hashCode(const celix_capability_t* cap) {
    unsigned int hash = celix_utils_stringHash(cap->ns);
    const char* visit;

    //FIXME order of attributes/directives is not taken into account
    CELIX_PROPERTIES_FOR_EACH(cap->attributes, visit) {
        const char* value = celix_properties_get(cap->attributes, visit, NULL);
        hash += celix_utils_stringHash(visit);
        hash += celix_utils_stringHash(value);
    }
    CELIX_PROPERTIES_FOR_EACH(cap->directives, visit) {
        const char* value = celix_properties_get(cap->directives, visit, NULL);
        hash += celix_utils_stringHash(visit);
        hash += celix_utils_stringHash(value);
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
    const char* visit;
    CELIX_PROPERTIES_FOR_EACH(attributes, visit) {
        const char* value = celix_properties_get(attributes, visit, NULL);
        celix_properties_set(cap->attributes, visit, value);
    }
}

void celix_capability_addDirective(celix_capability_t* cap, const char* key, const char* value) {
    celix_properties_set(cap->directives, key, value);
}

void celix_capability_addDirectives(celix_capability_t* cap, const celix_properties_t* directives) {
    const char* visit;
    CELIX_PROPERTIES_FOR_EACH(directives, visit) {
        const char* value = celix_properties_get(directives, visit, NULL);
        celix_properties_set(cap->directives, visit, value);
    }
}
