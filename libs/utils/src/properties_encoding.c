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

static celix_status_t
celix_properties_decodeValue(celix_properties_t* props, const char* key, json_t* jsonValue, int flags);

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
        celix_err_pushf("Invalid array list element type %d", elType);
        return CELIX_ILLEGAL_ARGUMENT;
        // LCOV_EXCL_STOP
    }
    if (!*out) {
        celix_err_push("Failed to create json value");
        return CELIX_ENOMEM;
    }
    return CELIX_SUCCESS;
}

static celix_status_t celix_properties_arrayEntryValueToJson(const char* key,
                                                             const celix_properties_entry_t* entry,
                                                             int flags,
                                                             json_t** out) {
    *out = NULL;
    if (celix_arrayList_size(entry->typed.arrayValue) == 0) {
        if (flags & CELIX_PROPERTIES_ENCODE_ERROR_ON_EMPTY_ARRAYS) {
            celix_err_pushf("Invalid empty array for key %s", key);
            return CELIX_ILLEGAL_ARGUMENT;
        }
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

static celix_status_t
celix_properties_entryValueToJson(const char* key, const celix_properties_entry_t* entry, int flags, json_t** out) {
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
            celix_err_pushf("Invalid NaN or Inf in key '%s'.", key);
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
        return celix_properties_arrayEntryValueToJson(key, entry, flags, out);
    default:
        // LCOV_EXCL_START
        celix_err_pushf("Invalid properties entry type %d.", entry->valueType);
        return CELIX_ILLEGAL_ARGUMENT;
        // LCOV_EXCL_STOP
    }

    if (!*out) {
        celix_err_pushf("Failed to create json value for key '%s'.", key);
        return CELIX_ENOMEM;
    }
    return CELIX_SUCCESS;
}

static celix_status_t celix_properties_addJsonValueToJson(json_t* value, const char* key, json_t* obj, int flags) {
    if (!value) {
        // ignore unset values
        return CELIX_SUCCESS;
    }

    json_t* field = json_object_get(obj, key);
    if (field) {
        if (flags & CELIX_PROPERTIES_ENCODE_ERROR_ON_COLLISIONS) {
            celix_err_pushf("Invalid key collision. key '%s' already exists.", key);
            return CELIX_ILLEGAL_ARGUMENT;
        }
    }

    int rc = json_object_set_new(obj, key, value);
    if (rc != 0) {
        celix_err_push("Failed to set json object");
        return CELIX_ENOMEM;
    }
    return CELIX_SUCCESS;
}

static celix_status_t celix_properties_addPropertiesEntryFlatToJson(const celix_properties_entry_t* entry,
                                                                    const char* key,
                                                                    json_t* root,
                                                                    int flags) {
    json_t* value;
    celix_status_t status = celix_properties_entryValueToJson(key, entry, flags, &value);
    if (status != CELIX_SUCCESS) {
        return status;
    }
    return celix_properties_addJsonValueToJson(value, key, root, flags);
}

static celix_status_t celix_properties_addPropertiesEntryToJson(const celix_properties_entry_t* entry,
                                                                const char* key,
                                                                json_t* root,
                                                                int flags) {
    json_t* jsonObj = root;
    const char* fieldName = key;
    const char* slash = strstr(key, "/");
    while (slash) {
        char buf[64];
        char* name = celix_utils_writeOrCreateString(buf, sizeof(buf), "%.*s", (int)(slash - fieldName), fieldName);
        celix_auto(celix_utils_string_guard_t) strGuard = celix_utils_stringGuard_init(buf, name);
        if (!name) {
            celix_err_push("Failed to create name string");
            return CELIX_ENOMEM;
        }
        json_t* subObj = json_object_get(jsonObj, name);
        if (!subObj || !json_is_object(subObj)) {
            if (!json_is_object(subObj) && flags & CELIX_PROPERTIES_ENCODE_ERROR_ON_COLLISIONS) {
                celix_err_pushf("Invalid key collision. Key '%s' already exists.", name);
                return CELIX_ILLEGAL_ARGUMENT;
            }
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
        }

        jsonObj = subObj;
        fieldName = slash + 1;
        slash = strstr(fieldName, "/");
    }

    json_t* value;
    celix_status_t status = celix_properties_entryValueToJson(fieldName, entry, flags, &value);
    if (status != CELIX_SUCCESS) {
        return status;
    }
    return celix_properties_addJsonValueToJson(value, fieldName, jsonObj, flags);
}

celix_status_t celix_properties_saveToStream(const celix_properties_t* properties, FILE* stream, int encodeFlags) {
    json_t* root = json_object();
    if (!root) {
        celix_err_push("Failed to create json object");
    }

    if (!(encodeFlags & CELIX_PROPERTIES_ENCODE_FLAT) && !(encodeFlags & CELIX_PROPERTIES_ENCODE_NESTED)) {
        //no encoding flags set, default to flat
        encodeFlags |= CELIX_PROPERTIES_ENCODE_FLAT;
    }

    CELIX_PROPERTIES_ITERATE(properties, iter) {
        celix_status_t status;
        if (encodeFlags & CELIX_PROPERTIES_ENCODE_FLAT) {
            status = celix_properties_addPropertiesEntryFlatToJson(&iter.entry, iter.key, root, encodeFlags);
        } else {
            assert(encodeFlags & CELIX_PROPERTIES_ENCODE_NESTED);
            status = celix_properties_addPropertiesEntryToJson(&iter.entry, iter.key, root, encodeFlags);
        }
        if (status != CELIX_SUCCESS) {
            json_decref(root);
            return status;
        }
    }

    size_t jsonFlags = JSON_COMPACT;
    if (encodeFlags & CELIX_PROPERTIES_ENCODE_PRETTY) {
        jsonFlags = JSON_INDENT(2);
    }

    int rc = json_dumpf(root, stream, jsonFlags);
    json_decref(root);
    if (rc != 0) {
        celix_err_push("Failed to dump json object");
        return CELIX_ENOMEM;
    }
    return CELIX_SUCCESS;
}

celix_status_t celix_properties_save(const celix_properties_t* properties, const char* filename, int encodeFlags) {
    FILE* stream = fopen(filename, "w");
    if (!stream) {
        celix_err_pushf("Failed to open file %s", filename);
        return CELIX_FILE_IO_EXCEPTION;
    }
    celix_status_t status = celix_properties_saveToStream(properties, stream, encodeFlags);
    fclose(stream);
    return status;
}

celix_status_t celix_properties_saveToString(const celix_properties_t* properties, int encodeFlags, char** out) {
    *out = NULL;
    celix_autofree char* buffer = NULL;
    size_t size = 0;
    FILE* stream = open_memstream(&buffer, &size);
    if (!stream) {
        celix_err_push("Failed to open memstream");
        return CELIX_FILE_IO_EXCEPTION;
    }

    celix_status_t status = celix_properties_saveToStream(properties, stream, encodeFlags);
    fclose(stream);
    if (status == CELIX_SUCCESS) {
        *out = celix_steal_ptr(buffer);
    }
    return status;
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
 * If the array is of a mixed type, the element type cannot be determined and a CELIX_ILLEGAL_ARGUMENT is
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
    assert(size > 0); //precondition: size > 0

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
        } else if ((type == JSON_TRUE || type == JSON_FALSE) && json_is_boolean(value)) {
            // bool, ok.
            continue;
        } else if (type == JSON_INTEGER && json_typeof(value) == JSON_REAL) {
            // mixed integer and real, ok but promote to real
            type = JSON_REAL;
            continue;
        } else if (type == JSON_REAL && json_typeof(value) == JSON_INTEGER) {
            // mixed real and integer, ok
            continue;
        } else if (type != json_typeof(value)) {
            return CELIX_ILLEGAL_ARGUMENT;
        } else if (versionType) {
            if (json_typeof(value) != JSON_STRING || !celix_properties_isVersionString(json_string_value(value))) {
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
    case JSON_NULL:
    default:
        return CELIX_ILLEGAL_ARGUMENT;
    }

    return CELIX_SUCCESS;
}

static celix_status_t
celix_properties_decodeArray(celix_properties_t* props, const char* key, const json_t* jsonArray, int flags) {
    celix_array_list_element_type_t elType;
    celix_status_t status = celix_properties_determineArrayType(jsonArray, &elType);
    if (status != CELIX_SUCCESS && (flags & CELIX_PROPERTIES_DECODE_ERROR_ON_MIXED_ARRAYS)) {
        celix_autofree char* arrStr = json_dumps(jsonArray, JSON_ENCODE_ANY);
        celix_err_pushf("Invalid mixed or null array for key '%s': %s", key, arrStr);
        return status;
    } else if (status != CELIX_SUCCESS) {
        //ignore mixed types
        return CELIX_SUCCESS;
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
            status = celix_arrayList_addDouble(array, json_number_value(value));
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
            celix_err_pushf("Invalid array list element type %d for key %s", elType, key);
            return CELIX_ILLEGAL_ARGUMENT;
            // LCOV_EXCL_STOP
        }
        if (status != CELIX_SUCCESS) {
            return status;
        }
    }
    return celix_properties_assignArrayList(props, key, celix_steal_ptr(array));
}

