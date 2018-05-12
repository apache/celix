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
 * array_list.h
 *
 *  \date       Aug 4, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef ARRAY_LIST_H_
#define ARRAY_LIST_H_

#include "celixbool.h"
#include "exports.h"
#include "celix_errno.h"
#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef union celix_array_list_entry {
    void *voidPtrVal;
    int intVal;
    long int longVal;
    unsigned int uintVal;
    unsigned long ulongVal;
    double doubleVal;
    float floatVal;
    bool boolVal;
    size_t sizeVal;
} celix_array_list_entry_t;

typedef struct celix_arrayList celix_array_list_t;

typedef struct celix_arrayList *array_list_pt;
typedef struct celix_arrayList array_list_t;

typedef struct arrayListIterator *array_list_iterator_pt;
typedef struct arrayListIterator array_list_iterator_t;

typedef celix_status_t (*array_list_element_equals_pt)(const void *, const void *, bool *equals);

typedef bool (*celix_arrayList_equals_fp)(celix_array_list_entry_t, celix_array_list_entry_t);

UTILS_EXPORT celix_status_t arrayList_create(array_list_pt *list);

UTILS_EXPORT celix_status_t arrayList_createWithEquals(array_list_element_equals_pt equals, array_list_pt *list);

UTILS_EXPORT void arrayList_destroy(array_list_pt list);

UTILS_EXPORT void arrayList_trimToSize(array_list_pt list);

UTILS_EXPORT void arrayList_ensureCapacity(array_list_pt list, int capacity);

UTILS_EXPORT unsigned int arrayList_size(array_list_pt list);

UTILS_EXPORT bool arrayList_isEmpty(array_list_pt list);

UTILS_EXPORT bool arrayList_contains(array_list_pt list, void *element);

UTILS_EXPORT int arrayList_indexOf(array_list_pt list, void *element);

UTILS_EXPORT int arrayList_lastIndexOf(array_list_pt list, void *element);

UTILS_EXPORT void *arrayList_get(array_list_pt list, unsigned int index);

UTILS_EXPORT void *arrayList_set(array_list_pt list, unsigned int index, void *element);

UTILS_EXPORT bool arrayList_add(array_list_pt list, void *element);

UTILS_EXPORT int arrayList_addIndex(array_list_pt list, unsigned int index, void *element);

UTILS_EXPORT bool arrayList_addAll(array_list_pt list, array_list_pt toAdd);

UTILS_EXPORT void *arrayList_remove(array_list_pt list, unsigned int index);

UTILS_EXPORT bool arrayList_removeElement(array_list_pt list, void *element);

UTILS_EXPORT void arrayList_clear(array_list_pt list);

UTILS_EXPORT array_list_pt arrayList_clone(array_list_pt list);

UTILS_EXPORT array_list_iterator_pt arrayListIterator_create(array_list_pt list);

UTILS_EXPORT void arrayListIterator_destroy(array_list_iterator_pt iterator);

UTILS_EXPORT bool arrayListIterator_hasNext(array_list_iterator_pt iterator);

UTILS_EXPORT void *arrayListIterator_next(array_list_iterator_pt iterator);

UTILS_EXPORT bool arrayListIterator_hasPrevious(array_list_iterator_pt iterator);

UTILS_EXPORT void *arrayListIterator_previous(array_list_iterator_pt iterator);

UTILS_EXPORT void arrayListIterator_remove(array_list_iterator_pt iterator);



/**********************************************************************************************************************
 **********************************************************************************************************************
 * Updated API
 **********************************************************************************************************************
 **********************************************************************************************************************/


celix_array_list_t* celix_arrayList_create();

celix_array_list_t* celix_arrayList_createWithEquals(celix_arrayList_equals_fp equals);

void celix_arrayList_destroy(celix_array_list_t *list);

size_t celix_arrayList_size(const celix_array_list_t *list);

void* celix_arrayList_get(const celix_array_list_t *list, int index);

//more of the same for the different entry types
int celix_arrayList_getInt(const celix_array_list_t *list, int index);
long int celix_arrayList_getLong(const celix_array_list_t *list, int index);
unsigned int celix_arrayList_getUInt(const celix_array_list_t *list, int index);
unsigned long int celix_arrayList_getULong(const celix_array_list_t *list, int index);
float celix_arrayList_getFloat(const celix_array_list_t *list, int index);
double celix_arrayList_getDouble(const celix_array_list_t *list, int index);
bool celix_arrayList_getBool(const celix_array_list_t *list, int index);
size_t celix_arrayList_getSize(const celix_array_list_t *list, int index);

void celix_arrayList_add(celix_array_list_t *list, void* val);

//more of the same for the different entry types
void celix_arrayList_addInt(celix_array_list_t *list, int val);
void celix_arrayList_addLong(celix_array_list_t *list, long val);
void celix_arrayList_addUInt(celix_array_list_t *list, unsigned int val);
void celix_arrayList_addULong(celix_array_list_t *list, unsigned long val);
void celix_arrayList_addFloat(celix_array_list_t *list, float val);
void celix_arrayList_addDouble(celix_array_list_t *list, double val);
void celix_arrayList_addBool(celix_array_list_t *list, bool val);
void celix_arrayList_addSize(celix_array_list_t *list, size_t val);

int celix_arrayList_indexOf(celix_array_list_t *list, celix_array_list_entry_t entry);
void celix_arrayList_remove(celix_array_list_t *list, int index);

/**
 * Remove entry from array list. To use this first memset the entry to null to ensure it completely initialized or
 * ensure that the array list is created with a custom equals which matches the used entry.
 */
void celix_arrayList_removeEntry(celix_array_list_t *list, celix_array_list_entry_t entry);

#ifdef __cplusplus
}
#endif
#endif /* ARRAY_LIST_H_ */
