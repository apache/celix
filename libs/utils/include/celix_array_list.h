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

typedef int (*celix_array_list_sort_entries_fp)(celix_array_list_entry_t a, celix_array_list_entry_t b);


/**
 * Additional create options when creating a array list.
 */
typedef struct celix_array_list_create_options {
    /**
     * A simple removed callback, which if provided will be called if a entry is removed
     * from the array list. The removed entry is provided as pointer.
     *
     * @note Assumes that the array list entries are pointer values.
     *
     * @note only simpleRemovedCallback or removedCallback should be configured, if both are configured,
     * only the simpledRemoveCallback will be used.
     *
     * Default is NULL.
     */
    void (*simpleRemovedCallback)(void* value) CELIX_OPTS_INIT;

    /**
     * Optional callback data, which will be provided to the removedCallback callback.
     *
     * Default is NULL.
     */
    void* removedCallbackData CELIX_OPTS_INIT;

    /**
     * A removed callback, which if provided will be called if a entry is removed from the array list.
     * The callback data pointer will be provided as first argument to the removed callback.
     *
     * @note only simpleRemovedCallback or removedCallback should be configured, if both are configured,
     * only the simpledRemoveCallback will be used.
     *
     * Default is NULL.
     */
    void (*removedCallback)(void* data, celix_array_list_entry_t entry) CELIX_OPTS_INIT;

    /**
     * Equals callback used when trying to find a array list entry.
     */
    bool (*equalsCallback)(celix_array_list_entry_t a, celix_array_list_entry_t b) CELIX_OPTS_INIT;

} celix_array_list_create_options_t;

#ifndef __cplusplus
/**
 * @brief C Macro to create a empty string_hash_map_create_options_t type.
 */
#define CELIX_EMPTY_ARRAY_LIST_CREATE_OPTIONS {     \
    .simpleRemovedCallback = NULL,                  \
    .removedCallbackData = NULL,                    \
    .removedCallback = NULL,                        \
    .equalsCallback = NULL                          \
}
#endif

/**
 * @brief Creates a new empty array list.
 */
celix_array_list_t* celix_arrayList_create();

/**
 * @brief Creates a new empty array list, which uses the provided equals to check whether entries
 * are equal.
 * @deprecated This functions is deprecated, use celix_arrayList_createWithOptions instead.
 */
celix_array_list_t* celix_arrayList_createWithEquals(celix_arrayList_equals_fp equals);

/**
 * @brief Creates a new empty array listusing using the provided array list create options.
 * @param opts The create options, only used during the creation of the array list.
 */
celix_array_list_t* celix_arrayList_createWithOptions(const celix_array_list_create_options_t* opts);

/**
 * @brief Clears and then deallocated the array list.
 *
 * @note If a (simple) removed callback is configured, the callback will be called for every array list entry.
 */
void celix_arrayList_destroy(celix_array_list_t *list);

/**
 * @brief Returns the size of the array list.
 */
int celix_arrayList_size(const celix_array_list_t *list);

/**
 * @brief Returns the value for the provided index.
 *
 * @param map The array list.
 * @param index The entry index to return.
 * @return Returns the pointer value for the index. Returns NULL if index is out of bound.
 */
void* celix_arrayList_get(const celix_array_list_t *list, int index);

/**
 * @brief Returns the value for the provided index.
 *
 * @param map The array list.
 * @param index The entry index to return.
 * @return Returns the int value for the index. Returns 0 if index is out of bound.
 */
int celix_arrayList_getInt(const celix_array_list_t *list, int index);

/**
 * @brief Returns the value for the provided index.
 *
 * @param map The array list.
 * @param index The entry index to return.
 * @return Returns the long value for the index. Returns 0 if index is out of bound.
 */
long int celix_arrayList_getLong(const celix_array_list_t *list, int index);

/**
 * @brief Returns the value for the provided index.
 *
 * @param map The array list.
 * @param index The entry index to return.
 * @return Returns the unsigned int value for the index. Returns 0 if index is out of bound.
 */