static celix_status_t
celix_properties_decodeValue(celix_properties_t* props, const char* key, json_t* jsonValue, int flags) {
    if (strncmp(key, "", 1) == 0) {
        if (flags & CELIX_PROPERTIES_DECODE_ERROR_ON_EMPTY_KEYS) {
            celix_err_push("Key cannot be empty");
            return CELIX_ILLEGAL_ARGUMENT;
        }
        return CELIX_SUCCESS; // ignore empty keys.
    }

    if (!json_is_object(jsonValue) && celix_properties_hasKey(props, key) &&
        (flags & CELIX_PROPERTIES_DECODE_ERROR_ON_COLLISIONS)) {
        celix_err_pushf("Invalid key collision. Key '%s' already exists.", key);
        return CELIX_ILLEGAL_ARGUMENT;
    }

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
        const char* fieldName;
        json_t* fieldValue;
        json_object_foreach(jsonValue, fieldName, fieldValue) {
            char buf[64];
            char* combinedKey = celix_utils_writeOrCreateString(buf, sizeof(buf), "%s/%s", key, fieldName);
            celix_auto(celix_utils_string_guard_t) strGuard = celix_utils_stringGuard_init(buf, combinedKey);
            if (!combinedKey) {
                celix_err_push("Failed to create sub key");
                return CELIX_ENOMEM;
            }
            status = celix_properties_decodeValue(props, combinedKey, fieldValue, flags);
            if (status != CELIX_SUCCESS) {
                return status;
            }
        }
        return CELIX_SUCCESS;
    } else if (json_is_array(jsonValue) && json_array_size(jsonValue) == 0) {
        if (flags & CELIX_PROPERTIES_DECODE_ERROR_ON_EMPTY_ARRAYS) {
            celix_err_pushf("Invalid empty array for key '%s'", key);
            return CELIX_ILLEGAL_ARGUMENT;
        }
        // ignore empty arrays
        return CELIX_SUCCESS;
    } else if (json_is_array(jsonValue)) {
        status = celix_properties_decodeArray(props, key, jsonValue, flags);
    } else if (json_is_null(jsonValue)) {
        if (flags & CELIX_PROPERTIES_DECODE_ERROR_ON_NULL_VALUES) {
            celix_err_pushf("Invalid null value for key '%s'", key);
            return CELIX_ILLEGAL_ARGUMENT;
        }
        // ignore null values
        return CELIX_SUCCESS;
    } else {
        // LCOV_EXCL_START
        celix_err_pushf("Invalid json value type for key '%s'", key);
        return CELIX_ILLEGAL_ARGUMENT;
        // LCOV_EXCL_STOP
    }
    return status;
}

