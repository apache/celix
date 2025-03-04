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
#include "celix_properties_private.h"

#include "celix_err.h"
#include "celix_stdlib_cleanup.h"
#include "celix_utils.h"
#include "celix_array_list_encoding_private.h"
#include "celix_json_utils_private.h"

#include <assert.h>
#include <jansson.h>
#include <math.h>
#include <string.h>

#define CELIX_PROPERTIES_JSONPATH_SEPARATOR '.'

static celix_status_t
celix_properties_decodeValue(celix_properties_t* props, const char* key, json_t* jsonValue, int flags);

static celix_status_t celix_properties_arrayEntryValueToJson(const char* key,
                                                             const celix_properties_entry_t* entry,
                                                             int flags,
                                                             json_t** out) {
    *out = NULL;
    int encodeArrayFlags = 0;
    if (flags & CELIX_PROPERTIES_ENCODE_ERROR_ON_EMPTY_ARRAYS) {
        encodeArrayFlags |= CELIX_ARRAY_LIST_ENCODE_ERROR_ON_EMPTY_ARRAYS;
    }
    if (flags & CELIX_PROPERTIES_ENCODE_ERROR_ON_NAN_INF) {
        encodeArrayFlags |= CELIX_ARRAY_LIST_ENCODE_ERROR_ON_NAN_INF;
    }
    json_auto_t* array = NULL;
    celix_status_t status = celix_arrayList_encodeToJson(entry->typed.arrayValue, encodeArrayFlags, &array);
    if (status != CELIX_SUCCESS) {
        celix_err_pushf("Failed to encode array list for key %s.", key);
        return status;
    }

    if (json_array_size(array) == 0) {//allow empty arrays, but treat as unset property
        return CELIX_SUCCESS;
    }

    *out = celix_steal_ptr(array);
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
            if (flags & CELIX_PROPERTIES_ENCODE_ERROR_ON_NAN_INF) {
                celix_err_pushf("Invalid NaN or Inf in key '%s'.", key);
                return CELIX_ILLEGAL_ARGUMENT;
            }
            return CELIX_SUCCESS; // ignore NaN and Inf
        }
        *out = json_real(entry->typed.doubleValue);
        break;
    case CELIX_PROPERTIES_VALUE_TYPE_BOOL:
        *out = json_boolean(entry->typed.boolValue);
        break;
    case CELIX_PROPERTIES_VALUE_TYPE_VERSION:
        return celix_utils_versionToJson(entry->typed.versionValue, out);
    case CELIX_PROPERTIES_VALUE_TYPE_ARRAY_LIST:
        return celix_properties_arrayEntryValueToJson(key, entry, flags, out);
    default:
        // LCOV_EXCL_START
        celix_err_pushf("Unexpected properties entry type %d.", entry->valueType);
        return CELIX_ILLEGAL_ARGUMENT;
        // LCOV_EXCL_STOP
    }

    if (!*out) {
        celix_err_pushf("Failed to create json value for key '%s'.", key);
        return ENOMEM;
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
            json_decref(value);
            return CELIX_ILLEGAL_ARGUMENT;
        }
    }

    int rc = json_object_set_new(obj, key, value);
    if (rc != 0) {
        celix_err_push("Failed to set json object");
        return ENOMEM;
    }
    return CELIX_SUCCESS;
}

static celix_status_t celix_properties_addPropertiesEntryFlatToJson(const celix_properties_entry_t* entry,
                                                                    const char* key,
                                                                    json_t* root,
                                                                    int flags) {
    json_t* value;
    celix_status_t status = celix_properties_entryValueToJson(key, entry, flags, &value);
    status = CELIX_DO_IF(status, celix_properties_addJsonValueToJson(value, key, root, flags));
    return status;
}

static celix_status_t celix_properties_addPropertiesEntryToJson(const celix_properties_entry_t* entry,
                                                                const char* key,
                                                                json_t* root,
                                                                int flags) {
    json_t* jsonObj = root;
    const char* fieldName = key;
    const char* slash = strchr(key, CELIX_PROPERTIES_JSONPATH_SEPARATOR);
    while (slash) {
        char buf[64];
        char* name = celix_utils_writeOrCreateString(buf, sizeof(buf), "%.*s", (int)(slash - fieldName), fieldName);
        celix_auto(celix_utils_string_guard_t) strGuard = celix_utils_stringGuard_init(buf, name);
        if (!name) {
            celix_err_push("Failed to create name string");
            return ENOMEM;
        }
        json_t* subObj = json_object_get(jsonObj, name);
        if (subObj && !json_is_object(subObj)) {
            if (flags & CELIX_PROPERTIES_ENCODE_ERROR_ON_COLLISIONS) {
                celix_err_pushf("Invalid key collision. Key '%s' already exists.", name);
                return CELIX_ILLEGAL_ARGUMENT;
            }
            return CELIX_SUCCESS;
        }
        if (!subObj) {
            subObj = json_object();
            if (!subObj) {
                celix_err_push("Failed to create json object");
                return ENOMEM;
            }
            int rc = json_object_set_new(jsonObj, name, subObj);
            if (rc != 0) {
                celix_err_push("Failed to set json object");
                return ENOMEM;
            }
        }

        jsonObj = subObj;
        fieldName = slash + 1;
        slash = strchr(fieldName, CELIX_PROPERTIES_JSONPATH_SEPARATOR);
    }

    json_t* value;
    celix_status_t status = celix_properties_entryValueToJson(fieldName, entry, flags, &value);
    if (status != CELIX_SUCCESS) {
        return status;
    }
    return celix_properties_addJsonValueToJson(value, fieldName, jsonObj, flags);
}