unsigned int celix_arrayList_getUInt(const celix_array_list_t *list, int index);

/**
 * @brief Returns the value for the provided index.
 *
 * @param map The array list.
 * @param index The entry index to return.
 * @return Returns the unsigned long value for the index. Returns 0 if index is out of bound.
 */
unsigned long int celix_arrayList_getULong(const celix_array_list_t *list, int index);

/**
 * @brief Returns the value for the provided index.
 *
 * @param map The array list.
 * @param index The entry index to return.
 * @return Returns the float value for the index. Returns 0 if index is out of bound.
 */
float celix_arrayList_getFloat(const celix_array_list_t *list, int index);

/**
 * @brief Returns the value for the provided index.
 *
 * @param map The array list.
 * @param index The entry index to return.
 * @return Returns the double value for the index. Returns 0 if index is out of bound.
 */
double celix_arrayList_getDouble(const celix_array_list_t *list, int index);

/**
 * @brief Returns the value for the provided index.
 *
 * @param map The array list.
 * @param index The entry index to return.
 * @return Returns the bool value for the index. Returns false if index is out of bound.
 */
bool celix_arrayList_getBool(const celix_array_list_t *list, int index);

/**
 * @brief Returns the value for the provided index.
 *
 * @param map The array list.
 * @param index The entry index to return.
 * @return Returns the size_t value for the index. Returns 0 if index is out of bound.
 */
size_t celix_arrayList_getSize(const celix_array_list_t *list, int index);

/**
 * @brief add pointer entry to the back of the array list.
 *
 * @param map The array list.
 * @param value The pointer value to add to the array list.
 */
void celix_arrayList_add(celix_array_list_t *list, void* value);

/**
 * @brief add pointer entry to the back of the array list.
 *
 * @param map The array list.
 * @param value The int value to add to the array list.
 */
void celix_arrayList_addInt(celix_array_list_t *list, int value);

/**
 * @brief add pointer entry to the back of the array list.
 *
 * @param map The array list.
 * @param value The long value to add to the array list.
 */
void celix_arrayList_addLong(celix_array_list_t *list, long value);

/**
 * @brief add pointer entry to the back of the array list.
 *
 * @param map The array list.
 * @param value The unsigned int value to add to the array list.
 */
void celix_arrayList_addUInt(celix_array_list_t *list, unsigned int value);

/**
 * @brief add pointer entry to the back of the array list.
 *
 * @param map The array list.
 * @param value The unsigned long value to add to the array list.
 */
void celix_arrayList_addULong(celix_array_list_t *list, unsigned long value);

/**
 * @brief add pointer entry to the back of the array list.
 *
 * @param map The array list.
 * @param value The float value to add to the array list.
 */
void celix_arrayList_addFloat(celix_array_list_t *list, float value);

/**
 * @brief add pointer entry to the back of the array list.
 *
 * @param map The array list.
 * @param value The double value to add to the array list.
 */
void celix_arrayList_addDouble(celix_array_list_t *list, double value);

/**
 * @brief add pointer entry to the back of the array list.
 *
 * @param map The array list.
 * @param value The bool value to add to the array list.
 */
void celix_arrayList_addBool(celix_array_list_t *list, bool value);

/**
 * @brief add pointer entry to the back of the array list.
 *
 * @param map The array list.
 * @param value The size_t value to add to the array list.
 */
void celix_arrayList_addSize(celix_array_list_t *list, size_t value);

/**
 * @brief Returns the index of the provided entry, if found.
 *
 * The equals callback function provided when the array list was created will be used
 * to determine if a array list entry equals the provided entry.
 *
 * The equals callback provided when the array list was created will be used to find the entry.
 * If there was no equals callback provided a direct memory compare will be done.
 *
 * @param list The array list.
 * @param entry The entry to find.
 * @return The index of the entry or -1 if the entry is not found.
 */
int celix_arrayList_indexOf(celix_array_list_t *list, celix_array_list_entry_t entry);

/**
 * @brief Removes an entry at the provided index.
 * If the provided index < 0 or out of bound, nothing will be removed.
 */
