/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */
/*
 * filter.h
 *
 *  \date       Apr 28, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef FILTER_H_
#define FILTER_H_

#include "celix_errno.h"
#include "properties.h"
#include "celixbool.h"
#include "framework_exports.h"
#include "array_list.h"

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

typedef struct celix_filter_struct filter_t; //deprecated
typedef struct celix_filter_struct *filter_pt; //deprecated

typedef struct celix_filter_struct celix_filter_t;

struct celix_filter_struct {
    celix_filter_operand_t operand;
    char *attribute; //NULL for operands AND, OR ot NOT
    char *value; //NULL for operands AND, OR or NOT NOT
    char *filterStr;

    //type is celix_filter_t* for AND, OR and NOT operator and char* for SUBSTRING
    //for other operands childern is NULL
    array_list_t *children;
};

FRAMEWORK_EXPORT celix_filter_t* filter_create(const char *filterString);

FRAMEWORK_EXPORT void filter_destroy(celix_filter_t *filter);

FRAMEWORK_EXPORT celix_status_t filter_match(celix_filter_t *filter, properties_t *properties, bool *result);

FRAMEWORK_EXPORT celix_status_t filter_match_filter(celix_filter_t *src, filter_t *dest, bool *result);

FRAMEWORK_EXPORT celix_status_t filter_getString(celix_filter_t *filter, const char **filterStr);

FRAMEWORK_EXPORT celix_status_t filter_getString(celix_filter_t *filter, const char **filterStr);

#ifdef __cplusplus
}
#endif

#endif /* FILTER_H_ */
