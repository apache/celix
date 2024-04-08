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
#include "celix_utils.h"

#include <assert.h>
#include <jansson.h>
#include <math.h>
#include <string.h>

static celix_status_t celix_properties_loadValue(celix_properties_t* props, const char* key, const json_t* jsonValue);

// TODO make jansson wrapper header for auto cleanup, wrap json_object_set_new and json_dumpf for error injection

static celix_status_t celix_properties_versionToJson(const celix_version_t* version, json_t** out) {
    celix_autofree char* versionStr = celix_version_toString(version);
    if (!versionStr) {
        celix_err_push("Failed to create version string");
        return CELIX_ENOMEM;
    }
    *out = json_sprintf("celix_version<%s>", versionStr);
    if (!*out) {
        celix_err_push("Failed to create json string");
        return CELIX_ENOMEM;
    }
    return CELIX_SUCCESS;
}

static celix_status_t celix_properties_arrayElementEntryValueToJson(celix_array_list_element_type_t elType,
                                                                    celix_array_list_entry_t entry,
                                                                    json_t** out) {
    *out = NULL;
    switch (elType) {
    case CELIX_ARRAY_LIST_ELEMENT_TYPE_STRING:
        *out = json_string(entry.stringVal);
        break;
    case CELIX_ARRAY_LIST_ELEMENT_TYPE_LONG:
        *out = json_integer(entry.longVal);
        break;
    case CELIX_ARRAY_LIST_ELEMENT_TYPE_DOUBLE:
        *out = json_real(entry.doubleVal);
        break;
    case CELIX_ARRAY_LIST_ELEMENT_TYPE_BOOL:
        *out = json_boolean(entry.boolVal);
        break;
    case CELIX_ARRAY_LIST_ELEMENT_TYPE_VERSION:
        return celix_properties_versionToJson(entry.versionVal, out);
    default:
        // LCOV_EXCL_START
        celix_err_pushf("Unexpected array list element type %d", elType);
        return CELIX_ILLEGAL_ARGUMENT;
        // LCOV_EXCL_STOP
    }
    if (!*out) {
        celix_err_push("Failed to create json value");
        return CELIX_ENOMEM;
    }
    return CELIX_SUCCESS;
}

static celix_status_t celix_properties_arrayEntryValueToJson(const celix_properties_entry_t* entry, json_t** out) {
    *out = NULL;
    if (celix_arrayList_size(entry->typed.arrayValue) == 0) {
        return CELIX_SUCCESS; // empty array -> treat as unset property
    }

    json_t* array = json_array();
    if (!array) {
        celix_err_push("Failed to create json array");
        return CELIX_ENOMEM;
    }

    for (int i = 0; i < celix_arrayList_size(entry->typed.arrayValue); ++i) {
        celix_array_list_entry_t arrayEntry = celix_arrayList_getEntry(entry->typed.arrayValue, i);
        celix_array_list_element_type_t elType = celix_arrayList_getElementType(entry->typed.arrayValue);
        json_t* jsonValue;
        celix_status_t status = celix_properties_arrayElementEntryValueToJson(elType, arrayEntry, &jsonValue);
        if (status != CELIX_SUCCESS) {
            json_decref(array);
            return status;
        } else if (!jsonValue) {
            // ignore unset values
        } else {
            int rc = json_array_append_new(array, jsonValue);
            if (rc != 0) {
                celix_err_push("Failed to append json string to array");
                json_decref(array);
                return CELIX_ENOMEM;
            }
        }
    }

    *out = array;
    return CELIX_SUCCESS;
}

