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

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils.h>

#include "celix_convert_utils.h"
#include "celix_err.h"
#include "celix_errno.h"
#include "celix_filter.h"
#include "celix_stdio_cleanup.h"
#include "celix_stdlib_cleanup.h"
#include "celix_version.h"
#include "filter.h"

// ignoring clang-tidy recursion warnings for this file, because filter uses recursion
// NOLINTBEGIN(misc-no-recursion)

struct celix_filter_internal {
    bool convertedToLong;
    long longValue;

    bool convertedToDouble;
    double doubleValue;

    bool convertedToBool;
    bool boolValue;

    bool convertedToVersion;
    celix_version_t* versionValue;
};

static void celix_filter_skipWhiteSpace(const char* filterString, int* pos);
static celix_filter_t* celix_filter_parseFilter(const char* filterString, int* pos);
static celix_filter_t* celix_filter_parseFilterNode(const char* filterString, int* pos);
static celix_filter_t* celix_filter_parseAndOrOr(const char* filterString, celix_filter_operand_t andOrOr, int* pos);
static celix_filter_t* celix_filter_parseNot(const char* filterString, int* pos);
static celix_filter_t* celix_filter_parseItem(const char* filterString, int* pos);
static char* celix_filter_parseAttributeOrValue(const char* filterString, int* pos, bool parseAttribute);
static celix_array_list_t* celix_filter_parseSubstring(const char* filterString, int* pos);
static bool celix_filter_isSubString(const char* filterString, int startPos);
static bool celix_filter_isNextNonWhiteSpaceChar(const char* filterString, int pos, char c);

static void celix_filter_skipWhiteSpace(const char* filterString, int* pos) {
    for (; filterString[*pos] != '\0' && isspace(filterString[*pos]);) {
        (*pos)++;
    }
}

static bool celix_filter_isNextNonWhiteSpaceChar(const char* filterString, int pos, char c) {
    for (int i = pos; filterString[i] != '\0'; i++) {
        if (!isspace(filterString[i])) {
            return filterString[i] == c;
        }
    }
    return false;
}

celix_filter_t* filter_create(const char* filterString) { return celix_filter_create(filterString); }

void filter_destroy(celix_filter_t* filter) { return celix_filter_destroy(filter); }

static celix_filter_t* celix_filter_parseFilter(const char* filterString, int* pos) {
    celix_filter_skipWhiteSpace(filterString, pos);
    if (filterString[*pos] != '(') {
        celix_err_pushf("Filter Error: Missing '(' in filter string '%s'.", filterString);
        return NULL;
    }
    (*pos)++; // eat '('

    celix_autoptr(celix_filter_t) filter = celix_filter_parseFilterNode(filterString, pos);
    if (!filter) {
        return NULL;
    }

    celix_filter_skipWhiteSpace(filterString, pos);

    if (filterString[*pos] != ')') {
        celix_err_pushf("Filter Error: Missing ')' in filter string '%s'.", filterString);
        return NULL;
    }
    (*pos)++; // eat ')'
    celix_filter_skipWhiteSpace(filterString, pos);

    return celix_steal_ptr(filter);
}

static celix_filter_t* celix_filter_parseFilterNode(const char* filterString, int* pos) {
    char c;
    celix_filter_skipWhiteSpace(filterString, pos);

    c = filterString[*pos];

    switch (c) {
    case '&': {
        (*pos)++;
        return celix_filter_parseAndOrOr(filterString, CELIX_FILTER_OPERAND_AND, pos);
    }
    case '|': {
        (*pos)++;
        return celix_filter_parseAndOrOr(filterString, CELIX_FILTER_OPERAND_OR, pos);
    }
    case '!': {
        (*pos)++;
        return celix_filter_parseNot(filterString, pos);
    }
    default: {
        return celix_filter_parseItem(filterString, pos);
    }
    }
}

static celix_filter_t* celix_filter_parseAndOrOr(const char* filterString, celix_filter_operand_t andOrOr, int* pos) {
    celix_filter_skipWhiteSpace(filterString, pos);

    celix_autoptr(celix_filter_t) filter = (celix_filter_t*)calloc(1, sizeof(*filter));
    if (!filter) {
        celix_err_push("Filter Error: Failed to allocate memory.");
        return NULL;
    }

    celix_array_list_create_options_t ops = CELIX_EMPTY_ARRAY_LIST_CREATE_OPTIONS;
    ops.simpleRemovedCallback = (void (*)(void*))celix_filter_destroy;
    celix_autoptr(celix_array_list_t) children = celix_arrayList_createWithOptions(&ops);
    if (!children) {
        celix_err_push("Filter Error: Failed to allocate memory.");
        return NULL;
    }

    if (filterString[*pos] == ')') {
        // empty and/or filter
    } else if (filterString[*pos] != '(') {
        celix_err_push("Filter Error: Missing '('.");
        return NULL;
    } else {
        while (filterString[*pos] == '(') {
            celix_filter_t* child = celix_filter_parseFilter(filterString, pos);
            if (child == NULL) {
                return NULL;
            }
            if (celix_arrayList_add(children, child) != CELIX_SUCCESS) {
                celix_err_push("Filter Error: Failed to add filter node.");
                celix_filter_destroy(child);
                return NULL;
            }
        }
    }

    filter->operand = andOrOr;
    filter->children = celix_steal_ptr(children);

    return celix_steal_ptr(filter);
}

