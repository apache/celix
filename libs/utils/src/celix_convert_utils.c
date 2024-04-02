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

#include "celix_convert_utils.h"
#include "celix_convert_utils_private.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "celix_array_list.h"
#include "celix_err.h"
#include "celix_stdio_cleanup.h"
#include "celix_stdlib_cleanup.h"
#include "celix_utils.h"

#define ESCAPE_CHAR '\\'
#define SEPARATOR_CHAR ','

bool celix_utils_isEndptrEndOfStringOrOnlyContainsWhitespaces(const char* endptr) {
    bool result = false;
    if (endptr != NULL) {
        while (*endptr != '\0') {
            if (!isspace(*endptr)) {
                break;
            }
            endptr++;
        }
        result = *endptr == '\0';
    }
    return result;
}

bool celix_utils_convertStringToBool(const char* val, bool defaultValue, bool* converted) {
    bool result;
    if (converted != NULL) {
        *converted = false;
    }
    if (val == NULL) {
        return defaultValue;
    }
    const char* p = val;
    while (isspace(*p)) {
        p++;
    }
    if (strncasecmp("true", p, 4) == 0) {
        p += 4;
        result = true;
    } else if (strncasecmp("false", p, 5) == 0) {
        p += 5;
        result = false;
    } else {
        return defaultValue;
    }
    if (!celix_utils_isEndptrEndOfStringOrOnlyContainsWhitespaces(p)) {
        return defaultValue;
    }
    if (converted != NULL) {
        *converted = true;
    }
    return result;
}

double celix_utils_convertStringToDouble(const char* val, double defaultValue, bool* converted) {
    double result = defaultValue;
    if (converted != NULL) {
        *converted = false;
    }
    if (val != NULL) {
        char* endptr;
        double d = strtod(val, &endptr);
        if (endptr != val && celix_utils_isEndptrEndOfStringOrOnlyContainsWhitespaces(endptr)) {
            result = d;
            if (converted) {
                *converted = true;
            }
        }
    }
    return result;
}

long celix_utils_convertStringToLong(const char* val, long defaultValue, bool* converted) {
    long result = defaultValue;
    if (converted != NULL) {
        *converted = false;
    }
    if (val != NULL) {
        char* endptr;
        long l = strtol(val, &endptr, 10);
        if (endptr != val && celix_utils_isEndptrEndOfStringOrOnlyContainsWhitespaces(endptr)) {
            result = l;
            if (converted) {
                *converted = true;
            }
        }
    }
    return result;
}

celix_status_t
celix_utils_convertStringToVersion(const char* val, const celix_version_t* defaultValue, celix_version_t** version) {
    assert(version != NULL);
    if (!val && defaultValue) {
        *version = celix_version_copy(defaultValue);
        return *version ? CELIX_ILLEGAL_ARGUMENT : CELIX_ENOMEM;
    }

    celix_status_t status = celix_version_parse(val, version);
    if (status == CELIX_ILLEGAL_ARGUMENT) {
        if (defaultValue) {
            *version = celix_version_copy(defaultValue);
            return *version ? status : CELIX_ENOMEM;
        }
        return status;
    }
    return status;
}

/**
 * @brief Convert the provided string to an array list using the provided addEntry callback to convert the string
 * to a specific type and add it to the list.
 */
