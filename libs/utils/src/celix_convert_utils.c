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

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "celix_array_list.h"
#include "celix_err.h"
#include "celix_utils.h"

#define ESCAPE_CHAR '\\'
#define SEPARATOR_CHAR ','

static bool celix_utils_isEndptrEndOfStringOrOnlyContainsWhitespaces(const char* endptr) {
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
                                                           const celix_array_list_t* defaultValue,
                                                           celix_array_list_t** list,
                                                           void (*freeCb)(void*),
                                                           celix_status_t (*addEntry)(celix_array_list_t*,
                                                                                      const char*)) {
    assert(list != NULL);
    *list = NULL;

    if (!val && defaultValue) {
        *list = celix_arrayList_copy(defaultValue);
        return *list ? CELIX_ILLEGAL_ARGUMENT : CELIX_ENOMEM;
    } else if (!val) {
        return CELIX_ILLEGAL_ARGUMENT;
    }

    celix_array_list_create_options_t opts = CELIX_EMPTY_ARRAY_LIST_CREATE_OPTIONS;
    opts.simpleRemovedCallback = freeCb;
    celix_autoptr(celix_array_list_t) result = celix_arrayList_createWithOptions(&opts);
    if (!result) {
        return CELIX_ENOMEM;
    }

    char* buf = NULL;
    size_t bufSize = 0;
    FILE* entryStream = NULL;
    celix_status_t status = CELIX_SUCCESS;
    size_t max = strlen(val);
    for (size_t i = 0; i <= max; ++i) {
        if (!entryStream) {
            entryStream = open_memstream(&buf, &bufSize);
            if (!entryStream) {
                 return CELIX_ENOMEM;
            }
        }
        if (val[i] == ESCAPE_CHAR) {
            // escape character, next char must be escapeChar or separatorChar
            if (i + 1 < max && (val[i + 1] == ESCAPE_CHAR || val[i + 1] == SEPARATOR_CHAR)) {
                // write escaped char
                i += 1;
                int rc = fputc(val[i], entryStream);
                if (rc == EOF) {
                    return CELIX_ENOMEM;
                }
                continue;
            } else {
                // invalid escape (ending with escapeChar or followed by an invalid char)
                status = CELIX_ILLEGAL_ARGUMENT;
                break;
            }
        } else if (val[i] == SEPARATOR_CHAR || val[i] == '\0') {
            //end of entry
            fclose(entryStream);
            entryStream = NULL;
            status = addEntry(result, buf);
            if (status == CELIX_ENOMEM) {
                return status;
            }
        } else {
            //normal char
            int rc = fputc(val[i], entryStream);
            if (rc == EOF) {
                return CELIX_ENOMEM;
            }
        }
    }


    if (status == CELIX_SUCCESS) {
        *list = celix_steal_ptr(result);
    } else if (status == CELIX_ILLEGAL_ARGUMENT) {
        if (defaultValue) {
            *list = celix_arrayList_copy(defaultValue);
            return *list ? status : CELIX_ENOMEM;
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
    return celix_utils_convertStringToArrayList(val, defaultValue, list, NULL, celix_utils_addLongEntry);
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
    return celix_utils_convertStringToArrayList(val, defaultValue, list, NULL, celix_utils_addDoubleEntry);
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
    return celix_utils_convertStringToArrayList(val, defaultValue, list, NULL, celix_utils_addBoolEntry);
}

celix_status_t celix_utils_addStringEntry(celix_array_list_t* list, const char* entry) {
    char* copy = celix_utils_strdup(entry);
    if (!copy) {
            return CELIX_ENOMEM;
    }
    return celix_arrayList_add(list, copy);
}

celix_status_t celix_utils_convertStringToStringArrayList(const char* val,
                                                          const celix_array_list_t* defaultValue,
                                                          celix_array_list_t** list) {
    return celix_utils_convertStringToArrayList(val, defaultValue, list, free, celix_utils_addStringEntry);
}

static celix_status_t celix_utils_addVersionEntry(celix_array_list_t* list, const char* entry) {
    celix_version_t* version;
    celix_status_t convertStatus = celix_utils_convertStringToVersion(entry, NULL, &version);
    if (convertStatus == CELIX_SUCCESS) {
        return celix_arrayList_add(list, version);
    }
    return convertStatus;
}

static void celix_utils_destroyVersionEntry(void* entry) { celix_version_destroy(entry); }

celix_status_t celix_utils_convertStringToVersionArrayList(const char* val,
                                                           const celix_array_list_t* defaultValue,
                                                           celix_array_list_t** list) {
    return celix_utils_convertStringToArrayList(
        val, defaultValue, list, celix_utils_destroyVersionEntry, celix_utils_addVersionEntry);
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
static char* celix_utils_arrayListToString(const celix_array_list_t* list,
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

char* celix_utils_longArrayListToString(const celix_array_list_t* list) {
    return celix_utils_arrayListToString(list, celix_utils_printLongEntry);
}

static int celix_utils_printDoubleEntry(FILE* stream, const celix_array_list_entry_t* entry) {
    return fprintf(stream, "%lf", entry->doubleVal);
}

char* celix_utils_doubleArrayListToString(const celix_array_list_t* list) {
    return celix_utils_arrayListToString(list, celix_utils_printDoubleEntry);
}

static int celix_utils_printBoolEntry(FILE* stream, const celix_array_list_entry_t* entry) {
    return fprintf(stream, "%s", entry->boolVal ? "true" : "false");
}

char* celix_utils_boolArrayListToString(const celix_array_list_t* list) {
    return celix_utils_arrayListToString(list, celix_utils_printBoolEntry);
}

static int celix_utils_printStrEntry(FILE* stream, const celix_array_list_entry_t* entry) {
    const char* str = entry->strVal;
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

char* celix_utils_stringArrayListToString(const celix_array_list_t* list) {
    return celix_utils_arrayListToString(list, celix_utils_printStrEntry);
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

char* celix_utils_versionArrayListToString(const celix_array_list_t* list) {
    return celix_utils_arrayListToString(list, celix_utils_printVersionEntry);
}
