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

#include "celixbool.h"
#include "exports.h"
#include "celix_errno.h"
#include "stdbool.h"

#ifndef CELIX_ARRAY_LIST_H_
#define CELIX_ARRAY_LIST_H_

/**
 * Init macro so that the opts are correctly initialized for C++ compilers
 */
#ifdef __cplusplus
#define CELIX_OPTS_INIT {}
#else
#define CELIX_OPTS_INIT
#endif


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

typedef struct celix_array_list celix_array_list_t;

typedef bool (*celix_arrayList_equals_fp)(celix_array_list_entry_t, celix_array_list_entry_t);

typedef int (*celix_arrayList_sort_fp)(const void *, const void *);


/**
 * Additional create options when creating a array list.
 */
typedef struct celix_array_list_create_options {
    /**
     * Equals function, used when removing items with celix_arrayList_removeEntry.
     * TODO maybe allow a different (better) equals here:
     * bool (*equals)(celix_array_list_entry_t,celix_array_list_entry_t)
     */
    celix_arrayList_equals_fp equals CELIX_OPTS_INIT;

    /**
     * A simple removed callback, which if provided will be called if a entry is removed
     * from the array list. The removed entry is provided as pointer.
     *
     * @note Assumes that the array list entries are pointer values.
     *
     * @note only simpleRemovedCallback or removedCallback should be configured, if both as configured,
     * only the simpledRemoveCallback will be used.
     */
    void (*simpleRemovedCallback)(void* value) CELIX_OPTS_INIT;

    /**
     * Optional callback data, which will be provided to the removedCallback callback.
     */
    void* removedCallbackData CELIX_OPTS_INIT;

    /**
     * A removed callback, which if provided will be called if a entry is removed from the array list.
     * The callback data pointer will be provided as first argument to the removed callback.
     *
     * @note only simpleRemovedCallback or removedCallback should be configured, if both as configured,
     * only the simpledRemoveCallback will be used.
     */
    void (*removedCallback)(void* data, celix_array_list_entry_t entry) CELIX_OPTS_INIT;
} celix_array_list_create_options_t;

/**
 * @brief C Macro to create a empty string_hash_map_create_options_t type.
 */
#ifndef __cplusplus
#define CELIX_EMPTY_HASH_MAP_CREATE_OPTIONS {       \
    .equals                                         \
    .simpleRemovedCallback = NULL,                  \
    .removedCallbackData = NULL,                    \
    .removedCallback = NULL                         \
}
#endif


//TODO document that entry of an array should be of the same type
celix_array_list_t* celix_arrayList_create();

celix_array_list_t* celix_arrayList_createWithEquals(celix_arrayList_equals_fp equals);

//TODO impl and test
celix_array_list_t* celix_arrayList_createWithOptions(const celix_array_list_create_options_t* opts);

void celix_arrayList_destroy(celix_array_list_t *list);

int celix_arrayList_size(const celix_array_list_t *list);

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
void celix_arrayList_removeAt(celix_array_list_t *list, int index);

void celix_arrayList_clear(celix_array_list_t *list);

/**
 * Remove entry from array list. To use this first memset the entry to null to ensure it completely initialized or
 * ensure that the array list is created with a custom equals which matches the used entry.
 */
void celix_arrayList_removeEntry(celix_array_list_t *list, celix_array_list_entry_t entry);

void celix_arrayList_remove(celix_array_list_t *list, void *ptr);
void celix_arrayList_removeInt(celix_array_list_t *list, int val);
void celix_arrayList_removeLong(celix_array_list_t *list, long val);
void celix_arrayList_removeUInt(celix_array_list_t *list, unsigned int val);
void celix_arrayList_removeULong(celix_array_list_t *list, unsigned long val);
void celix_arrayList_removeFloat(celix_array_list_t *list, float val);
void celix_arrayList_removeDouble(celix_array_list_t *list, double val);
void celix_arrayList_removeBool(celix_array_list_t *list, bool val);
void celix_arrayList_removeSize(celix_array_list_t *list, size_t val);

/**
 * @warning Never use this function with array of doubles, since on some 32-bit platform (sizeof(double)==8 && sizeof(void*)==4)
 */
void celix_arrayList_sort(celix_array_list_t *list, celix_arrayList_sort_fp sortFp);

#ifdef __cplusplus
}
#endif

#undef CELIX_OPTS_INIT

#endif /* CELIX_ARRAY_LIST_H_ */
