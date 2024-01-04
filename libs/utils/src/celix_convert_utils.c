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
#include "celix_utils.h"
#include "celix_stdio_cleanup.h"
#include "celix_err.h"

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
        char *endptr;
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
        char *endptr;
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

celix_status_t celix_utils_convertStringToVersion(const char* val, const celix_version_t* defaultValue, celix_version_t** version) {
    assert(version != NULL);
    if (!val && defaultValue) {
        *version = celix_version_copy(defaultValue);
        return *version ? CELIX_ILLEGAL_ARGUMENT : CELIX_ENOMEM;
    } else if (!val) {
        return CELIX_ILLEGAL_ARGUMENT;
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

celix_status_t celix_utils_convertStringToLongArrayList(const char* val, const celix_array_list_t* defaultValue, celix_array_list_t** list) {
        assert(list != NULL);
        *list = NULL;

        if (!val && defaultValue) {
            *list = celix_arrayList_copy(defaultValue);
            return *list ? CELIX_ILLEGAL_ARGUMENT : CELIX_ENOMEM;
        } else if (!val) {
            return CELIX_ILLEGAL_ARGUMENT;
        }

        celix_autoptr(celix_array_list_t) result = celix_arrayList_create();
        if (!result) {
            return CELIX_ENOMEM;
        }

        celix_status_t status = CELIX_SUCCESS;
        if (val) {
                char buf[256];
                char* valCopy = celix_utils_writeOrCreateString(buf, sizeof(buf), "%s", val);
                char* savePtr = NULL;
                char* token = strtok_r(valCopy, ",", &savePtr);
                while (token != NULL) {
                    bool converted;
                    long l = celix_utils_convertStringToLong(token, 0L, &converted);
                    if (!converted) {
                        status =  CELIX_ILLEGAL_ARGUMENT;
                        break;
                    }
                    status = celix_arrayList_addLong(result, l);
                    if (status != CELIX_SUCCESS) {
                        break;
                    }
                    token = strtok_r(NULL, ",", &savePtr);
                }
                celix_utils_freeStringIfNotEqual(buf, valCopy);
        }
        if (status == CELIX_ILLEGAL_ARGUMENT) {
            if (defaultValue) {
                *list = celix_arrayList_copy(defaultValue);
                return *list ? status : CELIX_ENOMEM;
            }
            return status;
        }
        *list = celix_steal_ptr(result);
        return status;
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
static char* celix_utils_arrayListToString(const celix_array_list_t *list, int (*printCb)(FILE* stream, const celix_array_list_entry_t* entry)) {
    char* result = NULL;
    size_t len;
    celix_autoptr(FILE) stream = open_memstream(&result, &len);
    if (!stream) {
        celix_err_push("Cannot open memstream");
        return NULL;
    }

    int size = list ? celix_arrayList_size(list) : 0;
    for (int i = 0; i < size; ++i) {
        celix_array_list_entry_t entry = celix_arrayList_getEntry(list, i);
        int rc = printCb(stream, &entry);
        if (rc >= 0 && i < size - 1) {
            rc = fputs(", ", stream);
        }
        if (rc < 0) {
            celix_err_push("Cannot print to stream");
            return NULL;
        }
    }
    fclose(celix_steal_ptr(stream));
    return result;
}

static int celix_utils_printLongEntry(FILE* stream, const celix_array_list_entry_t* entry) {
    return fprintf(stream, "%li", entry->longVal);
}

//static int celix_properties_printDoubleEntry(FILE* stream, const celix_array_list_entry_t* entry) {
//    return fprintf(stream, "%lf", entry->doubleVal);
//}
//
//static int celix_properties_printBoolEntry(FILE* stream, const celix_array_list_entry_t* entry) {
//    return fprintf(stream, "%s", entry->boolVal ? "true" : "false");
//}
//
//static int celix_properties_printStrEntry(FILE* stream, const celix_array_list_entry_t* entry) {
//    return fprintf(stream, "%s", (const char*)entry->voidPtrVal);
//}

char* celix_utils_longArrayListToString(const celix_array_list_t* list) {
    return celix_utils_arrayListToString(list, celix_utils_printLongEntry);
}

//static char* celix_properties_doubleArrayListToString(const celix_array_list_t* list) {
//    return celix_properties_arrayListToString(list, celix_properties_printDoubleEntry);
//}
//
//static char* celix_properties_boolArrayListToString(const celix_array_list_t* list) {
//    return celix_properties_arrayListToString(list, celix_properties_printBoolEntry);
//}
//
//static char* celix_properties_stringArrayListToString(const celix_array_list_t* list) {
//    return celix_properties_arrayListToString(list, celix_properties_printStrEntry);
//}