static celix_filter_t* celix_filter_parseNot(const char* filterString, int* pos) {
    celix_filter_skipWhiteSpace(filterString, pos);

    char c = filterString[*pos];
    if (c != '(') {
        celix_err_push("Filter Error: Missing '('.");
        return NULL;
    }

    celix_autoptr(celix_filter_t) child = celix_filter_parseFilter(filterString, pos);
    if (!child) {
        return NULL;
    }

    celix_array_list_create_options_t ops = CELIX_EMPTY_ARRAY_LIST_CREATE_OPTIONS;
    ops.simpleRemovedCallback = (void (*)(void*))celix_filter_destroy;
    celix_autoptr(celix_array_list_t) children = celix_arrayList_createWithOptions(&ops);
    if (!children) {
        celix_err_push("Filter Error: Failed to allocate memory.");
        return NULL;
    }
    if (celix_arrayList_add(children, child) != CELIX_SUCCESS) {
        celix_err_push("Filter Error: Failed to add NOT filter node.");
        return NULL;
    }
    child = NULL;

    celix_filter_t* filter = (celix_filter_t*)calloc(1, sizeof(*filter));
    if (!filter) {
        celix_err_push("Filter Error: Failed to allocate memory.");
        return NULL;
    }

    filter->operand = CELIX_FILTER_OPERAND_NOT;
    filter->children = celix_steal_ptr(children);
    return filter;
}

static celix_filter_t* celix_filter_parseItem(const char* filterString, int* pos) {
    celix_autofree char* attr = celix_filter_parseAttributeOrValue(filterString, pos, true);
    if (!attr || strlen(attr) == 0) {
        celix_err_push("Filter Error: Missing attr.");
        return NULL;
    }

    celix_autoptr(celix_filter_t) filter = calloc(1, sizeof(*filter));
    if (!filter) {
        celix_err_push("Filter Error: Failed to allocate memory.");
        return NULL;
    }
    filter->attribute = celix_steal_ptr(attr);

    celix_filter_skipWhiteSpace(filterString, pos);
    char op = filterString[*pos];
    switch (op) {
    case '~': {
        char secondOp = filterString[*pos + 1];
        if (secondOp == '=') {
            *pos += 2;
            filter->operand = CELIX_FILTER_OPERAND_APPROX;
            filter->value = celix_filter_parseAttributeOrValue(filterString, pos, false);
            if (!filter->value) {
                return NULL;
            }
            break;
        }
        celix_err_pushf("Filter Error: Invalid operand char after ~. Expected `=` got `%c`", secondOp);
        return NULL;
    }
    case '>': {
        if (filterString[*pos + 1] == '=') {
            *pos += 2;
            filter->operand = CELIX_FILTER_OPERAND_GREATEREQUAL;
        } else {
            *pos += 1;
            filter->operand = CELIX_FILTER_OPERAND_GREATER;
        }
        filter->value = celix_filter_parseAttributeOrValue(filterString, pos, false);
        if (!filter->value) {
            return NULL;
        }
        break;
    }
    case '<': {
        if (filterString[*pos + 1] == '=') {
            *pos += 2;
            filter->operand = CELIX_FILTER_OPERAND_LESSEQUAL;
        } else {
            *pos += 1;
            filter->operand = CELIX_FILTER_OPERAND_LESS;
        }
        filter->value = celix_filter_parseAttributeOrValue(filterString, pos, false);
        if (!filter->value) {
            return NULL;
        }
        break;
    }
    case '=': {
        if (filterString[*pos + 1] == '*') {
            int oldPos = *pos;
            *pos += 2;
            celix_filter_skipWhiteSpace(filterString, pos);
            if (filterString[*pos] == ')') {
                filter->operand = CELIX_FILTER_OPERAND_PRESENT;
                filter->value = NULL;
                break;
            }
            *pos = oldPos;
        }
        (*pos)++;
        if (celix_filter_isSubString(filterString, *pos)) {
            celix_array_list_t* subs = celix_filter_parseSubstring(filterString, pos);
            if (!subs) {
                return NULL;
            }
            filter->operand = CELIX_FILTER_OPERAND_SUBSTRING;
            filter->children = subs;
        } else {
            filter->operand = CELIX_FILTER_OPERAND_EQUAL;
            filter->value = celix_filter_parseAttributeOrValue(filterString, pos, false);
            if (!filter->value) {
                return NULL;
            }
        }
        break;
    }
    default: {
        celix_err_pushf("Filter Error: Invalid operand char `%c`", op);
        return NULL;
    }
    }
    return celix_steal_ptr(filter);
}