celix_status_t celix_properties_saveToStream(const celix_properties_t* properties, FILE* stream, int encodeFlags) {
    json_auto_t* root = json_object();
    if (!root) {
        celix_err_push("Failed to create json object");
        return ENOMEM;
    }

    if (!(encodeFlags & CELIX_PROPERTIES_ENCODE_FLAT_STYLE) && !(encodeFlags & CELIX_PROPERTIES_ENCODE_NESTED_STYLE)) {
        //no encoding flags set, default to flat
        encodeFlags |= CELIX_PROPERTIES_ENCODE_FLAT_STYLE;
    }

    CELIX_PROPERTIES_ITERATE(properties, iter) {
        celix_status_t status;
        if (encodeFlags & CELIX_PROPERTIES_ENCODE_FLAT_STYLE) {
            status = celix_properties_addPropertiesEntryFlatToJson(&iter.entry, iter.key, root, encodeFlags);
        } else {
            assert(encodeFlags & CELIX_PROPERTIES_ENCODE_NESTED_STYLE);
            status = celix_properties_addPropertiesEntryToJson(&iter.entry, iter.key, root, encodeFlags);
        }
        if (status != CELIX_SUCCESS) {
            return status;
        }
    }

    size_t jsonFlags = JSON_COMPACT;
    if (encodeFlags & CELIX_PROPERTIES_ENCODE_PRETTY) {
        jsonFlags = JSON_INDENT(2);
    }

    int rc = json_dumpf(root, stream, jsonFlags);
    if (rc != 0) {
        celix_err_push("Failed to dump json object to stream.");
        return CELIX_FILE_IO_EXCEPTION;
    }
    return CELIX_SUCCESS;
}

celix_status_t celix_properties_save(const celix_properties_t* properties, const char* filename, int encodeFlags) {
    FILE* stream = fopen(filename, "w");
    if (!stream) {
        celix_err_pushf("Failed to open file %s.", filename);
        return CELIX_FILE_IO_EXCEPTION;
    }
    celix_status_t status = celix_properties_saveToStream(properties, stream, encodeFlags);
    int rc = fclose(stream);
    if (rc != 0) {
        celix_err_pushf("Failed to close file %s: %s", filename, strerror(errno));
        return CELIX_FILE_IO_EXCEPTION;
    }
    return status;
}

celix_status_t celix_properties_saveToString(const celix_properties_t* properties, int encodeFlags, char** out) {
    *out = NULL;
    celix_autofree char* buffer = NULL;
    size_t size = 0;
    FILE* stream = open_memstream(&buffer, &size);
    if (!stream) {
        celix_err_push("Failed to open memstream.");
        return ENOMEM;
    }

    celix_status_t status = celix_properties_saveToStream(properties, stream, encodeFlags);
    (void)fclose(stream);
    if (!buffer || status != CELIX_SUCCESS) {
        if (!buffer || status == CELIX_FILE_IO_EXCEPTION) {
            return ENOMEM; // Using memstream as stream, return ENOMEM instead of CELIX_FILE_IO_EXCEPTION
        }
        return status;
    }
    *out = celix_steal_ptr(buffer);
    return CELIX_SUCCESS;
}