static celix_status_t celix_properties_entryValueToJson(const celix_properties_entry_t* entry, json_t** out) {
    *out = NULL;
    switch (entry->valueType) {
    case CELIX_PROPERTIES_VALUE_TYPE_STRING:
        *out = json_string(entry->value);
        break;
    case CELIX_PROPERTIES_VALUE_TYPE_LONG:
        *out = json_integer(entry->typed.longValue);
        break;
    case CELIX_PROPERTIES_VALUE_TYPE_DOUBLE:
        if (isnan(entry->typed.doubleValue) || isinf(entry->typed.doubleValue)) {
            celix_err_pushf("Double NaN or Inf not supported in JSON.");
            return CELIX_ILLEGAL_ARGUMENT;
        }
        *out = json_real(entry->typed.doubleValue);
        break;
    case CELIX_PROPERTIES_VALUE_TYPE_BOOL:
        *out = json_boolean(entry->typed.boolValue);
        break;
    case CELIX_PROPERTIES_VALUE_TYPE_VERSION:
        return celix_properties_versionToJson(entry->typed.versionValue, out);
    case CELIX_PROPERTIES_VALUE_TYPE_ARRAY_LIST:
        return celix_properties_arrayEntryValueToJson(entry, out);
    default:
        // LCOV_EXCL_START
        celix_err_pushf("Unexpected properties entry type %d", entry->valueType);
        return CELIX_ILLEGAL_ARGUMENT;
        // LCOV_EXCL_STOP
    }

    if (!*out) {
        celix_err_push("Failed to create json value");
        return CELIX_ENOMEM;
    }
    return CELIX_SUCCESS;
}

static celix_status_t
celix_properties_addEntryToJson(const celix_properties_entry_t* entry, const char* key, json_t* root) {
    json_t* jsonObj = root;
    const char* subKey = key;
    const char* slash = strstr(key, "/");
    while (slash) {
        celix_autofree const char* name = strndup(subKey, slash - subKey);
        if (!name) {
            celix_err_push("Failed to create name string");
            return CELIX_ENOMEM;
        }
        json_t* subObj = json_object_get(jsonObj, name);
        if (!subObj) {
            subObj = json_object();
            if (!subObj) {
                celix_err_push("Failed to create json object");
                return CELIX_ENOMEM;
            }
            int rc = json_object_set_new(jsonObj, name, subObj);
            if (rc != 0) {
                celix_err_push("Failed to set json object");
                return CELIX_ENOMEM;
            }
        } else if (!json_is_object(subObj)) {
            // subObj is not an object, so obj cannot be added -> adding obj flat
            jsonObj = root;
            subKey = key;
            break;
        }

        jsonObj = subObj;
        subKey = slash + 1;
        slash = strstr(subKey, "/");

        json_t* field = json_object_get(jsonObj, subKey);
        if (field) {
            // field already exists, so adding obj flat
            jsonObj = root;
            subKey = key;
            break;
        }
    }

    json_t* value;
    celix_status_t status = celix_properties_entryValueToJson(entry, &value);
    if (status != CELIX_SUCCESS) {
        return status;
    } else if (!value) {
        // ignore unset values
    } else {
        int rc = json_object_set_new(jsonObj, subKey, value);
        if (rc != 0) {
            celix_err_push("Failed to set json object");
            return CELIX_ENOMEM;
        }
    }

    return CELIX_SUCCESS;
}

celix_status_t celix_properties_saveToStream(const celix_properties_t* properties, FILE* stream) {
    json_t* root = json_object();
    if (!root) {
        celix_err_push("Failed to create json object");
    }

    CELIX_PROPERTIES_ITERATE(properties, iter) {
        celix_status_t status = celix_properties_addEntryToJson(&iter.entry, iter.key, root);
        if (status != CELIX_SUCCESS) {
            json_decref(root);
            return status;
        }
    }

    int rc =
        json_dumpf(root,
                   stream,
                   JSON_COMPACT); // TODO make celix properties flags for COMPACT and INDENT and maybe other json flags
    json_decref(root);
    if (rc != 0) {
        celix_err_push("Failed to dump json object");
        return CELIX_ENOMEM; // TODO improve error
    }
    return CELIX_SUCCESS;
}

