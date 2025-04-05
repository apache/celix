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
#include "celix_array_list_encoding.h"
#include "celix_array_list_encoding_private.h"
#include "celix_json_utils_private.h"
#include "celix_err.h"
#include "celix_stdlib_cleanup.h"
#include "celix_stdio_cleanup.h"

#include <jansson.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

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
static celix_status_t celix_arrayList_determineArrayType(const json_t* jsonArray,
                                                          celix_array_list_element_type_t* out) {
    assert(json_array_size(jsonArray) > 0); //precondition: size > 0

    json_t* value;
    int index;
    json_type type = JSON_NULL;
    bool versionType = false;
    json_array_foreach(jsonArray, index, value) {
        if (index == 0) {
            type = json_typeof(value);
            if (type == JSON_STRING && celix_utils_isVersionJsonString(value)) {
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
            if (json_typeof(value) != JSON_STRING || !celix_utils_isVersionJsonString(value)) {
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
            //JSON_NULL, JSON_OBJECT and  JSON_ARRAY
            return CELIX_ILLEGAL_ARGUMENT;
    }

    return CELIX_SUCCESS;
}

celix_status_t celix_arrayList_decodeFromJson(const json_t* jsonArray, int decodeFlags, celix_array_list_t** out) {
    assert(jsonArray != NULL);
    assert(out != NULL);
    *out = NULL;
    if (!json_is_array(jsonArray)) {
        celix_err_push("Expected a json array.");
        return CELIX_ILLEGAL_ARGUMENT;
    }
    if (json_array_size(jsonArray) == 0) {
        if (decodeFlags & CELIX_ARRAY_LIST_DECODE_ERROR_ON_EMPTY_ARRAYS) {
            celix_err_push("Expected a non-empty json array.");
            return CELIX_ILLEGAL_ARGUMENT;
        }
        return CELIX_SUCCESS;//empty array treated as out = NULL
    }
    celix_array_list_element_type_t elType;
    celix_status_t status = celix_arrayList_determineArrayType(jsonArray, &elType);
    if (status != CELIX_SUCCESS && (decodeFlags & CELIX_ARRAY_LIST_DECODE_ERROR_ON_UNSUPPORTED_ARRAYS)) {
        celix_autofree char* arrStr = json_dumps(jsonArray, JSON_ENCODE_ANY);
        celix_err_pushf("Invalid mixed, null, object or multidimensional array: %s.", arrStr);
        return status;
    } else if (status != CELIX_SUCCESS) {
        //ignore mixed types
        return CELIX_SUCCESS;
    }

    celix_array_list_create_options_t opts = CELIX_EMPTY_ARRAY_LIST_CREATE_OPTIONS;
    opts.elementType = elType;
    celix_autoptr(celix_array_list_t) array = celix_arrayList_createWithOptions(&opts);
    if (!array) {
        return ENOMEM;
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
                celix_version_t* v;
                status = celix_utils_jsonToVersion(value, &v);
                status = CELIX_DO_IF(status, celix_arrayList_assignVersion(array, v));
                break;
            }
            default:
                // LCOV_EXCL_START
                celix_err_pushf("Unexpected array list element type %d.", elType);
                return CELIX_ILLEGAL_ARGUMENT;
                // LCOV_EXCL_STOP
        }
        if (status != CELIX_SUCCESS) {
            return status;
        }
    }
    *out = celix_steal_ptr(array);
    return CELIX_SUCCESS;
}

celix_status_t celix_arrayList_loadFromStream(FILE* stream, int decodeFlags, celix_array_list_t** out) {
    if (!stream || !out) {
        celix_err_push("Invalid arguments.");
        return CELIX_ILLEGAL_ARGUMENT;
    }
    json_error_t jsonError;
    size_t jsonFlags = 0;
    json_auto_t* root = json_loadf(stream, jsonFlags, &jsonError);
    if (!root) {
        celix_err_pushf("Failed to parse json from %s:%i:%i: %s.",
                        jsonError.source,
                        jsonError.line,
                        jsonError.column,
                        jsonError.text);
        return celix_utils_jsonErrorToStatus(json_error_code(&jsonError));
    }
    return celix_arrayList_decodeFromJson(root, decodeFlags, out);
}

celix_status_t celix_arrayList_load(const char* filename, int decodeFlags, celix_array_list_t** out) {
    if (!filename || !out) {
        celix_err_push("Invalid arguments.");
        return CELIX_ILLEGAL_ARGUMENT;
    }
    celix_autoptr(FILE) stream = fopen(filename, "r");
    if (!stream) {
        celix_err_pushf("Failed to open file %s.", filename);
        return CELIX_FILE_IO_EXCEPTION;
    }
    return celix_arrayList_loadFromStream(stream, decodeFlags, out);
}

celix_status_t celix_arrayList_loadFromString(const char* input, int decodeFlags, celix_array_list_t** out) {
    if (!input || !out) {
        celix_err_push("Invalid arguments.");
        return CELIX_ILLEGAL_ARGUMENT;
    }
    celix_autoptr(FILE) stream = fmemopen((void*)input, strlen(input), "r");
    if (!stream) {
        celix_err_pushf("Failed to open memstream.");
        return ENOMEM;
    }
    return celix_arrayList_loadFromStream(stream, decodeFlags, out);
}

static celix_status_t celix_arrayList_elementEntryValueToJson(celix_array_list_element_type_t elType,
                                                                    celix_array_list_entry_t entry,
                                                                    int flags,
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
            if (isnan(entry.doubleVal) || isinf(entry.doubleVal)) {
                if (flags & CELIX_ARRAY_LIST_ENCODE_ERROR_ON_NAN_INF) {
                    celix_err_push("Invalid NaN or Inf.");
                    return CELIX_ILLEGAL_ARGUMENT;
                }
                return CELIX_SUCCESS; // ignore NaN and Inf
            }
            *out = json_real(entry.doubleVal);
            break;
        case CELIX_ARRAY_LIST_ELEMENT_TYPE_BOOL:
            *out = json_boolean(entry.boolVal);
            break;
        case CELIX_ARRAY_LIST_ELEMENT_TYPE_VERSION:
            return celix_utils_versionToJson(entry.versionVal, out);
        default:
            // LCOV_EXCL_START
            celix_err_pushf("Invalid array list element type %d.", elType);
            return CELIX_ILLEGAL_ARGUMENT;
            // LCOV_EXCL_STOP
    }
    if (!*out) {
        celix_err_push("Failed to create json value.");
        return ENOMEM;
    }
    return CELIX_SUCCESS;
}