static char* celix_filter_parseAttributeOrValue(const char* filterString, int* pos, bool parseAttribute) {
    const char* name = parseAttribute ? "attribute" : "attribute value";
    celix_autofree char* value = NULL;
    size_t valueSize = 0;
    celix_autoptr(FILE) stream = NULL;

    bool keepRunning = true;
    while (keepRunning) {
        char c = filterString[*pos];

        switch (c) {
        case ')': {
            if (parseAttribute) {
                celix_err_pushf("Filter Error: Unexpected `)` char while parsing %s.", name);
                return NULL;
            }
            keepRunning = false;
            break;
        }
        case '(': {
            celix_err_pushf("Filter Error: Unexpected `(` char while parsing %s.", name);
            return NULL;
        }
        case '\0': {
            celix_err_pushf("Filter Error: Unexpected end of string while parsing %s.", name);
            return NULL;
        }
        case '<':
        case '>':
        case '=':
        case '~': {
            if (parseAttribute) {
                keepRunning = false; // done for attribute, valid for value
                break;
            }
            /* no break */
        }
        default: {
            if (c == '\\') {
                if (filterString[*pos + 1] == '\0') {
                    celix_err_pushf("Filter Error: Unexpected end of string while parsing %s.", name);
                    return NULL;
                }
                (*pos)++; // eat '\'
                c = filterString[*pos];
            }
            if (!stream) {
                stream = open_memstream(&value, &valueSize);
                if (!stream) {
                    celix_err_push("Filter Error: Failed to open mem stream.");
                    return NULL;
                }
            }
            int rc = fputc(c, stream);
            if (rc == EOF) {
                celix_err_push("Filter Error: Failed to write to stream.");
                return NULL;
            }
            (*pos)++;
            break;
        }
        }
    }

    if (!stream) {
        // empty value
        value = celix_utils_strdup("");
        if (!value) {
            celix_err_push("Filter Error: Failed to allocate memory.");
            return NULL;
        }
        return celix_steal_ptr(value);
    }

    int rc = fclose(celix_steal_ptr(stream));
    if (rc != 0) {
        celix_err_push("Filter Error: Failed to close stream.");
        return NULL;
    }

    return celix_steal_ptr(value);
}

static bool celix_filter_isSubString(const char* filterString, int startPos) {
    for (int i = startPos; filterString[i] != '\0' && filterString[i] != ')'; i++) {
        if (filterString[i] == '*') {
            return true;
        }
    }
    return false;
}

static celix_status_t celix_filter_parseSubstringAny(const char* filterString, int* pos, char** out) {
    celix_autofree char* any = NULL;
    size_t anySize = 0;
    celix_autoptr(FILE) stream = NULL;
    while (filterString[*pos] != ')' && filterString[*pos] != '*') {
        int rc;
        if (filterString[*pos] == '\\') {
            (*pos)++; // eat '\'
        }
        if (filterString[*pos] == '\0') {
            celix_err_push("Filter Error: Unexpected end of string while parsing attribute value.");
            return CELIX_INVALID_SYNTAX;
        }
        if (!stream) {
            stream = open_memstream(&any, &anySize);
            if (!stream) {
                celix_err_push("Filter Error: Failed to open mem stream.");
                return CELIX_ENOMEM;
            }
        }
        rc = fputc(filterString[*pos], stream);
        if (rc == EOF) {
            celix_err_push("Filter Error: Failed to write to stream.\n");
            return CELIX_ENOMEM;
        }
        (*pos)++;
    }

    if (!stream) {
        // empty any
        *out = NULL;
        return CELIX_SUCCESS;
    }

    int rc = fclose(celix_steal_ptr(stream));
    if (rc != 0) {
        celix_err_push("Filter Error: Failed to close stream.");
        return CELIX_FILE_IO_EXCEPTION;
    }

    *out = celix_steal_ptr(any);
    return CELIX_SUCCESS;
}

/**
 * @brief Parses a substring filter.
 * Returns a array list with at least 2 elements: The first element is the initial element, the last element is the
 * final element. The initial and final element can be NULL.
 *
 * e.g.:
 * - (foo=bar*) -> [bar, NULL]
 * - (foo=*bar) -> [NULL, bar]
 * - (foo=*bar*) -> [NULL, bar, NULL]
 * - (foo=bar*bar) -> [bar, bar]
 * - (foo=bar*bar*) -> [bar, bar, NULL]
 */
