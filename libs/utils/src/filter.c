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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <utils.h>

#include "celix_convert_utils.h"
#include "celix_err.h"
#include "celix_errno.h"
#include "celix_filter.h"
#include "celix_stdlib_cleanup.h"
#include "celix_version.h"
#include "filter.h"

struct celix_filter_internal {
    bool convertedToLong;
    long longValue;
    bool convertedToDouble;
    double doubleValue;
    bool convertedToVersion;
    celix_version_t *versionValue;
};

static void filter_skipWhiteSpace(char* filterString, int* pos);
static celix_filter_t * filter_parseFilter(char* filterString, int* pos);
static celix_filter_t * filter_parseFilterComp(char* filterString, int* pos);
static celix_filter_t * filter_parseAndOrOr(char* filterString, celix_filter_operand_t andOrOr, int* pos);
static celix_filter_t * filter_parseNot(char* filterString, int* pos);
static celix_filter_t * filter_parseItem(char* filterString, int* pos);
static char * filter_parseAttr(char* filterString, int* pos);
static char * filter_parseValue(char* filterString, int* pos);
static celix_array_list_t* filter_parseSubstring(char* filterString, int* pos);

static void filter_skipWhiteSpace(char * filterString, int * pos) {
    int length;
    for (length = strlen(filterString); (*pos < length) && isspace(filterString[*pos]);) {
        (*pos)++;
    }
}

celix_filter_t * filter_create(const char* filterString) {
    return celix_filter_create(filterString);
}

void filter_destroy(celix_filter_t * filter) {
    return celix_filter_destroy(filter);
}

static celix_filter_t * filter_parseFilter(char* filterString, int* pos) {
    filter_skipWhiteSpace(filterString, pos);
    if (filterString[*pos] == '\0') {
        //empty filter
        celix_err_push("Filter Error: Cannot create LDAP filter from an empty string.\n");
        return NULL;
    } else if (filterString[*pos] != '(') {
        celix_err_pushf("Filter Error: Missing '(' in filter string '%s'.\n", filterString);
        return NULL;
    }
    (*pos)++; //eat '('

    celix_autoptr(celix_filter_t) filter = filter_parseFilterComp(filterString, pos);

    filter_skipWhiteSpace(filterString, pos);

    if (filterString[*pos] != ')') {
        celix_err_pushf("Filter Error: Missing ')' in filter string '%s'.\n", filterString);
        return NULL;
    }
    (*pos)++; //eat ')'
    filter_skipWhiteSpace(filterString, pos);

    return celix_steal_ptr(filter);
}

static celix_filter_t * filter_parseFilterComp(char * filterString, int * pos) {
    char c;
    filter_skipWhiteSpace(filterString, pos);

    c = filterString[*pos];

    switch (c) {
        case '&': {
            (*pos)++;
            return filter_parseAndOrOr(filterString, CELIX_FILTER_OPERAND_AND, pos);
        }
        case '|': {
            (*pos)++;
            return filter_parseAndOrOr(filterString, CELIX_FILTER_OPERAND_OR, pos);
        }
        case '!': {
            (*pos)++;
            return filter_parseNot(filterString, pos);
        }
    }
    return filter_parseItem(filterString, pos);
}

static celix_filter_t * filter_parseAndOrOr(char * filterString, celix_filter_operand_t andOrOr, int * pos) {

    filter_skipWhiteSpace(filterString, pos);
    bool failure = false;

    if (filterString[*pos] != '(') {
        fprintf(stderr, "Filter Error: Missing '('.\n");
        return NULL;
    }

    celix_array_list_t *children = celix_arrayList_create();
    while(filterString[*pos] == '(') {
        celix_filter_t * child = filter_parseFilter(filterString, pos);
        if(child == NULL) {
            failure = true;
            break;
        }
        celix_arrayList_add(children, child);
    }

    if(failure == true){
        for (int i = 0; i < celix_arrayList_size(children); ++i) {
            celix_filter_t * f = celix_arrayList_get(children, i);
            filter_destroy(f);
        }
        celix_arrayList_destroy(children);
        return NULL;
    }

    celix_filter_t * filter = (celix_filter_t *) calloc(1, sizeof(*filter));
    filter->operand = andOrOr;
    filter->children = children;

    return filter;
}

