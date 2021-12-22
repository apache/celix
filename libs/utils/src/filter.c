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

#include "celix_filter.h"
#include "filter.h"
#include "celix_errno.h"

static void filter_skipWhiteSpace(char* filterString, int* pos);
static celix_filter_t * filter_parseFilter(char* filterString, int* pos);
static celix_filter_t * filter_parseFilterComp(char* filterString, int* pos);
static celix_filter_t * filter_parseAndOrOr(char* filterString, celix_filter_operand_t andOrOr, int* pos);
static celix_filter_t * filter_parseNot(char* filterString, int* pos);
static celix_filter_t * filter_parseItem(char* filterString, int* pos);
static char * filter_parseAttr(char* filterString, int* pos);
static char * filter_parseValue(char* filterString, int* pos);
static celix_array_list_t* filter_parseSubstring(char* filterString, int* pos);

static celix_status_t filter_compare(const celix_filter_t* filter, const char *propertyValue, bool *result);

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

static celix_filter_t * filter_parseFilter(char * filterString, int * pos) {
    celix_filter_t * filter;
    filter_skipWhiteSpace(filterString, pos);
    if (filterString[*pos] == '\0') {
        //empty filter
        fprintf(stderr, "Filter Error: Cannot create LDAP filter from an empty string.\n");
        return NULL;
    } else if (filterString[*pos] != '(') {
        fprintf(stderr, "Filter Error: Missing '(' in filter string '%s'.\n", filterString);
        return NULL;
    }
    (*pos)++; //eat '('

    filter = filter_parseFilterComp(filterString, pos);

    filter_skipWhiteSpace(filterString, pos);

    if (filterString[*pos] != ')') {
        fprintf(stderr, "Filter Error: Missing ')' in filter string '%s'.\n", filterString);
        if(filter!=NULL){
            filter_destroy(filter);
        }
        return NULL;
    }
    (*pos)++; //eat ')'
    filter_skipWhiteSpace(filterString, pos);

    return filter;
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
        children = NULL;
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
                    celix_filter_t * filter = (celix_filter_t *) calloc(1, sizeof(*filter));
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

celix_status_t filter_match(celix_filter_t * filter, celix_properties_t *properties, bool *out) {
    bool result = celix_filter_match(filter, properties);
    if (out != NULL) {
        *out = result;
    }
    return CELIX_SUCCESS;
}

static celix_status_t filter_compare(const celix_filter_t* filter, const char *propertyValue, bool *out) {
    celix_status_t  status = CELIX_SUCCESS;
    bool result = false;

    if (filter == NULL || propertyValue == NULL) {
        *out = false;
        return status;
    }

    switch (filter->operand) {
        case CELIX_FILTER_OPERAND_SUBSTRING: {
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
                            *out = false;
                            return CELIX_SUCCESS;
                        }

                        pos = index + strlen(substr2);
                        if (i + 2 < size) {
                            i++;
                        }
                    } else {
                        unsigned int len = strlen(substr);
                        char * region = (char *)calloc(1, len+1);
                        strncpy(region, propertyValue+pos, len);
                        region[len]    = '\0';
                        if (strcmp(region, substr) == 0) {
                            pos += len;
                        } else {
                            free(region);
                            *out = false;
                            return CELIX_SUCCESS;
                        }
                        free(region);
                    }
                } else {
                    unsigned int len;
                    int begin;

                    if (substr == NULL) {
                        *out = true;
                        return CELIX_SUCCESS;
                    }
                    len = strlen(substr);
                    begin = strlen(propertyValue)-len;
                    *out = (strcmp(propertyValue+begin, substr) == 0);
                    return CELIX_SUCCESS;
                }
            }
            *out = true;
            return CELIX_SUCCESS;
        }
        case CELIX_FILTER_OPERAND_APPROX: //TODO: Implement strcmp with ignorecase and ignorespaces
        case CELIX_FILTER_OPERAND_EQUAL: {
            *out = (strcmp(propertyValue, filter->value) == 0);
            return CELIX_SUCCESS;
        }
        case CELIX_FILTER_OPERAND_GREATER: {
            *out = (strcmp(propertyValue, filter->value) > 0);
            return CELIX_SUCCESS;
        }
        case CELIX_FILTER_OPERAND_GREATEREQUAL: {
            *out = (strcmp(propertyValue, filter->value) >= 0);
            return CELIX_SUCCESS;
        }
        case CELIX_FILTER_OPERAND_LESS: {
            *out = (strcmp(propertyValue, filter->value) < 0);
            return CELIX_SUCCESS;
        }
        case CELIX_FILTER_OPERAND_LESSEQUAL: {
            *out = (strcmp(propertyValue, filter->value) <= 0);
            return CELIX_SUCCESS;
        }
        case CELIX_FILTER_OPERAND_AND:
        case CELIX_FILTER_OPERAND_NOT:
        case CELIX_FILTER_OPERAND_OR:
        case CELIX_FILTER_OPERAND_PRESENT: {
        }
            /* no break */
    }

    if (status == CELIX_SUCCESS && out != NULL) {
        *out = result;
    }
    return status;
}

celix_status_t filter_getString(celix_filter_t * filter, const char **filterStr) {
    if (filter != NULL) {
        *filterStr = filter->filterStr;
    }
    return CELIX_SUCCESS;
}

