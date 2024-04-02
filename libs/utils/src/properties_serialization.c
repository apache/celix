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

#include "celix_properties.h"

#include "celix_err.h"
#include "celix_stdlib_cleanup.h"

#include <jansson.h>

//TODO make jansson wrapper header for auto cleanup, wrap json_object_set_new and json_dumpf for error injection

static json_t* celix_properties_versionToJson(const celix_version_t *version) {
    celix_autofree char* versionStr = celix_version_toString(version); // TODO error handling
    return json_sprintf("celix_version<%s>", versionStr);
}

static json_t* celix_properties_arrayElementEntryValueToJson(celix_array_list_element_type_t elType,
                                                             celix_array_list_entry_t entry) {
    switch (elType) {
    case CELIX_ARRAY_LIST_ELEMENT_TYPE_STRING:
        return json_string(entry.stringVal);
    case CELIX_ARRAY_LIST_ELEMENT_TYPE_LONG:
        return json_integer(entry.longVal);
    case CELIX_ARRAY_LIST_ELEMENT_TYPE_DOUBLE:
        return json_real(entry.doubleVal);
    case CELIX_ARRAY_LIST_ELEMENT_TYPE_BOOL:
        return json_boolean(entry.boolVal);
    case CELIX_ARRAY_LIST_ELEMENT_TYPE_VERSION:
        celix_properties_versionToJson(entry.versionVal);
    default:
        // LCOV_EXCL_START
        celix_err_pushf("Unexpected array list element type %d", elType);
        return NULL;
        // LCOV_EXCL_STOP
    }
}

static json_t* celix_properties_arrayEntryValueToJson(const celix_properties_entry_t* entry) {
    json_t* array = json_array();
    if (!array) {
        celix_err_push("Failed to create json array");
        return NULL;
    }

    for (int i = 0; i < celix_arrayList_size(entry->typed.arrayValue); ++i) {
        celix_array_list_entry_t arrayEntry = celix_arrayList_getEntry(entry->typed.arrayValue, i);
        celix_array_list_element_type_t elType = celix_arrayList_getElementType(entry->typed.arrayValue);
        json_t* jsonValue = celix_properties_arrayElementEntryValueToJson(elType, arrayEntry);
        if (!jsonValue) {
            celix_err_push("Failed to create json string");
            json_decref(array);
            return NULL;
        }
        int rc = json_array_append_new(array, jsonValue);
        if (rc != 0) {
            celix_err_push("Failed to append json string to array");
            json_decref(array);
            return NULL;
        }
    }
    return array;
}

static json_t* celix_properties_entryValueToJson(const celix_properties_entry_t* entry) {
    switch (entry->valueType) {
        case CELIX_PROPERTIES_VALUE_TYPE_STRING:
            return json_string(entry->value);
        case CELIX_PROPERTIES_VALUE_TYPE_LONG:
            return json_integer(entry->typed.longValue);
        case CELIX_PROPERTIES_VALUE_TYPE_DOUBLE:
            return json_real(entry->typed.doubleValue);
        case CELIX_PROPERTIES_VALUE_TYPE_BOOL:
            return json_boolean(entry->typed.boolValue);
        case CELIX_PROPERTIES_VALUE_TYPE_VERSION:
            return celix_properties_versionToJson(entry->typed.versionValue);
        case CELIX_PROPERTIES_VALUE_TYPE_ARRAY_LIST:
            return celix_properties_arrayEntryValueToJson(entry);
        default:
            //LCOV_EXCL_START
            celix_err_pushf("Unexpected properties entry type %d", entry->valueType);\
            return NULL;
            //LCOV_EXCL_STOP
    }
}

celix_status_t celix_properties_saveToStream(const celix_properties_t* properties, FILE* stream) {
    json_t* root = json_object();
    if (!root) {
        celix_err_push("Failed to create json object");
    }

    CELIX_PROPERTIES_ITERATE(properties, iter) {
        const char* key = iter.key;
        json_t* value = celix_properties_entryValueToJson(&iter.entry);
        if (!value) {
            json_decref(root);
            return CELIX_ENOMEM; //TODO improve error
        }
        int rc = json_object_set_new(root, key, value);
        if (rc != 0) {
            celix_err_push("Failed to set json object");
            json_decref(root);
            return CELIX_ENOMEM; //TODO improve error
        }
    }

    int rc = json_dumpf(root, stream, JSON_COMPACT); //TODO make celix properties flags for COMPACT and INDENT and maybe other json flags
    json_decref(root);
    if (rc != 0) {
        celix_err_push("Failed to dump json object");
        return CELIX_ENOMEM; //TODO improve error
    }
    return CELIX_SUCCESS;
}

celix_status_t celix_properties_loadFromStream(FILE* stream, celix_properties_t** out) {
    celix_status_t status = CELIX_SUCCESS;
    (void)stream;
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    *out = celix_steal_ptr(props);
    return status;
}