static celix_filter_t * filter_parseNot(char * filterString, int * pos) {
    celix_filter_t * child = NULL;
    filter_skipWhiteSpace(filterString, pos);

    if (filterString[*pos] != '(') {
        fprintf(stderr, "Filter Error: Missing '('.\n");
        return NULL;
    }

    child = filter_parseFilter(filterString, pos);
    if (child == NULL) {
        return NULL;
    }
    celix_array_list_t* children = celix_arrayList_create();
    celix_arrayList_add(children, child);

    celix_filter_t * filter = (celix_filter_t *) calloc(1, sizeof(*filter));
    filter->operand = CELIX_FILTER_OPERAND_NOT;
    filter->children = children;

    return filter;
}

static celix_filter_t * filter_parseItem(char * filterString, int * pos) {
    char * attr = filter_parseAttr(filterString, pos);
    if(attr == NULL){
        return NULL;
    }

    filter_skipWhiteSpace(filterString, pos);
    switch(filterString[*pos]) {
        case '~': {
            if (filterString[*pos + 1] == '=') {
                celix_filter_t * filter = (celix_filter_t *) calloc(1, sizeof(*filter));
                *pos += 2;
                filter->operand = CELIX_FILTER_OPERAND_APPROX;
                filter->attribute = attr;
                filter->value = filter_parseValue(filterString, pos);
                return filter;
            }
            break;
        }
        case '>': {
            if (filterString[*pos + 1] == '=') {
                celix_filter_t * filter = (celix_filter_t *) calloc(1, sizeof(*filter));
                *pos += 2;
                filter->operand = CELIX_FILTER_OPERAND_GREATEREQUAL;
                filter->attribute = attr;
                filter->value = filter_parseValue(filterString, pos);
                return filter;
            }
            else {
                celix_filter_t * filter = (celix_filter_t *) calloc(1, sizeof(*filter));
                *pos += 1;
                filter->operand = CELIX_FILTER_OPERAND_GREATER;
                filter->attribute = attr;
                filter->value = filter_parseValue(filterString, pos);
                return filter;
            }
            break;
        }
        case '<': {
            if (filterString[*pos + 1] == '=') {
                celix_filter_t * filter = (celix_filter_t *) calloc(1, sizeof(*filter));
                *pos += 2;
                filter->operand = CELIX_FILTER_OPERAND_LESSEQUAL;
                filter->attribute = attr;
                filter->value = filter_parseValue(filterString, pos);
                return filter;
            }
            else {
                celix_filter_t * filter = (celix_filter_t *) calloc(1, sizeof(*filter));
                *pos += 1;
                filter->operand = CELIX_FILTER_OPERAND_LESS;
                filter->attribute = attr;
                filter->value = filter_parseValue(filterString, pos);
                return filter;
            }
            break;
        }
        case '=': {
            celix_filter_t * filter = NULL;
            celix_array_list_t *subs;
            if (filterString[*pos + 1] == '*') {
                int oldPos = *pos;
                *pos += 2;
                filter_skipWhiteSpace(filterString, pos);
                if (filterString[*pos] == ')') {
                    filter = (celix_filter_t *) calloc(1, sizeof(*filter));
                    filter->operand = CELIX_FILTER_OPERAND_PRESENT;
                    filter->attribute = attr;
                    filter->value = NULL;
                    return filter;
                }
                *pos = oldPos;
            }
            filter = (celix_filter_t *) calloc(1, sizeof(*filter));            
            (*pos)++;
            subs = filter_parseSubstring(filterString, pos);
            if(subs!=NULL){
                if (celix_arrayList_size(subs) == 1) {
                    char * string = (char *) celix_arrayList_get(subs, 0);
                    if (string != NULL) {
                        filter->operand = CELIX_FILTER_OPERAND_EQUAL;
                        filter->attribute = attr;
                        filter->value = string;

                        celix_arrayList_clear(subs);
                        celix_arrayList_destroy(subs);

                        return filter;
                    }
                }
            }
            filter->operand = CELIX_FILTER_OPERAND_SUBSTRING;
            filter->attribute = attr;
            filter->children = subs;
            return filter;
        }
    }
    fprintf(stderr, "Filter Error: Invalid operator.\n");
    free(attr);
    return NULL;
}