static celix_version_t* celix_properties_parseVersion(const char* value) {
    // precondition: value is a valid version string (14 chars prefix and 1 char suffix)
    celix_version_t* version = NULL;
    char buf[32];
    char* corrected = celix_utils_writeOrCreateString(buf, sizeof(buf), "%.*s", (int)strlen(value) - 15, value + 14);
    if (!corrected) {
        celix_err_push("Failed to create corrected version string");
        return NULL;
    }
    celix_status_t status = celix_version_parse(corrected, &version);
    celix_utils_freeStringIfNotEqual(buf, corrected);
    if (status != CELIX_SUCCESS) {
        celix_err_push("Failed to parse version string");
        return NULL;
    }
    return version;
}

static bool celix_properties_isVersionString(const char* value) {
    return strncmp(value, "celix_version<", 14) == 0 && value[strlen(value) - 1] == '>';
}

/**
 * @brief Determine the array list element type based on the json value.
 *
 * If the array is empty or of a mixed type, the element type cannot be determined and a CELIX_ILLEGAL_ARGUMENT is
 * returned.
 *
 * @param[in] value The json value.
 * @param[out] out The array list element type.
 * @return CELIX_SUCCESS if the array list element type could be determined or CELIX_ILLEGAL_ARGUMENT if the array
 * type could not be determined.
 */
static celix_status_t celix_properties_determineArrayType(const json_t* jsonArray,
                                                          celix_array_list_element_type_t* out) {
    size_t size = json_array_size(jsonArray);
    if (size == 0) {
        celix_err_push("Empty array");
        return CELIX_ILLEGAL_ARGUMENT;
    }

    json_t* value;
    int index;
    json_type type = JSON_NULL;
    bool versionType = false;
    json_array_foreach(jsonArray, index, value) {
        if (index == 0) {
            type = json_typeof(value);
            if (type == JSON_STRING && celix_properties_isVersionString(json_string_value(value))) {
                versionType = true;
            }
        } else if ((type == JSON_TRUE || type == JSON_FALSE) &&
                   (json_typeof(value) == JSON_TRUE || json_typeof(value) == JSON_FALSE)) {
            // bool, ok.
            continue;
        } else if (type != json_typeof(value)) {
            celix_err_push("Mixed types in array");
            return CELIX_ILLEGAL_ARGUMENT;
        } else if (versionType) {
            if (json_typeof(value) != JSON_STRING || !celix_properties_isVersionString(json_string_value(value))) {
                celix_err_push("Mixed version and non-version strings in array");
                return CELIX_ILLEGAL_ARGUMENT;
            }
        }
    }

    switch (type) {
    case JSON_STRING:
        *out = versionType ? CELIX_ARRAY_LIST_ELEMENT_TYPE_VERSION : CELIX_ARRAY_LIST_ELEMENT_TYPE_STRING;
        break;
    case JSON_INTEGER:
        *out = CELIX_ARRAY_LIST_ELEMENT_TYPE_LONG;
        break;
    case JSON_REAL:
        *out = CELIX_ARRAY_LIST_ELEMENT_TYPE_DOUBLE;
        break;
    case JSON_TRUE:
    case JSON_FALSE:
        *out = CELIX_ARRAY_LIST_ELEMENT_TYPE_BOOL;
        break;
    default:
        celix_err_pushf("Unexpected json array type %d", type);
        return CELIX_ILLEGAL_ARGUMENT;
    }

    return CELIX_SUCCESS;
}