static celix_array_list_t* celix_filter_parseSubstring(const char* filterString, int* pos) {
    celix_autoptr(celix_array_list_t) subs = celix_arrayList_createStringArray();
    if (!subs) {
        celix_err_push("Filter Error: Failed to allocate memory.");
        return NULL;
    }

    if (filterString[*pos] == '*') {
        // initial substring is NULL
        // eat '*'
        (*pos)++;
        if (celix_arrayList_addString(subs, "") != CELIX_SUCCESS) {
            celix_err_push("Filter Error: Failed to add element to array list.");
            return NULL;
        }
    }

    char* element = NULL;
    celix_status_t status;
    do {
        status = celix_filter_parseSubstringAny(filterString, pos, &element);
        if (status != CELIX_SUCCESS) {
            return NULL;
        }
        if (element) {
            if (celix_arrayList_assignString(subs, element) != CELIX_SUCCESS) {
                celix_err_push("Filter Error: Failed to add element to array list.");
                return NULL;
            }
        }
        if (filterString[*pos] == '*') {
            // final substring is NULL
            (*pos)++; // eat '*'
            if (celix_filter_isNextNonWhiteSpaceChar(filterString, *pos, ')')) {
                if (celix_arrayList_addString(subs, "") != CELIX_SUCCESS) {
                    celix_err_push("Filter Error: Failed to add element to array list.");
                    return NULL;
                }
                break;
            }
        }
    } while (element);

    celix_filter_skipWhiteSpace(filterString, pos);
    return celix_steal_ptr(subs);
}

static bool celix_filter_isCompareOperand(celix_filter_operand_t operand) {
    return operand == CELIX_FILTER_OPERAND_EQUAL || operand == CELIX_FILTER_OPERAND_GREATER ||
           operand == CELIX_FILTER_OPERAND_LESS || operand == CELIX_FILTER_OPERAND_GREATEREQUAL ||
           operand == CELIX_FILTER_OPERAND_LESSEQUAL;
}

static bool celix_filter_hasFilterChildren(celix_filter_t* filter) {
    return filter->operand == CELIX_FILTER_OPERAND_AND || filter->operand == CELIX_FILTER_OPERAND_OR ||
           filter->operand == CELIX_FILTER_OPERAND_NOT;
}

static void celix_filter_destroyInternal(celix_filter_internal_t* internal) {
    if (internal) {
        celix_version_destroy(internal->versionValue);
        free(internal);
    }
}

CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(celix_filter_internal_t, celix_filter_destroyInternal)

/**
 * Compiles the filter, so that the attribute values are converted to the typed values if possible.
 */
static celix_status_t celix_filter_compile(celix_filter_t* filter) {
    if (celix_filter_isCompareOperand(filter->operand)) {
        celix_autoptr(celix_filter_internal_t) internal = calloc(1, sizeof(*internal));
        if (!internal) {
            return ENOMEM;
        }
        internal->longValue =
            celix_utils_convertStringToLong(filter->value, 0, &internal->convertedToLong);
        internal->doubleValue =
                celix_utils_convertStringToDouble(filter->value, 0.0, &internal->convertedToDouble);
        internal->boolValue =
                celix_utils_convertStringToBool(filter->value, false, &internal->convertedToBool);

        celix_status_t convertStatus = celix_utils_convertStringToVersion(filter->value, NULL, &internal->versionValue);
        if (convertStatus == ENOMEM) {
            return ENOMEM;
        }
        internal->convertedToVersion = convertStatus == CELIX_SUCCESS;

        filter->internal = celix_steal_ptr(internal);
    }

    if (celix_filter_hasFilterChildren(filter)) {
        for (int i = 0; i < celix_arrayList_size(filter->children); i++) {
            celix_filter_t* child = celix_arrayList_get(filter->children, i);
            celix_status_t status = celix_filter_compile(child);
            if (status != CELIX_SUCCESS) {
                return status;
            }
        }
    }

    return CELIX_SUCCESS;
}

celix_status_t filter_match(celix_filter_t* filter, celix_properties_t* properties, bool* out) {
    bool result = celix_filter_match(filter, properties);
    if (out != NULL) {
        *out = result;
    }
    return CELIX_SUCCESS;
}

static int celix_filter_cmpLong(long a, long b) {
    if (a < b) {
        return -1;
    } else if (a > b) {
        return 1;
    } else {
        return 0;
    }
}

static int celix_filter_cmpDouble(double a, double b) {
    if (a < b) {
        return -1;
    } else if (a > b) {
        return 1;
    } else {
        return 0;
    }
}

static int celix_filter_cmpBool(bool a, bool b) {
    if (a == b) {
        return 0;
    } else if (a) {
        return 1;
    } else {
        return -1;
    }
}