static celix_status_t celix_utils_convertStringToArrayList(const char* val,
                                                           celix_array_list_t* listIn,
                                                           const celix_array_list_t* defaultValue,
                                                           celix_status_t (*addEntry)(celix_array_list_t*,
                                                                                      const char*),
                                                           celix_array_list_t** listOut) {
    assert(listOut != NULL);
    *listOut = NULL;
    celix_autoptr(celix_array_list_t) list = listIn;

    if (!val && defaultValue) {
        *listOut = celix_arrayList_copy(defaultValue);
        return *listOut ? CELIX_ILLEGAL_ARGUMENT : CELIX_ENOMEM;
    } else if (!list) {
        return ENOMEM;
    } else if (!val) {
        return CELIX_ILLEGAL_ARGUMENT;
    }

    char* buf = NULL;
    size_t bufSize = 0;
    FILE* entryStream = NULL;
    celix_status_t status = CELIX_SUCCESS;
    size_t max = strlen(val);
    entryStream = open_memstream(&buf, &bufSize);
    if (!entryStream) {
        return CELIX_ENOMEM;
    }
    for (size_t i = 0; i <= max && status == CELIX_SUCCESS; ++i) {
        if (val[i] == ESCAPE_CHAR) {
            // escape character, next char must be escapeChar or separatorChar
            if (i + 1 < max && (val[i + 1] == ESCAPE_CHAR || val[i + 1] == SEPARATOR_CHAR)) {
                // write escaped char
                i += 1;
                int rc = fputc(val[i], entryStream);
                if (rc == EOF) {
                    status = CELIX_ENOMEM;
                }
            } else {
                // invalid escape (ending with escapeChar or followed by an invalid char)
                status = CELIX_ILLEGAL_ARGUMENT;
            }
        } else if (val[i] == SEPARATOR_CHAR || val[i] == '\0') {
            //end of entry
            int rc = fputc('\0', entryStream);
            if (rc == EOF) {
                status = CELIX_ENOMEM;
                continue;
            }
            fflush(entryStream);
            status = addEntry(list, buf);
            rewind(entryStream);
        } else {
            //normal char
            int rc = fputc(val[i], entryStream);
            if (rc == EOF) {
                status = CELIX_ENOMEM;
            }
        }
    }
    fclose(entryStream);
    free(buf);

    if (status == CELIX_SUCCESS) {
        *listOut = celix_steal_ptr(list);
    } else if (status == CELIX_ILLEGAL_ARGUMENT) {
        if (defaultValue) {
            *listOut = celix_arrayList_copy(defaultValue);
            return *listOut ? status : CELIX_ENOMEM;
        }
        return status;
    }
    return status;
}

celix_status_t celix_utils_addLongEntry(celix_array_list_t* list, const char* entry) {
    bool converted;
    long l = celix_utils_convertStringToLong(entry, 0L, &converted);
    if (!converted) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    return celix_arrayList_addLong(list, l);
}

celix_status_t celix_utils_convertStringToLongArrayList(const char* val,
                                                        const celix_array_list_t* defaultValue,
                                                        celix_array_list_t** list) {
    return celix_utils_convertStringToArrayList(
        val, celix_arrayList_createLongArray(), defaultValue, celix_utils_addLongEntry, list);
}

celix_status_t celix_utils_addDoubleEntry(celix_array_list_t* list, const char* entry) {
    bool converted;
    double d = celix_utils_convertStringToDouble(entry, 0.0, &converted);
    if (!converted) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    return celix_arrayList_addDouble(list, d);
}

celix_status_t celix_utils_convertStringToDoubleArrayList(const char* val,
                                                          const celix_array_list_t* defaultValue,
                                                          celix_array_list_t** list) {
    return celix_utils_convertStringToArrayList(
        val, celix_arrayList_createDoubleArray(), defaultValue, celix_utils_addDoubleEntry, list);
}

celix_status_t celix_utils_addBoolEntry(celix_array_list_t* list, const char* entry) {
    bool converted;
    bool b = celix_utils_convertStringToBool(entry, true, &converted);
    if (!converted) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    return celix_arrayList_addBool(list, b);
}

celix_status_t celix_utils_convertStringToBoolArrayList(const char* val,
                                                        const celix_array_list_t* defaultValue,
                                                        celix_array_list_t** list) {
    return celix_utils_convertStringToArrayList(
        val, celix_arrayList_createBoolArray(), defaultValue, celix_utils_addBoolEntry, list);
}

celix_status_t celix_utils_convertStringToStringArrayList(const char* val,
                                                          const celix_array_list_t* defaultValue,
                                                          celix_array_list_t** list) {
    return celix_utils_convertStringToArrayList(
        val, celix_arrayList_createStringArray(), defaultValue, celix_arrayList_addString, list);
}

static celix_status_t celix_utils_addVersionEntry(celix_array_list_t* list, const char* entry) {
    celix_version_t* version;
    celix_status_t convertStatus = celix_utils_convertStringToVersion(entry, NULL, &version);
    if (convertStatus == CELIX_SUCCESS) {
        return celix_arrayList_assignVersion(list, version);
    }
    return convertStatus;
}