static celix_status_t celix_properties_loadArray(celix_properties_t* props, const char* key, const json_t* jsonArray) {
    celix_array_list_element_type_t elType;
    celix_status_t status = celix_properties_determineArrayType(jsonArray, &elType);
    if (status != CELIX_SUCCESS) {
        return status;
    }

    celix_array_list_create_options_t opts = CELIX_EMPTY_ARRAY_LIST_CREATE_OPTIONS;
    opts.elementType = elType;
    celix_autoptr(celix_array_list_t) array = celix_arrayList_createWithOptions(&opts);
    if (!array) {
        return CELIX_ENOMEM;
    }

    json_t* value;
    int index;
    json_array_foreach(jsonArray, index, value) {
        switch (elType) {
        case CELIX_ARRAY_LIST_ELEMENT_TYPE_STRING:
            status = celix_arrayList_addString(array, json_string_value(value));
            break;
        case CELIX_ARRAY_LIST_ELEMENT_TYPE_LONG:
            status = celix_arrayList_addLong(array, (long)json_integer_value(value));
            break;
        case CELIX_ARRAY_LIST_ELEMENT_TYPE_DOUBLE:
            status = celix_arrayList_addDouble(array, json_real_value(value));
            break;
        case CELIX_ARRAY_LIST_ELEMENT_TYPE_BOOL:
            status = celix_arrayList_addBool(array, json_boolean_value(value));
            break;
        case CELIX_ARRAY_LIST_ELEMENT_TYPE_VERSION: {
            celix_version_t* v = celix_properties_parseVersion(json_string_value(value));
            if (!v) {
                return CELIX_ILLEGAL_ARGUMENT;
            }
            status = celix_arrayList_addVersion(array, v);
            break;
        }
        default:
            // LCOV_EXCL_START
            celix_err_pushf("Unexpected array list element type %d", elType);
            return CELIX_ILLEGAL_ARGUMENT;
            // LCOV_EXCL_STOP
        }
        if (status != CELIX_SUCCESS) {
            return status;
        }
    }
    return celix_properties_assignArrayList(props, key, celix_steal_ptr(array));
}

static celix_status_t celix_properties_loadValue(celix_properties_t* props, const char* key, const json_t* jsonValue) {
    celix_status_t status = CELIX_SUCCESS;
    if (json_is_string(jsonValue) && celix_properties_isVersionString(json_string_value(jsonValue))) {
        celix_version_t* version = celix_properties_parseVersion(json_string_value(jsonValue));
        if (!version) {
            return CELIX_ILLEGAL_ARGUMENT;
        }
        status = celix_properties_setVersion(props, key, version);
    } else if (json_is_string(jsonValue)) {
        status = celix_properties_setString(props, key, json_string_value(jsonValue));
    } else if (json_is_integer(jsonValue)) {
        status = celix_properties_setLong(props, key, json_integer_value(jsonValue));
    } else if (json_is_real(jsonValue)) {
        status = celix_properties_setDouble(props, key, json_real_value(jsonValue));
    } else if (json_is_boolean(jsonValue)) {
        status = celix_properties_setBool(props, key, json_boolean_value(jsonValue));
    } else if (json_is_object(jsonValue)) {
        // TODO
        status = CELIX_ILLEGAL_ARGUMENT;
    } else if (json_is_array(jsonValue)) {
        status = celix_properties_loadArray(props, key, jsonValue);
    } else {
        // LCOV_EXCL_START
        celix_err_pushf("Unexpected json value type");
        return CELIX_ILLEGAL_ARGUMENT;
        // LCOV_EXCL_STOP
    }
    return status;
}

static celix_status_t celix_properties_loadFromJson(json_t* obj, celix_properties_t** out) {
    assert(obj != NULL && json_is_object(obj));
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    if (!props) {
        return CELIX_ENOMEM;
    }

    // add loop (obj=root, prefix="" and extend prefix when going into sub objects)
    const char* key;
    json_t* value;
    json_object_foreach(obj, key, value) {
        if (json_is_object(value)) {
            // TODO
            return CELIX_ILLEGAL_ARGUMENT;
        }
        celix_status_t status = celix_properties_loadValue(props, key, value);
        if (status != CELIX_SUCCESS) {
            return status;
        }
    }

    *out = celix_steal_ptr(props);
    return CELIX_SUCCESS;
}

celix_status_t celix_properties_loadFromStream(FILE* stream, celix_properties_t** out) {
    json_error_t jsonError;
    json_t* root = json_loadf(stream, 0, &jsonError);
    if (!root) {
        celix_err_pushf("Failed to parse json: %s", jsonError.text);
        return CELIX_ILLEGAL_ARGUMENT;
    }

    return celix_properties_loadFromJson(root, out);
}