static bool celix_utils_convertCompareToBool(enum celix_filter_operand_enum op, int cmp) {
    switch (op) {
    case CELIX_FILTER_OPERAND_EQUAL:
        return cmp == 0;
    case CELIX_FILTER_OPERAND_GREATER:
        return cmp > 0;
    case CELIX_FILTER_OPERAND_GREATEREQUAL:
        return cmp >= 0;
    case CELIX_FILTER_OPERAND_LESS:
        return cmp < 0;
    case CELIX_FILTER_OPERAND_LESSEQUAL:
        return cmp <= 0;
    default:
        //LCOV_EXCL_START
        assert(false);
        return false;
        //LCOV_EXCL_STOP
    }
}

static bool celix_utils_matchLongArrays(enum celix_filter_operand_enum op, const celix_array_list_t* list, long attributeValue) {
    assert(list != NULL);
    for (int i = 0 ; i < celix_arrayList_size(list); ++i) {
        int cmp = celix_filter_cmpLong(celix_arrayList_getLong(list, i), attributeValue);
        if (celix_utils_convertCompareToBool(op, cmp)) {
            return true;
        }
    }
    return false;
}

static bool celix_utils_matchDoubleArrays(enum celix_filter_operand_enum op, const celix_array_list_t* list, double attributeValue) {
    assert(list != NULL);
    for (int i = 0 ; i < celix_arrayList_size(list); ++i) {
        int cmp = celix_filter_cmpDouble(celix_arrayList_getDouble(list, i), attributeValue);
        if (celix_utils_convertCompareToBool(op, cmp)) {
            return true;
        }
    }
    return false;
}


static bool celix_utils_matchBoolArrays(enum celix_filter_operand_enum op, const celix_array_list_t* list, bool attributeValue) {
    assert(list != NULL);
    for (int i = 0 ; i < celix_arrayList_size(list); ++i) {
        int cmp = celix_filter_cmpBool(celix_arrayList_getBool(list, i), attributeValue);
        if (celix_utils_convertCompareToBool(op, cmp)) {
            return true;
        }
    }
    return false;
}

static bool celix_utils_matchVersionArrays(enum celix_filter_operand_enum op, const celix_array_list_t* list, celix_version_t* attributeValue) {
    assert(list != NULL);
    for (int i = 0 ; i < celix_arrayList_size(list); ++i) {
        int cmp = celix_version_compareTo(celix_arrayList_getVersion(list, i), attributeValue);
        if (celix_utils_convertCompareToBool(op, cmp)) {
            return true;
        }
    }
    return false;
}

static bool celix_utils_matchStringArrays(enum celix_filter_operand_enum op, const celix_array_list_t* list, const char* attributeValue) {
    assert(list != NULL);
    for (int i = 0 ; i < celix_arrayList_size(list); ++i) {
        int cmp = strcmp(celix_arrayList_getString(list, i), attributeValue);
        if (celix_utils_convertCompareToBool(op, cmp)) {
            return true;
        }
    }
    return false;
}


static bool celix_filter_matchSubStringForValue(const celix_filter_t* filter, const char* value) {
    assert(filter->children && celix_arrayList_size(filter->children) >= 2);

    size_t strLen = celix_utils_strlen(value);
    const char* initial = celix_arrayList_getString(filter->children, 0);
    const char* final = celix_arrayList_getString(filter->children, celix_arrayList_size(filter->children) - 1);

    const char* currentValue = value;

    if (initial) {
        const char* found = strstr(value, initial);
        currentValue = found + celix_utils_strlen(initial);
        if (!found || found != value) {
            return false;
        }
    }

    for (int i = 1; i < celix_arrayList_size(filter->children) - 1; i++) {
        const char* substr = celix_arrayList_getString(filter->children, i);
        const char* found = strstr(currentValue, substr);
        if (!found) {
            return false;
        }
        currentValue = found + celix_utils_strlen(substr);
    }

    if (!celix_utils_stringEquals(final, "")) {
        const char* found = strstr(currentValue, final);
        if (!found || found + celix_utils_strlen(final) != value + strLen) {
            return false;
        }
    }

    return true;
}

static bool celix_filter_matchSubString(const celix_filter_t* filter, const celix_properties_entry_t* entry) {
    if (entry->valueType == CELIX_PROPERTIES_VALUE_TYPE_STRING_ARRAY) {
        for (int i = 0; i < celix_arrayList_size(entry->typed.arrayValue); i++) {
            const char* substr = celix_arrayList_getString(entry->typed.arrayValue, i);
            if (celix_filter_matchSubStringForValue(filter, substr)) {
                return true;
            }
            return false;
        }
    }
    return celix_filter_matchSubStringForValue(filter, entry->value);
}