static celix_status_t celix_properties_decodeFromJson(json_t* obj, int flags, celix_properties_t** out) {
    if (!json_is_object(obj)) {
        celix_err_push("Expected json object");
        return CELIX_ILLEGAL_ARGUMENT;
    }

    celix_autoptr(celix_properties_t) props = celix_properties_create();
    if (!props) {
        return CELIX_ENOMEM;
    }

    const char* key;
    json_t* value;
    json_object_foreach(obj, key, value) {
        celix_status_t status = celix_properties_decodeValue(props, key, value, flags);
        if (status != CELIX_SUCCESS) {
            return status;
        }
    }

    *out = celix_steal_ptr(props);
    return CELIX_SUCCESS;
}

celix_status_t celix_properties_loadFromStream(FILE* stream, int decodeFlags, celix_properties_t** out) {
    json_error_t jsonError;
    size_t jsonFlags = 0;
    if (decodeFlags & CELIX_PROPERTIES_DECODE_ERROR_ON_DUPLICATES) {
        jsonFlags = JSON_REJECT_DUPLICATES;
    }
    json_t* root = json_loadf(stream, jsonFlags, &jsonError);
    if (!root) {
        celix_err_pushf("Failed to parse json: %s", jsonError.text);
        return CELIX_ILLEGAL_ARGUMENT;
    }
    return celix_properties_decodeFromJson(root, decodeFlags, out);
}

celix_status_t celix_properties_load2(const char* filename, int decodeFlags, celix_properties_t** out) {
    FILE* stream = fopen(filename, "r");
    if (!stream) {
        celix_err_pushf("Failed to open file %s", filename);
        return CELIX_FILE_IO_EXCEPTION;
    }
    celix_status_t status = celix_properties_loadFromStream(stream, decodeFlags, out);
    fclose(stream);
    return status;
}

celix_status_t celix_properties_loadFromString2(const char* input, int decodeFlags, celix_properties_t** out) {
    FILE* stream = fmemopen((void*)input, strlen(input), "r");
    if (!stream) {
        celix_err_push("Failed to open memstream");
        return CELIX_FILE_IO_EXCEPTION;
    }
    celix_status_t status = celix_properties_loadFromStream(stream, decodeFlags, out);
    fclose(stream);
    return status;
}