static char * filter_parseAttr(char * filterString, int * pos) {
    char c;
    int begin = *pos;
    int end = *pos;
    int length = 0;

    filter_skipWhiteSpace(filterString, pos);
    c = filterString[*pos];

    while (c != '~' && c != '<' && c != '>' && c != '=' && c != '(' && c != ')') {
        (*pos)++;

        if (!isspace(c)) {
            end = *pos;
        }

        c = filterString[*pos];
    }

    length = end - begin;

    if (length == 0) {
        fprintf(stderr, "Filter Error: Missing attr.\n");
        return NULL;
    } else {
        char * attr = (char *) calloc(1, length+1);
        strncpy(attr, filterString+begin, length);
        attr[length] = '\0';
        return attr;
    }
}

static char * filter_parseValue(char * filterString, int * pos) {
    char *value = calloc(strlen(filterString) + 1, sizeof(*value));
    int keepRunning = 1;

    while (keepRunning) {
        char c = filterString[*pos];

        switch (c) {
            case ')': {
                keepRunning = 0;
                break;
            }
            case '(': {
                fprintf(stderr, "Filter Error: Invalid value.\n");
                free(value);
                return NULL;
            }
            case '\0':{
                fprintf(stderr, "Filter Error: Unclosed bracket.\n");
                free(value);
                return NULL;
            }
            case '\\': {
                (*pos)++;
                c = filterString[*pos];
            }
            /* no break */
            default: {
                char ch[2];
                ch[0] = c;
                ch[1] = '\0';
                strcat(value, ch);
                (*pos)++;
                break;
            }
        }
    }

    if (strlen(value) == 0) {
        fprintf(stderr, "Filter Error: Missing value.\n");
        free(value);
        return NULL;
    }
    return value;
}

static celix_array_list_t* filter_parseSubstring(char * filterString, int * pos) {
    char *sub = calloc(strlen(filterString) + 1, sizeof(*sub));
    celix_array_list_t* operands = celix_arrayList_create();
    int keepRunning = 1;

    while (keepRunning) {
        char c = filterString[*pos];
        

        switch (c) {
            case ')': {
                if (strlen(sub) > 0) {
                    celix_arrayList_add(operands, strdup(sub));
                }
                keepRunning = 0;
                break;
            }
            case '\0':{
                fprintf(stderr, "Filter Error: Unclosed bracket.\n");
                keepRunning = false;
                break;
            }
            case '(': {
                fprintf(stderr, "Filter Error: Invalid value.\n");
                keepRunning = false;
                break;
            }
            case '*': {
                if (strlen(sub) > 0) {
                    celix_arrayList_add(operands, strdup(sub));
                }
                sub[0] = '\0';
                celix_arrayList_add(operands, NULL);
                (*pos)++;
                break;
            }
            case '\\': {
                (*pos)++;
                c = filterString[*pos];
            }
            /* no break */
            default: {
                char ch[2];
                ch[0] = c;
                ch[1] = '\0';
                strcat(sub, ch);
                (*pos)++;
                break;
            }
        }
    }
    free(sub);

    if (celix_arrayList_size(operands) == 0) {
        fprintf(stderr, "Filter Error: Missing value.\n");
        celix_arrayList_destroy(operands);
        return NULL;
    }

    return operands;
}