static bool celix_filter_matchApprox(const celix_filter_t* filter, const celix_properties_entry_t* entry) {
    if (entry->valueType == CELIX_PROPERTIES_VALUE_TYPE_STRING_ARRAY) {
        for (int i = 0; i < celix_arrayList_size(entry->typed.arrayValue); i++) {
            const char* substr = celix_arrayList_getString(entry->typed.arrayValue, i);
            if (strcasestr(substr, filter->value) != NULL) {
                return true;
            }
        }
        return false;
    }
    return strcasestr(entry->value, filter->value) != NULL;
}

static bool celix_filter_matchPropertyEntry(const celix_filter_t* filter, const celix_properties_entry_t* entry) {
    if (filter->operand == CELIX_FILTER_OPERAND_SUBSTRING) {
        return celix_filter_matchSubString(filter, entry);
    } else if (filter->operand == CELIX_FILTER_OPERAND_APPROX) {
        return celix_filter_matchApprox(filter, entry);
    }

    assert(filter->operand == CELIX_FILTER_OPERAND_EQUAL || filter->operand == CELIX_FILTER_OPERAND_GREATER ||
           filter->operand == CELIX_FILTER_OPERAND_LESS || filter->operand == CELIX_FILTER_OPERAND_GREATEREQUAL ||
           filter->operand == CELIX_FILTER_OPERAND_LESSEQUAL);


    //match for array types
    if (entry->valueType == CELIX_PROPERTIES_VALUE_TYPE_LONG_ARRAY && filter->internal->convertedToLong) {
        return celix_utils_matchLongArrays(filter->operand, entry->typed.arrayValue, filter->internal->longValue);
    } else if (entry->valueType == CELIX_PROPERTIES_VALUE_TYPE_DOUBLE_ARRAY && filter->internal->convertedToDouble) {
        return celix_utils_matchDoubleArrays(filter->operand, entry->typed.arrayValue, filter->internal->doubleValue);
    } else if (entry->valueType == CELIX_PROPERTIES_VALUE_TYPE_BOOL_ARRAY && filter->internal->convertedToBool) {
        return celix_utils_matchBoolArrays(filter->operand, entry->typed.arrayValue, filter->internal->boolValue);
    } else if (entry->valueType == CELIX_PROPERTIES_VALUE_TYPE_VERSION_ARRAY && filter->internal->convertedToVersion) {
        return celix_utils_matchVersionArrays(filter->operand, entry->typed.arrayValue, filter->internal->versionValue);
    } else if (entry->valueType == CELIX_PROPERTIES_VALUE_TYPE_STRING_ARRAY) {
        return celix_utils_matchStringArrays(filter->operand, entry->typed.arrayValue, filter->value);
    }

    //regular compare -> match
    int cmp;
    if (entry->valueType == CELIX_PROPERTIES_VALUE_TYPE_LONG && filter->internal->convertedToLong) {
        cmp = celix_filter_cmpLong(entry->typed.longValue, filter->internal->longValue);
    } else if (entry->valueType == CELIX_PROPERTIES_VALUE_TYPE_DOUBLE && filter->internal->convertedToDouble) {
        cmp = celix_filter_cmpDouble(entry->typed.doubleValue, filter->internal->doubleValue);
    } else if (entry->valueType == CELIX_PROPERTIES_VALUE_TYPE_BOOL && filter->internal->convertedToBool) {
        cmp = celix_filter_cmpBool(entry->typed.boolValue, filter->internal->boolValue);
    } else if (entry->valueType == CELIX_PROPERTIES_VALUE_TYPE_VERSION && filter->internal->convertedToVersion) {
        cmp = celix_version_compareTo(entry->typed.versionValue, filter->internal->versionValue);
    } else {
        // type string or property type and converted filter attribute value do not match ->
        // fallback on string compare
        cmp = strcmp(entry->value, filter->value);
    }
    return celix_utils_convertCompareToBool(filter->operand, cmp);
}

celix_status_t filter_getString(celix_filter_t* filter, const char** filterStr) {
    if (filter != NULL) {
        *filterStr = filter->filterStr;
    }
    return CELIX_SUCCESS;
}

celix_status_t filter_match_filter(celix_filter_t* src, celix_filter_t* dest, bool* out) {
    bool result = celix_filter_equals(src, dest);
    if (out != NULL) {
        *out = result;
    }
    return CELIX_SUCCESS;
}