void celix_arrayList_removeAt(celix_array_list_t *list, int index);

/**
 * @brief Clear all entries in the array list.
 *
 * @note If a (simple) removed callback is configured, the callback will be called for every array list entry.
 */
void celix_arrayList_clear(celix_array_list_t *list);

/**
 * @brief Remove the first entry from array list which matches the provided value.
 *
 * The provided entry is expected to be cleared using memset, before it gets assigned a value.
 * If there is no equals provided, the entry will not be removed.
 *
 * The equals callback provided when the array list was created will be used to find the entry.
 * If there was no equals callback provided a direct memory compare will be done.
 */
void celix_arrayList_removeEntry(celix_array_list_t *list, celix_array_list_entry_t entry);

/**
 * @brief Remove the first pointer entry from array list which matches the provided value.
 *
 * The equals callback provided when the array list was created will be used to find the entry.
 * If there was no equals callback provided a direct memory compare will be done.
 */
void celix_arrayList_remove(celix_array_list_t *list, void *value);

/**
 * @brief Remove the first int entry from array list which matches the provided value.
 *
 * The equals callback provided when the array list was created will be used to find the entry.
 * If there was no equals callback provided a direct memory compare will be done.
 */
void celix_arrayList_removeInt(celix_array_list_t *list, int value);

/**
 * @brief Remove the first long entry from array list which matches the provided value.
 *
 * The equals callback provided when the array list was created will be used to find the entry.
 * If there was no equals callback provided a direct memory compare will be done.
 */
void celix_arrayList_removeLong(celix_array_list_t *list, long value);

/**
 * @brief Remove the first unsigned int entry from array list which matches the provided value.
 *
 * The equals callback provided when the array list was created will be used to find the entry.
 * If there was no equals callback provided a direct memory compare will be done.
 */
void celix_arrayList_removeUInt(celix_array_list_t *list, unsigned int value);

/**
 * @brief Remove the first unsigned long entry from array list which matches the provided value.
 *
 * The equals callback provided when the array list was created will be used to find the entry.
 * If there was no equals callback provided a direct memory compare will be done.
 */
void celix_arrayList_removeULong(celix_array_list_t *list, unsigned long value);

/**
 * @brief Remove the first float entry from array list which matches the provided value.
 *
 * The equals callback provided when the array list was created will be used to find the entry.
 * If there was no equals callback provided a direct memory compare will be done.
 */
void celix_arrayList_removeFloat(celix_array_list_t *list, float value);

/**
 * @brief Remove the first double entry from array list which matches the provided value.
 *
 * The equals callback provided when the array list was created will be used to find the entry.
 * If there was no equals callback provided a direct memory compare will be done.
 */
void celix_arrayList_removeDouble(celix_array_list_t *list, double value);

/**
 * @brief Remove the first bool entry from array list which matches the provided value.
 *
 * The equals callback provided when the array list was created will be used to find the entry.
 * If there was no equals callback provided a direct memory compare will be done.
 */
void celix_arrayList_removeBool(celix_array_list_t *list, bool value);

/**
 * @brief Remove the first size entry from array list which matches the provided value.
 *
 * The equals callback provided when the array list was created will be used to find the entry.
 * If there was no equals callback provided a direct memory compare will be done.
 */
void celix_arrayList_removeSize(celix_array_list_t *list, size_t value);

/**
 * @brief Sort the array list using the provided sort function.
 */
void celix_arrayList_sortEntries(celix_array_list_t *list, celix_array_list_sort_entries_fp sortFp);

/**
 * @warning Never use this function with array of doubles, since on some 32-bit platform (sizeof(double)==8 && sizeof(void*)==4)
 * @deprecated This function is deprecated, use celix_arrayList_sortEntries instead.
 */
void celix_arrayList_sort(celix_array_list_t *list, celix_arrayList_sort_fp sortFp);

#ifdef __cplusplus
}
#endif

#undef CELIX_OPTS_INIT

#endif /* CELIX_ARRAY_LIST_H_ */
