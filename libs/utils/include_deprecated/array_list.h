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

#ifndef ARRAY_LIST_H_
#define ARRAY_LIST_H_

#include "celixbool.h"
#include "exports.h"
#include "celix_errno.h"
#include "stdbool.h"
#include "celix_array_list.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ADD_CELIX_DEPRECATED_WARNING
#define CELIX_DEPRECATED_ATTR __attribute__ ((deprecated))
#else
#define CELIX_DEPRECATED_ATTR
#endif

typedef struct celix_array_list *array_list_pt CELIX_DEPRECATED_ATTR;
typedef struct celix_array_list array_list_t CELIX_DEPRECATED_ATTR;

typedef struct celix_array_list_iterator *array_list_iterator_pt;
typedef struct celix_array_list_iterator array_list_iterator_t;

typedef celix_status_t (*array_list_element_equals_pt)(const void *, const void *, bool *equals);

UTILS_EXPORT celix_status_t arrayList_create(celix_array_list_t **list);

UTILS_EXPORT celix_status_t arrayList_createWithEquals(array_list_element_equals_pt equals, celix_array_list_t **list);

UTILS_EXPORT void arrayList_destroy(celix_array_list_t *list);

UTILS_EXPORT void arrayList_trimToSize(celix_array_list_t *list);

UTILS_EXPORT void arrayList_ensureCapacity(celix_array_list_t *list, int capacity);

UTILS_EXPORT unsigned int arrayList_size(celix_array_list_t *list);

UTILS_EXPORT bool arrayList_isEmpty(celix_array_list_t *list);

UTILS_EXPORT bool arrayList_contains(celix_array_list_t *list, void *element);

UTILS_EXPORT int arrayList_indexOf(celix_array_list_t *list, void *element);

UTILS_EXPORT int arrayList_lastIndexOf(celix_array_list_t *list, void *element);

UTILS_EXPORT void *arrayList_get(celix_array_list_t *list, unsigned int index);

UTILS_EXPORT void *arrayList_set(celix_array_list_t *list, unsigned int index, void *element);

UTILS_EXPORT bool arrayList_add(celix_array_list_t *list, void *element);

UTILS_EXPORT int arrayList_addIndex(celix_array_list_t *list, unsigned int index, void *element);

UTILS_EXPORT bool arrayList_addAll(celix_array_list_t *list, celix_array_list_t *toAdd);

UTILS_EXPORT void *arrayList_remove(celix_array_list_t *list, unsigned int index);

UTILS_EXPORT bool arrayList_removeElement(celix_array_list_t *list, void *element);

UTILS_EXPORT void arrayList_clear(celix_array_list_t *list);

UTILS_EXPORT celix_array_list_t *arrayList_clone(celix_array_list_t *list);

UTILS_EXPORT array_list_iterator_pt arrayListIterator_create(celix_array_list_t *list);

UTILS_EXPORT void arrayListIterator_destroy(array_list_iterator_pt iterator);

UTILS_EXPORT bool arrayListIterator_hasNext(array_list_iterator_pt iterator);

UTILS_EXPORT void *arrayListIterator_next(array_list_iterator_pt iterator);

UTILS_EXPORT bool arrayListIterator_hasPrevious(array_list_iterator_pt iterator);

UTILS_EXPORT void *arrayListIterator_previous(array_list_iterator_pt iterator);

UTILS_EXPORT void arrayListIterator_remove(array_list_iterator_pt iterator);

#ifdef __cplusplus
}
#endif
#endif /* ARRAY_LIST_H_ */