celix_filter_t* celix_filter_create(const char* filterString) {
    if (!filterString || filterString[0] == '\0') {
        filterString = "(|)";
    }

    int pos = 0;
    celix_autoptr(celix_filter_t) filter = celix_filter_parseFilter(filterString, &pos);
    if (!filter) {
        celix_err_push("Failed to parse filter string");
        return NULL;
    }
    if (pos != strlen(filterString)) {
        celix_err_push("Filter Error: Extraneous trailing characters.");
        return NULL;
    }
    celix_status_t compileStatus = celix_filter_compile(filter);
    if (compileStatus != CELIX_SUCCESS) {
        celix_err_pushf("Failed to compile filter: %s", celix_strerror(compileStatus));
        return NULL;
    }
    filter->filterStr = celix_utils_strdup(filterString);
    if (NULL == filter->filterStr) {
        celix_err_push("Failed to create filter string");
        return NULL;
    }
    return celix_steal_ptr(filter);
}

void celix_filter_destroy(celix_filter_t* filter) {
    if (!filter) {
        return;
    }

    if (filter->children != NULL) {
        celix_arrayList_destroy(filter->children);
        filter->children = NULL;
    }
    free((char*)filter->value);
    filter->value = NULL;
    free((char*)filter->attribute);
    filter->attribute = NULL;
    free((char*)filter->filterStr);
    filter->filterStr = NULL;
    celix_filter_destroyInternal(filter->internal);
    free(filter);
}

bool celix_filter_match(const celix_filter_t* filter, const celix_properties_t* properties) {
    if (!filter) {
        return true; // if filter is NULL, it matches
    }

    if (filter->operand == CELIX_FILTER_OPERAND_PRESENT) {
        return celix_properties_get(properties, filter->attribute, NULL) != NULL;
    } else if (filter->operand == CELIX_FILTER_OPERAND_AND) {
        celix_array_list_t* children = filter->children;
        for (int i = 0; i < celix_arrayList_size(children); i++) {
            celix_filter_t* childFilter = (celix_filter_t*)celix_arrayList_get(children, i);
            bool childResult = celix_filter_match(childFilter, properties);
            if (!childResult) {
                return false;
            }
        }
        return true;
    } else if (filter->operand == CELIX_FILTER_OPERAND_OR) {
        celix_array_list_t* children = filter->children;
        if (celix_arrayList_size(children) == 0) {
            return true;
        }
        for (int i = 0; i < celix_arrayList_size(children); i++) {
            celix_filter_t* childFilter = (celix_filter_t*)celix_arrayList_get(children, i);
            bool childResult = celix_filter_match(childFilter, properties);
            if (childResult) {
                return true;
            }
        }
        return false;
    } else if (filter->operand == CELIX_FILTER_OPERAND_NOT) {
        celix_filter_t* childFilter = celix_arrayList_get(filter->children, 0);
        bool childResult = celix_filter_match(childFilter, properties);
        return !childResult;
    }

    // present, substring, equal, greater, greaterEqual, less, lessEqual, approx done with matchPropertyEntry
    const celix_properties_entry_t* entry = celix_properties_getEntry(properties, filter->attribute);
    if (!entry) {
        return false;
    }
    return celix_filter_matchPropertyEntry(filter, entry);
}

bool celix_filter_equals(const celix_filter_t* filter1, const celix_filter_t* filter2) {
    if (filter1 == filter2) {
        return true;
    }
    if (!filter1 || !filter2) {
        return false;
    }
    if (filter1->operand != filter2->operand) {
        return false;
    }

    if (filter1->operand == CELIX_FILTER_OPERAND_AND || filter1->operand == CELIX_FILTER_OPERAND_OR ||
        filter1->operand == CELIX_FILTER_OPERAND_NOT) {
        assert(filter1->children != NULL);
        assert(filter2->children != NULL);
        int sizeSrc = celix_arrayList_size(filter1->children);
        int sizeDest = celix_arrayList_size(filter2->children);
        if (sizeSrc == sizeDest) {
            int i;
            int k;
            int sameCount = 0;
            for (i = 0; i < sizeSrc; ++i) {
                bool same = false;
                celix_filter_t* srcPart = celix_arrayList_get(filter1->children, i);
                for (k = 0; k < sizeDest; ++k) {
                    celix_filter_t* destPart = celix_arrayList_get(filter2->children, k);
                    filter_match_filter(srcPart, destPart, &same);
                    if (same) {
                        sameCount += 1;
                        break;
                    }
                }
            }
            return sameCount == sizeSrc;
        }
        return false;
    }

    if (filter1->operand == CELIX_FILTER_OPERAND_SUBSTRING) {
        assert(filter1->children != NULL);
        assert(filter2->children != NULL);
        int sizeSrc = celix_arrayList_size(filter1->children);
        int sizeDest = celix_arrayList_size(filter2->children);
        if (sizeSrc == sizeDest) {
            int i;
            for (i = 0; i < celix_arrayList_size(filter1->children); i++) {
                const char* srcPart = celix_arrayList_getString(filter1->children, i);
                const char* destPart = celix_arrayList_getString(filter2->children, i);
                if (!celix_utils_stringEquals(srcPart, destPart)) {
                    break;
                }
            }
            return i == sizeSrc;
        }
        return false;
    }

    // compare attr and value
    bool attrSame = false;
    bool valSame = false;
    if (filter1->attribute == NULL && filter2->attribute == NULL) {
        attrSame = true;
    } else if (filter1->attribute != NULL && filter2->attribute != NULL) {
        attrSame = celix_utils_stringEquals(filter1->attribute, filter2->attribute);
    }

    if (filter1->value == NULL && filter2->value == NULL) {
        valSame = true;
    } else if (filter1->value != NULL && filter2->value != NULL) {
        valSame = celix_utils_stringEquals(filter1->value, filter2->value);
    }

    return attrSame && valSame;
}