static celix_status_t
celix_properties_decodeArray(celix_properties_t* props, const char* key, const json_t* jsonArray, int flags) {
    int decodeArrayFlags = 0;
    if (flags & CELIX_PROPERTIES_DECODE_ERROR_ON_EMPTY_ARRAYS) {
        decodeArrayFlags |= CELIX_ARRAY_LIST_DECODE_ERROR_ON_EMPTY_ARRAYS;
    }
    if (flags & CELIX_PROPERTIES_DECODE_ERROR_ON_UNSUPPORTED_ARRAYS) {
        decodeArrayFlags |= CELIX_ARRAY_LIST_DECODE_ERROR_ON_UNSUPPORTED_ARRAYS;
    }
    celix_autoptr(celix_array_list_t) array = NULL;
    celix_status_t status = celix_arrayList_decodeFromJson(jsonArray, decodeArrayFlags, &array);
    if (status != CELIX_SUCCESS) {
        celix_err_pushf("Failed to decode array list for key '%s'.", key);
        return status;
    }
    if (array == NULL) {
        // ignore empty arrays or mixed type arrays
        return CELIX_SUCCESS;
    }
    return celix_properties_assignArrayList(props, key, celix_steal_ptr(array));
}

static celix_status_t
celix_properties_decodeValue(celix_properties_t* props, const char* key, json_t* jsonValue, int flags) {
    if (strncmp(key, "", 1) == 0) {
        if (flags & CELIX_PROPERTIES_DECODE_ERROR_ON_EMPTY_KEYS) {
            celix_err_push("Key cannot be empty.");
            return CELIX_ILLEGAL_ARGUMENT;
        }
    }

    if (!json_is_object(jsonValue) && celix_properties_hasKey(props, key) &&
        (flags & CELIX_PROPERTIES_DECODE_ERROR_ON_COLLISIONS)) {
        celix_err_pushf("Invalid key collision. Key '%s' already exists.", key);
        return CELIX_ILLEGAL_ARGUMENT;
    }

    celix_status_t status = CELIX_SUCCESS;
    if (celix_utils_isVersionJsonString(jsonValue)) {
        celix_version_t* version;
        status = celix_utils_jsonToVersion(jsonValue, &version);
        status = CELIX_DO_IF(status, celix_properties_assignVersion(props, key, version));
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
            char* combinedKey = celix_utils_writeOrCreateString(buf, sizeof(buf), "%s%c%s", key, CELIX_PROPERTIES_JSONPATH_SEPARATOR, fieldName);
            celix_auto(celix_utils_string_guard_t) strGuard = celix_utils_stringGuard_init(buf, combinedKey);
            if (!combinedKey) {
                celix_err_push("Failed to create sub key.");
                return ENOMEM;
            }
            status = celix_properties_decodeValue(props, combinedKey, fieldValue, flags);
            if (status != CELIX_SUCCESS) {
                return status;
            }
        }
        return CELIX_SUCCESS;
    } else if (json_is_array(jsonValue)) {
        status = celix_properties_decodeArray(props, key, jsonValue, flags);
    } else if (json_is_null(jsonValue)) {
        if (flags & CELIX_PROPERTIES_DECODE_ERROR_ON_NULL_VALUES) {
            celix_err_pushf("Invalid null value for key '%s'.", key);
            return CELIX_ILLEGAL_ARGUMENT;
        }
        // ignore null values
        return CELIX_SUCCESS;
    } else {
        // LCOV_EXCL_START
        celix_err_pushf("Unexpected json value type for key '%s'.", key);
        return CELIX_ILLEGAL_ARGUMENT;
        // LCOV_EXCL_STOP
    }
    return status;
}

static celix_status_t celix_properties_decodeFromJson(json_t* obj, int flags, celix_properties_t** out) {
    *out = NULL;
    if (!json_is_object(obj)) {
        celix_err_push("Expected json object.");
        return CELIX_ILLEGAL_ARGUMENT;
    }

    celix_autoptr(celix_properties_t) props = celix_properties_create();
    if (!props) {
        return ENOMEM;
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
    json_auto_t* root = json_loadf(stream, jsonFlags, &jsonError);
    if (!root) {
        celix_err_pushf("Failed to parse json from %s:%i:%i: %s.",
                        jsonError.source,
                        jsonError.line,
                        jsonError.column,
                        jsonError.text);
        return celix_utils_jsonErrorToStatus(json_error_code(&jsonError));
    }
    return celix_properties_decodeFromJson(root, decodeFlags, out);
}

celix_status_t celix_properties_load(const char* filename, int decodeFlags, celix_properties_t** out) {
    FILE* stream = fopen(filename, "r");
    if (!stream) {
        celix_err_pushf("Failed to open file %s.", filename);
        return CELIX_FILE_IO_EXCEPTION;
    }
    celix_status_t status = celix_properties_loadFromStream(stream, decodeFlags, out);
    fclose(stream);
    return status;
}

celix_status_t celix_properties_loadFromString(const char* input, int decodeFlags, celix_properties_t** out) {
    FILE* stream = fmemopen((void*)input, strlen(input), "r");
    if (!stream) {
        celix_err_push("Failed to open memstream.");
        return ENOMEM;
    }
    celix_status_t status = celix_properties_loadFromStream(stream, decodeFlags, out);
    fclose(stream);
    return status;
}
