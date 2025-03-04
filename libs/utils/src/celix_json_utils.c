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

#include "celix_json_utils_private.h"
#include "celix_err.h"
#include "celix_utils.h"
#include "celix_stdlib_cleanup.h"

#include <string.h>
#include <assert.h>

celix_status_t celix_utils_versionToJson(const celix_version_t* version, json_t** out) {
    assert(version != NULL);
    assert(out != NULL);
    *out = NULL;
    celix_autofree char* versionStr = celix_version_toString(version);
    if (!versionStr) {
        celix_err_push("Failed to create version string.");
        return ENOMEM;
    }
    *out = json_sprintf("version<%s>", versionStr);
    if (!*out) {
        celix_err_push("Failed to create json string.");
        return ENOMEM;
    }
    return CELIX_SUCCESS;
}

celix_status_t celix_utils_jsonToVersion(const json_t* json, celix_version_t** out) {
    assert(json != NULL);
    assert(out != NULL);
    *out = NULL;
    if (!celix_utils_isVersionJsonString(json)) {
        celix_err_push("Failed to convert json to version, json is not a string.");
        return CELIX_ILLEGAL_ARGUMENT;
    }
    const char* value = json_string_value(json);
    assert(value != NULL);
    char buf[32];
    char* extracted = celix_utils_writeOrCreateString(buf, sizeof(buf), "%.*s", (int)strlen(value) - 9, value + 8);
    celix_auto(celix_utils_string_guard_t) guard = celix_utils_stringGuard_init(buf, extracted);
    if (!extracted) {
        celix_err_push("Failed to create extracted version string.");
        return ENOMEM;
    }
    celix_status_t status = celix_version_parse(extracted, out);
    if (status != CELIX_SUCCESS) {
        celix_err_push("Failed to parse version string.");
        return status;
    }
    return CELIX_SUCCESS;
}

bool celix_utils_isVersionJsonString(const json_t* string)
{
    if (!json_is_string(string)) {
        return false;
    }
    const char* value = json_string_value(string);
    assert(value != NULL);
    return strncmp(value, "version<", 8) == 0 && value[strlen(value) - 1] == '>';
}

celix_status_t celix_utils_jsonErrorToStatus(enum json_error_code error) {
    switch (error) {
        case json_error_unknown:
            return CELIX_ILLEGAL_STATE;
        case json_error_out_of_memory:
        case json_error_stack_overflow:
            return ENOMEM;
        case json_error_cannot_open_file:
            return CELIX_FILE_IO_EXCEPTION;
        case json_error_invalid_argument:
        case json_error_invalid_utf8:
        case json_error_premature_end_of_input:
        case json_error_end_of_input_expected:
        case json_error_invalid_syntax:
        case json_error_invalid_format:
        case json_error_wrong_type:
        case json_error_null_character:
        case json_error_null_value:
        case json_error_null_byte_in_key:
        case json_error_duplicate_key:
        case json_error_numeric_overflow:
        case json_error_item_not_found:
        case json_error_index_out_of_range:
        default:
            return CELIX_ILLEGAL_ARGUMENT;
    }
}
