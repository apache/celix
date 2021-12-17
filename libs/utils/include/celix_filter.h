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

#ifndef CELIX_FILTER_H_
#define CELIX_FILTER_H_

#include "celix_properties.h"
#include "celix_array_list.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum celix_filter_operand_enum
{
    CELIX_FILTER_OPERAND_EQUAL,
    CELIX_FILTER_OPERAND_APPROX,
    CELIX_FILTER_OPERAND_GREATER,
    CELIX_FILTER_OPERAND_GREATEREQUAL,
    CELIX_FILTER_OPERAND_LESS,
    CELIX_FILTER_OPERAND_LESSEQUAL,
    CELIX_FILTER_OPERAND_PRESENT,
    CELIX_FILTER_OPERAND_SUBSTRING,
    CELIX_FILTER_OPERAND_AND,
    CELIX_FILTER_OPERAND_OR,
    CELIX_FILTER_OPERAND_NOT,
} celix_filter_operand_t;

typedef struct celix_filter_struct celix_filter_t;

struct celix_filter_struct {
    celix_filter_operand_t operand;
    const char *attribute; //NULL for operands AND, OR ot NOT
    const char *value; //NULL for operands AND, OR or NOT NOT
    const char *filterStr;

    //type is celix_filter_t* for AND, OR and NOT operator and char* for SUBSTRING
    //for other operands children is NULL
    celix_array_list_t *children;
};



celix_filter_t* celix_filter_create(const char *filterStr);

void celix_filter_destroy(celix_filter_t *filter);

bool celix_filter_match(const celix_filter_t *filter, const celix_properties_t* props);

bool celix_filter_matchFilter(const celix_filter_t *filter1, const celix_filter_t *filter2);

const char* celix_filter_getFilterString(const celix_filter_t *filter);

/**
 * Find the filter attribute.
 * @return The attribute value or NULL if the attribute is not found.
 */
const char* celix_filter_findAttribute(const celix_filter_t *filter, const char *attribute);

/**
 * Check whether the filter indicates the mandatory presence of an attribute with a specific value for the provided attribute key.
 *
 * Example:
 *   using this function for attribute key "key1" on filter "(key1=value1)" yields true.
 *   using this function for attribute key "key1" on filter "(!(key1=value1))" yields false.
 *   using this function for attribute key "key1" on filter "(key1>=value1)" yields false.
 *   using this function for attribute key "key1" on filter "(|(key1=value1)(key2=value2))" yields false.
*/
bool celix_filter_hasMandatoryEqualsValueAttribute(const celix_filter_t *filter, const char *attribute);

/**
 * Chek whether the filter indicates the mandatory absence of an attribute, regardless of its value, for the provided attribute key.
 *
 * example:
 *   using this function for attribute key "key1" on the filter "(!(key1=*))" yields true.
 *   using this function for attribute key "key1" on the filter "(key1=*) yields false.
 *   using this function for attribute key "key1" on the filter "(key1=value)" yields false.
 *   using this function for attribute key "key1" on the filter "(|(!(key1=*))(key2=value2))" yields false.
 */
bool celix_filter_hasMandatoryNegatedPresenceAttribute(const celix_filter_t *filter, const char *attribute);

#ifdef __cplusplus
}
#endif

#endif /* CELIX_FILTER_H_ */