const char* celix_filter_getFilterString(const celix_filter_t* filter) {
    if (filter != NULL) {
        return filter->filterStr;
    }
    return NULL;
}

const char* celix_filter_findAttribute(const celix_filter_t* filter, const char* attribute) {
    const char* result = NULL;
    if (filter != NULL && attribute != NULL) {
        if (filter->operand == CELIX_FILTER_OPERAND_AND || filter->operand == CELIX_FILTER_OPERAND_OR ||
            filter->operand == CELIX_FILTER_OPERAND_NOT) {
            int size = celix_arrayList_size(filter->children);
            for (int i = 0; i < size; ++i) {
                celix_filter_t* child = celix_arrayList_get(filter->children, i);
                result = celix_filter_findAttribute(child, attribute);
                if (result != NULL) {
                    break;
                }
            }
        } else if (celix_utils_stringEquals(filter->attribute, attribute)) {
            result = filter->operand == CELIX_FILTER_OPERAND_PRESENT ? "*" : filter->value;
        }
    }
    return result;
}

static bool
hasMandatoryEqualsValueAttribute(const celix_filter_t* filter, const char* attribute, bool negated, bool optional) {
    bool equalsValueAttribute = false;

    if (filter != NULL && attribute != NULL) {
        if (filter->operand == CELIX_FILTER_OPERAND_AND || filter->operand == CELIX_FILTER_OPERAND_OR ||
            filter->operand == CELIX_FILTER_OPERAND_NOT) {
            int size = celix_arrayList_size(filter->children);
            for (int i = 0; i < size; ++i) {
                if (filter->operand == CELIX_FILTER_OPERAND_NOT) {
                    negated = !negated;
                } else if (filter->operand == CELIX_FILTER_OPERAND_OR) {
                    optional = true;
                }

                celix_filter_t* child = celix_arrayList_get(filter->children, i);

                equalsValueAttribute = hasMandatoryEqualsValueAttribute(child, attribute, negated, optional);

                if (equalsValueAttribute) {
                    break;
                }
            }
        } else if (filter->operand == CELIX_FILTER_OPERAND_EQUAL) {
            equalsValueAttribute = celix_utils_stringEquals(filter->attribute, attribute) && (!negated) && (!optional);
        }
    }

    return equalsValueAttribute;
}

bool celix_filter_hasMandatoryEqualsValueAttribute(const celix_filter_t* filter, const char* attribute) {
    return hasMandatoryEqualsValueAttribute(filter, attribute, false, false);
}

static bool
hasMandatoryNegatedPresenceAttribute(const celix_filter_t* filter, const char* attribute, bool negated, bool optional) {
    bool negatedPresenceAttribute = false;

    if (filter != NULL && attribute != NULL) {
        if (filter->operand == CELIX_FILTER_OPERAND_AND || filter->operand == CELIX_FILTER_OPERAND_OR ||
            filter->operand == CELIX_FILTER_OPERAND_NOT) {
            int size = celix_arrayList_size(filter->children);
            for (int i = 0; i < size; ++i) {
                if (filter->operand == CELIX_FILTER_OPERAND_NOT) {
                    negated = !negated;
                } else if (filter->operand == CELIX_FILTER_OPERAND_OR) {
                    optional = true;
                }

                celix_filter_t* child = celix_arrayList_get(filter->children, i);

                negatedPresenceAttribute = hasMandatoryNegatedPresenceAttribute(child, attribute, negated, optional);

                if (negatedPresenceAttribute) {
                    break;
                }
            }
        } else if (filter->operand == CELIX_FILTER_OPERAND_PRESENT) {
            negatedPresenceAttribute = celix_utils_stringEquals(filter->attribute, attribute) && negated && (!optional);
        }
    }

    return negatedPresenceAttribute;
}
bool celix_filter_hasMandatoryNegatedPresenceAttribute(const celix_filter_t* filter, const char* attribute) {
    return hasMandatoryNegatedPresenceAttribute(filter, attribute, false, false);
}

// NOLINTEND(misc-no-recursion)