static bool celix_filter_isCompareOperand(celix_filter_operand_t operand) {
    return  operand == CELIX_FILTER_OPERAND_EQUAL ||
            operand == CELIX_FILTER_OPERAND_GREATER ||
            operand == CELIX_FILTER_OPERAND_LESS ||
            operand == CELIX_FILTER_OPERAND_GREATEREQUAL ||
            operand == CELIX_FILTER_OPERAND_LESSEQUAL;
}


static bool celix_filter_hasFilterChildren(celix_filter_t* filter) {
    return filter->operand == CELIX_FILTER_OPERAND_AND ||
           filter->operand == CELIX_FILTER_OPERAND_OR ||
           filter->operand == CELIX_FILTER_OPERAND_NOT;
}

/**
 * Compiles the filter, so that the attribute values are converted to the typed values if possible.
 */
static celix_status_t celix_filter_compile(celix_filter_t* filter) {
    if (celix_filter_isCompareOperand(filter->operand)) {
        filter->internal = malloc(sizeof(*filter->internal));
        if (filter->internal == NULL) {
            return CELIX_ENOMEM;
        } else {
            filter->internal->longValue = celix_utils_convertStringToLong(filter->value, 0, &filter->internal->convertedToLong);
            filter->internal->doubleValue = celix_utils_convertStringToDouble(filter->value, 0.0, &filter->internal->convertedToDouble);
            filter->internal->versionValue = celix_utils_convertStringToVersion(filter->value, NULL, &filter->internal->convertedToVersion);
        }
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

celix_status_t filter_match(celix_filter_t * filter, celix_properties_t *properties, bool *out) {
    bool result = celix_filter_match(filter, properties);
    if (out != NULL) {
        *out = result;
    }
    return CELIX_SUCCESS;
}

static int celix_filter_compareAttributeValue(const celix_filter_t* filter, const celix_properties_entry_t* entry) {
    const char* propertyValue = entry->value;
    if (!filter->internal->convertedToLong && !filter->internal->convertedToDouble && !filter->internal->convertedToVersion) {
        return strcmp(propertyValue, filter->value);
    }

    if (filter->internal->convertedToLong) {
        bool propertyValueIsLong = false;
        long value = celix_utils_convertStringToLong(propertyValue, 0, &propertyValueIsLong);
        if (propertyValueIsLong) {
            if (value < filter->internal->longValue)
                return -1;
            else if (value > filter->internal->longValue)
                return 1;
            else
                return 0;
        }
    }

    if (filter->internal->convertedToDouble) {
        bool propertyValueIsDouble = false;
        double value = celix_utils_convertStringToDouble(propertyValue, 0.0, &propertyValueIsDouble);
        if (propertyValueIsDouble) {
            if (value < filter->internal->doubleValue) {
                return -1;
            } else if (value > filter->internal->doubleValue) {
                return 1;
            } else {
                return 0;
            }
        }
    }

    if (filter->internal->convertedToVersion) {
        bool propertyValueIsVersion = false;
        celix_version_t *value = celix_utils_convertStringToVersion(propertyValue, NULL, &propertyValueIsVersion);
        if (propertyValueIsVersion) {
            int cmp = celix_version_compareTo(value, filter->internal->versionValue);
            celix_version_destroy(value);
            return cmp;
        }
    }

    //fallback on string compare
    return strcmp(propertyValue, filter->value);
}

static bool filter_compareSubString(const celix_filter_t* filter, const celix_properties_entry_t* entry) {
    const char* propertyValue = entry->value;
    int pos = 0;
    int size = celix_arrayList_size(filter->children);
    for (int i = 0; i < size; i++) {
        char * substr = (char *) celix_arrayList_get(filter->children, i);

        if (i + 1 < size) {
            if (substr == NULL) {
                unsigned int index;
                char * substr2 = (char *) celix_arrayList_get(filter->children, i + 1);
                if (substr2 == NULL) {
                        continue;
                }
                index = strcspn(propertyValue+pos, substr2);
                if (index == strlen(propertyValue+pos)) {
                        return false;
                }

                pos = index + strlen(substr2);
                if (i + 2 < size) {
                        i++;
                }
            } else {
                unsigned int len = strlen(substr);
                char * region = (char *)calloc(1, len+1); //TODO refactor filter compile to prevent calloc need
                strncpy(region, propertyValue+pos, len);
                region[len]    = '\0';
                if (strcmp(region, substr) == 0) {
                        pos += len;
                } else {
                        free(region);
                        return false;
                }
                free(region);
            }
        } else {
            unsigned int len;
            int begin;

            if (substr == NULL) {
                return true;
            }
            len = strlen(substr);
            begin = strlen(propertyValue)-len;
            return (strcmp(propertyValue+begin, substr) == 0);
        }
    }
    return true;
}

static bool celix_filter_matchPropertyEntry(const celix_filter_t* filter, const celix_properties_entry_t* entry) {
    if (filter == NULL || entry == NULL) {
        return false;
    }

    switch (filter->operand) {
        case CELIX_FILTER_OPERAND_SUBSTRING:
            return filter_compareSubString(filter, entry);
        case CELIX_FILTER_OPERAND_APPROX:
            return strcasecmp(entry->value, filter->value) == 0;
        case CELIX_FILTER_OPERAND_EQUAL:
            return celix_filter_compareAttributeValue(filter, entry) == 0;
        case CELIX_FILTER_OPERAND_GREATER:
            return celix_filter_compareAttributeValue(filter, entry) > 0;
        case CELIX_FILTER_OPERAND_GREATEREQUAL:
            return celix_filter_compareAttributeValue(filter, entry) >= 0;
        case CELIX_FILTER_OPERAND_LESS:
            return celix_filter_compareAttributeValue(filter, entry) < 0;
        case CELIX_FILTER_OPERAND_LESSEQUAL:
            return celix_filter_compareAttributeValue(filter, entry) <= 0;
        case CELIX_FILTER_OPERAND_AND:
        case CELIX_FILTER_OPERAND_NOT:
        case CELIX_FILTER_OPERAND_OR:
        case CELIX_FILTER_OPERAND_PRESENT:
        default:
            assert(false); //should not reach here
    }
    return false;
}

celix_status_t filter_getString(celix_filter_t * filter, const char **filterStr) {
    if (filter != NULL) {
        *filterStr = filter->filterStr;
    }
    return CELIX_SUCCESS;
}

celix_status_t filter_match_filter(celix_filter_t *src, celix_filter_t *dest, bool *out) {
    bool result = celix_filter_equals(src, dest);
    if (out != NULL) {
        *out = result;
    }
    return CELIX_SUCCESS;
}


celix_filter_t* celix_filter_create(const char *filterString) {
    celix_autofree char* str = celix_utils_strdup(filterString);
    if (!str) {
        celix_err_push("Failed to create filter string");
        // TODO test error
        return NULL;
    }

    int pos = 0;
    celix_autoptr(celix_filter_t) filter = filter_parseFilter(str, &pos);
    if (!filter) {
        celix_err_push("Failed to parse filter string");
        return NULL;
    }
    if (pos != strlen(filterString)) {
        celix_err_push("Filter Error: Extraneous trailing characters.\n");
        return NULL;
    }

    if (filter->operand != CELIX_FILTER_OPERAND_OR && filter->operand != CELIX_FILTER_OPERAND_AND &&
        filter->operand != CELIX_FILTER_OPERAND_NOT && filter->operand != CELIX_FILTER_OPERAND_SUBSTRING &&
        filter->operand != CELIX_FILTER_OPERAND_PRESENT) {
        if (filter->attribute == NULL || filter->value == NULL) {
            celix_err_push("Filter Error: Missing attribute or value.\n");
            return NULL;
        }
    }

    filter->filterStr = celix_steal_ptr(str);
    if (celix_filter_compile(filter) != CELIX_SUCCESS) {
        // TODO test error
        return NULL;
    }
    return celix_steal_ptr(filter);
}

void celix_filter_destroy(celix_filter_t* filter) {
    if (!filter) {
        return;
    }

    if (filter->children != NULL) {
        int size = celix_arrayList_size(filter->children);
        if (filter->operand == CELIX_FILTER_OPERAND_SUBSTRING) {
            for (int i = 0; i < size; i++) {
                char* operand = celix_arrayList_get(filter->children, i);
                free(operand);
            }
            celix_arrayList_destroy(filter->children);
            filter->children = NULL;
        } else if (filter->operand == CELIX_FILTER_OPERAND_OR || filter->operand == CELIX_FILTER_OPERAND_AND ||
                   filter->operand == CELIX_FILTER_OPERAND_NOT) {
            for (int i = 0; i < size; i++) {
                celix_filter_t* f = celix_arrayList_get(filter->children, i);
                celix_filter_destroy(f);
            }
            celix_arrayList_destroy(filter->children);
            filter->children = NULL;
        }
    }
    free((char*)filter->value);
    filter->value = NULL;
    free((char*)filter->attribute);
    filter->attribute = NULL;
    free((char*)filter->filterStr);
    filter->filterStr = NULL;
    if (filter->internal != NULL) {
        celix_version_destroy(filter->internal->versionValue);
        free(filter->internal);
    }
    free(filter);
}

bool celix_filter_match(const celix_filter_t* filter, const celix_properties_t* properties) {
    if (filter == NULL) {
        return true; // matching on null(empty) filter is always true
    }

    switch (filter->operand) {
    case CELIX_FILTER_OPERAND_AND: {
        celix_array_list_t* childern = filter->children;
        for (int i = 0; i < celix_arrayList_size(childern); i++) {
            celix_filter_t* childFilter = (celix_filter_t*)celix_arrayList_get(childern, i);
            bool childResult = celix_filter_match(childFilter, properties);
            if (!childResult) {
                return false;
            }
        }
        return true;
    }
    case CELIX_FILTER_OPERAND_OR: {
        celix_array_list_t* children = filter->children;
        for (int i = 0; i < celix_arrayList_size(children); i++) {
            celix_filter_t* childFilter = (celix_filter_t*)celix_arrayList_get(children, i);
            bool childResult = celix_filter_match(childFilter, properties);
            if (childResult) {
                return true;
            }
        }
        return false;
    }
    case CELIX_FILTER_OPERAND_NOT: {
        celix_filter_t* childFilter = celix_arrayList_get(filter->children, 0);
        bool childResult = celix_filter_match(childFilter, properties);
        return !childResult;
    }
    case CELIX_FILTER_OPERAND_PRESENT: {
        char* value = (properties == NULL) ? NULL : (char*)celix_properties_get(properties, filter->attribute, NULL);
        return value != NULL;
    }
    case CELIX_FILTER_OPERAND_SUBSTRING:
    case CELIX_FILTER_OPERAND_EQUAL:
    case CELIX_FILTER_OPERAND_GREATER:
    case CELIX_FILTER_OPERAND_GREATEREQUAL:
    case CELIX_FILTER_OPERAND_LESS:
    case CELIX_FILTER_OPERAND_LESSEQUAL:
    case CELIX_FILTER_OPERAND_APPROX: {
        const celix_properties_entry_t* entry = celix_properties_getEntry(properties, filter->attribute);
        bool result = celix_filter_matchPropertyEntry(filter, entry);
        return result;
    }
    }

    // LCOV_EXCL_START
    assert(false); // should never happen
    return false;
    // LCOV_EXCL_STOP
}

bool celix_filter_equals(const celix_filter_t *filter1, const celix_filter_t *filter2) {
    if (filter1 == filter2) {
        return true;
    }
    if (!filter1 || !filter2) {
        return false;
    }
    if (filter1->operand != filter2->operand) {
        return false;
    }


    if (filter1->operand == CELIX_FILTER_OPERAND_AND || filter1->operand == CELIX_FILTER_OPERAND_OR || filter1->operand == CELIX_FILTER_OPERAND_NOT) {
        assert(filter1->children != NULL);
        assert(filter2->children != NULL);
        int sizeSrc = celix_arrayList_size(filter1->children);
        int sizeDest = celix_arrayList_size(filter2->children);
        if (sizeSrc == sizeDest) {
            int i;
            int k;
            int sameCount = 0;
            for (i =0; i < sizeSrc; ++i) {
                bool same = false;
                celix_filter_t *srcPart = celix_arrayList_get(filter1->children, i);
                for (k = 0; k < sizeDest; ++k) {
                    celix_filter_t *destPart = celix_arrayList_get(filter2->children, k);
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

    //compare attr and value
    bool attrSame = false;
    bool valSame = false;
    if (filter1->attribute == NULL && filter2->attribute == NULL) {
        attrSame = true;
    } else if (filter1->attribute != NULL && filter2->attribute != NULL) {
        attrSame = celix_utils_stringEquals(filter1->attribute, filter2->attribute);
    }

    if (filter1->value == NULL  && filter2->value == NULL) {
        valSame = true;
    } else if (filter1->value != NULL && filter2->value != NULL) {
        valSame = celix_utils_stringEquals(filter1->value, filter2->value);
    }

    return attrSame && valSame;
}

const char* celix_filter_getFilterString(const celix_filter_t *filter) {
    if (filter != NULL) {
        return filter->filterStr;
    }
    return NULL;
}

const char* celix_filter_findAttribute(const celix_filter_t *filter, const char *attribute) {
    const char *result = NULL;
    if (filter != NULL && attribute != NULL) {
        if (filter->operand == CELIX_FILTER_OPERAND_AND || filter->operand == CELIX_FILTER_OPERAND_OR || filter->operand == CELIX_FILTER_OPERAND_NOT) {
            int size = celix_arrayList_size(filter->children);
            for (int i = 0; i < size; ++i) {
                celix_filter_t *child = celix_arrayList_get(filter->children, i);
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

static bool hasMandatoryEqualsValueAttribute(const celix_filter_t *filter, const char *attribute, bool negated, bool optional) {
    bool equalsValueAttribute = false;

    if (filter != NULL && attribute != NULL) {
        if (filter->operand == CELIX_FILTER_OPERAND_AND || filter->operand == CELIX_FILTER_OPERAND_OR || filter->operand == CELIX_FILTER_OPERAND_NOT) {
            int size = celix_arrayList_size(filter->children);
            for (int i = 0; i < size; ++i) {

                if (filter->operand == CELIX_FILTER_OPERAND_NOT) {
                    negated = !negated;
                } else if (filter->operand == CELIX_FILTER_OPERAND_OR) {
                    optional = true;
                }

                celix_filter_t *child = celix_arrayList_get(filter->children, i);

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

bool celix_filter_hasMandatoryEqualsValueAttribute(const celix_filter_t *filter, const char *attribute) {
    return hasMandatoryEqualsValueAttribute(filter, attribute, false, false);
}

static bool hasMandatoryNegatedPresenceAttribute(const celix_filter_t *filter, const char *attribute, bool negated, bool optional) {
    bool negatedPresenceAttribute = false;

    if (filter != NULL && attribute != NULL) {
        if (filter->operand == CELIX_FILTER_OPERAND_AND || filter->operand == CELIX_FILTER_OPERAND_OR || filter->operand == CELIX_FILTER_OPERAND_NOT) {
            int size = celix_arrayList_size(filter->children);
            for (int i = 0; i < size; ++i) {

                if (filter->operand == CELIX_FILTER_OPERAND_NOT) {
                    negated = !negated;
                } else if (filter->operand == CELIX_FILTER_OPERAND_OR) {
                    optional = true;
                }

                celix_filter_t *child = celix_arrayList_get(filter->children, i);

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
bool celix_filter_hasMandatoryNegatedPresenceAttribute(const celix_filter_t *filter, const char *attribute) {
    return hasMandatoryNegatedPresenceAttribute(filter, attribute, false, false);
}