celix_status_t celix_arrayList_encodeToJson(const celix_array_list_t* list, int encodeFlags, json_t** out) {
    assert(list != NULL);
    assert(out != NULL);
    json_auto_t* array = json_array();
    if (!array) {
        celix_err_push("Failed to create json array.");
        return ENOMEM;
    }

    int size = celix_arrayList_size(list);
    celix_array_list_element_type_t elType = celix_arrayList_getElementType(list);
    for (int i = 0; i < size; ++i) {
        celix_array_list_entry_t arrayEntry = celix_arrayList_getEntry(list, i);
        json_t* jsonValue;
        celix_status_t status = celix_arrayList_elementEntryValueToJson(elType, arrayEntry, encodeFlags, &jsonValue);
        if (status != CELIX_SUCCESS) {
            celix_err_pushf("Failed to encode array element(%d).", i);
            return status;
        } else if (!jsonValue) {
            // ignore unset values
        } else {
            int rc = json_array_append_new(array, jsonValue);
            if (rc != 0) {
                celix_err_push("Failed to append json string to array.");
                return ENOMEM;
            }
        }
    }

    if (json_array_size(array) == 0) {
        if (encodeFlags & CELIX_ARRAY_LIST_ENCODE_ERROR_ON_EMPTY_ARRAYS) {
            celix_err_pushf("Invalid empty array.");
            return CELIX_ILLEGAL_ARGUMENT;
        }
    }

    *out = celix_steal_ptr(array);
    return CELIX_SUCCESS;
}

celix_status_t celix_arrayList_saveToStream(const celix_array_list_t* list, int encodeFlags, FILE* stream) {
    if (!list || !stream) {
        celix_err_push("Invalid arguments.");
        return CELIX_ILLEGAL_ARGUMENT;
    }
    json_auto_t* array = NULL;
    celix_status_t status = celix_arrayList_encodeToJson(list, encodeFlags, &array);
    if (status != CELIX_SUCCESS) {
        return status;
    }
    size_t jsonFlags = JSON_COMPACT;
    if (encodeFlags & CELIX_ARRAY_LIST_ENCODE_PRETTY) {
        jsonFlags = JSON_INDENT(2);
    }
    int rc = json_dumpf(array, stream, jsonFlags);
    if (rc != 0) {
        celix_err_push("Failed to write json to stream.");
        return CELIX_FILE_IO_EXCEPTION;
    }
    return CELIX_SUCCESS;
}

celix_status_t celix_arrayList_save(const celix_array_list_t* list, int encodeFlags, const char* filename) {
    if (!list || !filename) {
        celix_err_push("Invalid arguments.");
        return CELIX_ILLEGAL_ARGUMENT;
    }
    FILE* stream = fopen(filename, "w");
    if (!stream) {
        celix_err_pushf("Failed to open file %s.", filename);
        return CELIX_FILE_IO_EXCEPTION;
    }
    celix_status_t status = celix_arrayList_saveToStream(list, encodeFlags, stream);
    int rc = fclose(stream);
    if (rc != 0) {
        celix_err_pushf("Failed to close file %s: %s.", filename, strerror(errno));
        return CELIX_FILE_IO_EXCEPTION;
    }
    return status;
}

celix_status_t celix_arrayList_saveToString(const celix_array_list_t* list,  int encodeFlags, char** out) {
    if (!list || !out) {
        celix_err_push("Invalid arguments.");
        return CELIX_ILLEGAL_ARGUMENT;
    }
    celix_autofree char* buffer = NULL;
    size_t size = 0;
    FILE* stream = open_memstream(&buffer, &size);
    if (!stream) {
        celix_err_push("Failed to open memstream.");
        return ENOMEM;
    }

    celix_status_t status = celix_arrayList_saveToStream(list, encodeFlags, stream);
    (void)fclose(stream);
    if (!buffer || status != CELIX_SUCCESS) {
        celix_err_push("Failed to write json to memstream.");
        if (!buffer || status == CELIX_FILE_IO_EXCEPTION) {
            return ENOMEM; // Using memstream as stream, return ENOMEM instead of CELIX_FILE_IO_EXCEPTION
        }
        return status;
    }
    *out = celix_steal_ptr(buffer);
    return CELIX_SUCCESS;
}