celix_status_t filter_match_filter(celix_filter_t *src, celix_filter_t *dest, bool *out) {
    bool result = celix_filter_matchFilter(src, dest);
    if (out != NULL) {
        *out = result;
    }
    return CELIX_SUCCESS;
}


celix_filter_t* celix_filter_create(const char *filterString) {
    celix_filter_t * filter = NULL;
    char* filterStr = string_ndup(filterString, 1024*1024);
    int pos = 0;
    filter = filter_parseFilter(filterStr, &pos);
    if (filter != NULL && pos != strlen(filterStr)) {
        fprintf(stderr, "Filter Error: Extraneous trailing characters.\n");
        filter_destroy(filter);
        filter = NULL;
    } else if (filter != NULL) {
        if (filter->operand != CELIX_FILTER_OPERAND_OR && filter->operand != CELIX_FILTER_OPERAND_AND &&
            filter->operand != CELIX_FILTER_OPERAND_NOT && filter->operand != CELIX_FILTER_OPERAND_SUBSTRING &&
            filter->operand != CELIX_FILTER_OPERAND_PRESENT) {
            if (filter->attribute == NULL || filter->value == NULL) {
                filter_destroy(filter);
                filter = NULL;
            }
        }
    }

    if (filter == NULL) {
        free(filterStr);
    } else {
        filter->filterStr = filterStr;
    }

    return filter;
}

void celix_filter_destroy(celix_filter_t *filter) {
    if (filter != NULL) {
        if(filter->children != NULL){
            if (filter->operand == CELIX_FILTER_OPERAND_SUBSTRING) {
                int size = celix_arrayList_size(filter->children);
                int i = 0;
                for (i = 0; i < size; i++) {
                    char *operand = celix_arrayList_get(filter->children, i);
                    free(operand);
                }
                celix_arrayList_destroy(filter->children);
                filter->children = NULL;
            } else if (filter->operand == CELIX_FILTER_OPERAND_OR || filter->operand == CELIX_FILTER_OPERAND_AND || filter->operand == CELIX_FILTER_OPERAND_NOT) {
                int size = celix_arrayList_size(filter->children);
                int i = 0;
                for (i = 0; i < size; i++) {
                    celix_filter_t *f = celix_arrayList_get(filter->children, i);
                    filter_destroy(f);
                }
                celix_arrayList_destroy(filter->children);
                filter->children = NULL;
            } else {
                fprintf(stderr, "Filter Error: Corrupt filter. children has a value, but not an expected operand\n");
            }
        }
        free((char*)filter->value);
        filter->value = NULL;
        free((char*)filter->attribute);
        filter->attribute = NULL;
        free((char*)filter->filterStr);
        filter->filterStr = NULL;
        free(filter);
    }
}

bool celix_filter_match(const celix_filter_t *filter, const celix_properties_t* properties) {
    if (filter == NULL) {
        return true; //matching on null(empty) filter is always true
    }
    bool result = false;
    switch (filter->operand) {
        case CELIX_FILTER_OPERAND_AND: {
            celix_array_list_t* children = filter->children;
            for (int i = 0; i < celix_arrayList_size(children); i++) {
                celix_filter_t * sfilter = (celix_filter_t *) celix_arrayList_get(children, i);
                bool mresult = celix_filter_match(sfilter, properties);
                if (!mresult) {
                    return false;
                }
            }
            return true;
        }
        case CELIX_FILTER_OPERAND_OR: {
            celix_array_list_t* children = filter->children;
            for (int i = 0; i < celix_arrayList_size(children); i++) {
                celix_filter_t * sfilter = (celix_filter_t *) celix_arrayList_get(children, i);
                bool mresult = celix_filter_match(sfilter, properties);
                if (mresult) {
                    return true;
                }
            }
            return false;
        }
        case CELIX_FILTER_OPERAND_NOT: {
            celix_filter_t * sfilter = celix_arrayList_get(filter->children, 0);
            bool mresult = celix_filter_match(sfilter, properties);
            return !mresult;
        }
        case CELIX_FILTER_OPERAND_SUBSTRING :
        case CELIX_FILTER_OPERAND_EQUAL :
        case CELIX_FILTER_OPERAND_GREATER :
        case CELIX_FILTER_OPERAND_GREATEREQUAL :
        case CELIX_FILTER_OPERAND_LESS :
        case CELIX_FILTER_OPERAND_LESSEQUAL :
        case CELIX_FILTER_OPERAND_APPROX : {
            char * value = (properties == NULL) ? NULL: (char*)celix_properties_get(properties, filter->attribute, NULL);
            filter_compare(filter, value, &result);
            return result;
        }
        case CELIX_FILTER_OPERAND_PRESENT: {
            char * value = (properties == NULL) ? NULL: (char*)celix_properties_get(properties, filter->attribute, NULL);
            return value != NULL;
        }
    }
    return result;
}

bool celix_filter_matchFilter(const celix_filter_t *filter1, const celix_filter_t *filter2) {
    bool result = false;
    if (filter1 == filter2) {
        result = true; //NOTE. also means NULL filter are equal
    } else if (filter1 != NULL && filter2 != NULL && filter1->operand == filter2->operand) {
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
                result = sameCount == sizeSrc;
            }
        } else { //compare attr and value
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

            result = attrSame && valSame;
        }
    }

    return result;
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