celix_status_t celix_utils_convertStringToVersionArrayList(const char* val,
                                                           const celix_array_list_t* defaultValue,
                                                           celix_array_list_t** list) {
    return celix_utils_convertStringToArrayList(
        val, celix_arrayList_createVersionArray(), defaultValue, celix_utils_addVersionEntry, list);
}

/**
 * @brief Convert the provided array list to a string using the provided print callback.
 *
 * Will log an error message to celix_err if an error occurred.
 *
 * @param list The list to convert.
 * @param printCb The callback to use for printing the list entries.
 * @return The string representation of the list or NULL if an error occurred.
 */
static char* celix_utils_arrayListToStringInternal(const celix_array_list_t* list,
                                           int (*printCb)(FILE* stream, const celix_array_list_entry_t* entry)) {
    char* result = NULL;
    size_t len;
    FILE* stream = open_memstream(&result, &len);
    if (!stream) {
        celix_err_push("Cannot open memstream");
        return NULL;
    }

    int size = list ? celix_arrayList_size(list) : 0;
    for (int i = 0; i < size; ++i) {
        celix_array_list_entry_t entry = celix_arrayList_getEntry(list, i);
        int rc = printCb(stream, &entry);
        if (rc >= 0 && i < size - 1) {
            rc = fputs(",", stream);
        }
        if (rc < 0) {
            celix_err_push("Cannot print to stream");
            fclose(stream);
            free(result);
            return NULL;
        }
    }
    fclose(stream);
    return result;
}

static int celix_utils_printLongEntry(FILE* stream, const celix_array_list_entry_t* entry) {
    return fprintf(stream, "%li", entry->longVal);
}

static int celix_utils_printDoubleEntry(FILE* stream, const celix_array_list_entry_t* entry) {
    return fprintf(stream, "%lf", entry->doubleVal);
}

static int celix_utils_printBoolEntry(FILE* stream, const celix_array_list_entry_t* entry) {
    return fprintf(stream, "%s", entry->boolVal ? "true" : "false");
}

static int celix_utils_printStrEntry(FILE* stream, const celix_array_list_entry_t* entry) {
    const char* str = entry->stringVal;
    int rc = 0;
    for (int i = 0; str[i] != '\0'; ++i) {
        if (str[i] == ESCAPE_CHAR || str[i] == SEPARATOR_CHAR) {
            //both escape and separator char need to be escaped
            rc = fputc(ESCAPE_CHAR, stream);
        }
        if (rc != EOF) {
            rc = fputc(str[i], stream);
        }
        if (rc == EOF) {
            break;
        }
    }
    return rc;
}

static int celix_utils_printVersionEntry(FILE* stream, const celix_array_list_entry_t* entry) {
    celix_version_t* version = entry->voidPtrVal;
    int major = celix_version_getMajor(version);
    int minor = celix_version_getMinor(version);
    int micro = celix_version_getMicro(version);
    const char* q = celix_version_getQualifier(version);
    if (celix_utils_isStringNullOrEmpty(q)) {
        return fprintf(stream, "%i.%i.%i", major, minor, micro);
    }
    return fprintf(stream, "%i.%i.%i.%s", major, minor, micro, q);
}

char* celix_utils_arrayListToString(const celix_array_list_t* list) {
    if (!list) {
        return NULL;
    }
    celix_array_list_element_type_t elType = celix_arrayList_getElementType(list);
    switch (elType) {
        case CELIX_ARRAY_LIST_ELEMENT_TYPE_LONG:
            return celix_utils_arrayListToStringInternal(list, celix_utils_printLongEntry);
        case CELIX_ARRAY_LIST_ELEMENT_TYPE_DOUBLE:
            return celix_utils_arrayListToStringInternal(list, celix_utils_printDoubleEntry);
        case CELIX_ARRAY_LIST_ELEMENT_TYPE_BOOL:
            return celix_utils_arrayListToStringInternal(list, celix_utils_printBoolEntry);
        case CELIX_ARRAY_LIST_ELEMENT_TYPE_STRING:
            return celix_utils_arrayListToStringInternal(list, celix_utils_printStrEntry);
        case CELIX_ARRAY_LIST_ELEMENT_TYPE_VERSION:
            return celix_utils_arrayListToStringInternal(list, celix_utils_printVersionEntry);
        default:
            return NULL;
    }
}
