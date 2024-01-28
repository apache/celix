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

#include <stdbool.h>

#include "celix_cleanup.h"
#include "celix_errno.h"
#include "celix_utils_export.h"

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
    void* voidPtrVal;
    const char* strVal;
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
 * @brief Compare function for array list entries, which can be used to sort a array list.
 */
typedef int (*celix_array_list_compare_entries_fp)(celix_array_list_entry_t a, celix_array_list_entry_t b);

typedef celix_array_list_compare_entries_fp celix_array_list_sort_entries_fp __attribute__((deprecated("Use celix_arrayList_compare_entries_fp instead")));


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

    /**
     * Initial capacity of the array list. If 0, the default capacity will be used.
     */
    size_t initialCapacity CELIX_OPTS_INIT;

} celix_array_list_create_options_t;

#ifndef __cplusplus
/**
 * @brief C Macro to create a empty string_hash_map_create_options_t type.
 */
#define CELIX_EMPTY_ARRAY_LIST_CREATE_OPTIONS {     \
    .simpleRemovedCallback = NULL,                  \
    .removedCallbackData = NULL,                    \
    .removedCallback = NULL,                        \
    .equalsCallback = NULL,                         \
    .initialCapacity = 0                            \
}
#endif

/**
 * @brief Creates a new empty array list.
 *
 * If NULL is returned, an error message is logged to celix_err.
 *
 * @return A new empty array list or NULL if there is not enough memory.
 */
CELIX_UTILS_EXPORT
celix_array_list_t* celix_arrayList_create();

/**
 * @brief Creates a new empty array list, which uses the provided equals to check whether entries
 * are equal.
 * @deprecated This functions is deprecated, use celix_arrayList_createWithOptions instead.
 */
CELIX_UTILS_EXPORT
celix_array_list_t* celix_arrayList_createWithEquals(celix_arrayList_equals_fp equals);

/**
 * @brief Creates a new empty array listusing using the provided array list create options.
 *
 * If NULL is returned, an error message is logged to celix_err.
 *
 * @param opts The create options, only used during the creation of the array list.
 * @return A new empty array list or NULL if there is not enough memory.
 */
CELIX_UTILS_EXPORT
celix_array_list_t* celix_arrayList_createWithOptions(const celix_array_list_create_options_t* opts);

/**
 * @brief Clears and then deallocated the array list.
 *
 * @note If a (simple) removed callback is configured, the callback will be called for every array list entry.
 */
CELIX_UTILS_EXPORT
void celix_arrayList_destroy(celix_array_list_t *list);

CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(celix_array_list_t, celix_arrayList_destroy)

/**
 * @brief Returns the size of the array list.
 */
CELIX_UTILS_EXPORT
int celix_arrayList_size(const celix_array_list_t *list);

/**
 * @brief Create a shallow copy of the array list.
 *
 * The returned array list will be a shallow copy of the provided array list.
 * If the entries are pointers, the pointers will be copied, but the pointed to values will not be copied.
 * The equals callback provided when the provided array list was created will be copied, the removed callback
 * will not be copied.
 *
 * If the provided list is NULL, NULL is returned.
 * If the return value is NULL, an error message is logged to celix_err.
 *
 * @param list The array list.
 * @return A shallow copy of the array list or NULL if there is not enough memory.
 */
CELIX_UTILS_EXPORT
celix_array_list_t* celix_arrayList_copy(const celix_array_list_t *list);

/**
 * @brief Returns the value for the provided index.
 *
 * @param list The array list.
 * @param index The entry index to return.
 * @return Returns the pointer value for the index. Returns NULL if index is out of bound.
 */
CELIX_UTILS_EXPORT
void* celix_arrayList_get(const celix_array_list_t *list, int index);

/**
 * @brief Returns the value for the provided index.
 *
 * @param list The array list.
 * @param index The entry index to return.
 * @return Returns the int value for the index. Returns 0 if index is out of bound.
 */
CELIX_UTILS_EXPORT
int celix_arrayList_getInt(const celix_array_list_t *list, int index);

/**
 * @brief Returns the value for the provided index.
 *
 * @param list The array list.
 * @param index The entry index to return.
 * @return Returns the long value for the index. Returns 0 if index is out of bound.
 */
CELIX_UTILS_EXPORT
long int celix_arrayList_getLong(const celix_array_list_t *list, int index);

/**
 * @brief Returns the value for the provided index.
 *
 * @param list The array list.
 * @param index The entry index to return.
 * @return Returns the unsigned int value for the index. Returns 0 if index is out of bound.
 */
CELIX_UTILS_EXPORT
unsigned int celix_arrayList_getUInt(const celix_array_list_t *list, int index);

/**
 * @brief Returns the value for the provided index.
 *
 * @param list The array list.
 * @param index The entry index to return.
 * @return Returns the unsigned long value for the index. Returns 0 if index is out of bound.
 */
CELIX_UTILS_EXPORT
unsigned long int celix_arrayList_getULong(const celix_array_list_t *list, int index);

/**
 * @brief Returns the value for the provided index.
 *
 * @param list The array list.
 * @param index The entry index to return.
 * @return Returns the float value for the index. Returns 0 if index is out of bound.
 */
CELIX_UTILS_EXPORT
float celix_arrayList_getFloat(const celix_array_list_t *list, int index);

/**
 * @brief Returns the value for the provided index.
 *
 * @param list The array list.
 * @param index The entry index to return.
 * @return Returns the double value for the index. Returns 0 if index is out of bound.
 */
CELIX_UTILS_EXPORT
double celix_arrayList_getDouble(const celix_array_list_t *list, int index);

/**
 * @brief Returns the value for the provided index.
 *
 * @param list The array list.
 * @param index The entry index to return.
 * @return Returns the bool value for the index. Returns false if index is out of bound.
 */
CELIX_UTILS_EXPORT
bool celix_arrayList_getBool(const celix_array_list_t *list, int index);

/**
 * @brief Returns the value for the provided index.
 *
 * @param list The array list.
 * @param index The entry index to return.
 * @return Returns the size_t value for the index. Returns 0 if index is out of bound.
 */
CELIX_UTILS_EXPORT
size_t celix_arrayList_getSize(const celix_array_list_t *list, int index);

/**
 * @brief Returns the value for the provided index.
 *
 * @param list The array list.
 * @param index The entry index to return.
 * @return Returns the string (const char*) value for the index. Returns NULL if index is out of bound.
 */
CELIX_UTILS_EXPORT
const char* celix_arrayList_getString(const celix_array_list_t *list, int index);

/**
 * @brief Returns the entry for the provided index.
 * 
 * @param list The array list.
 * @param index The entry index to return.
 * @return Returns the entry for the index. Returns NULL if index is out of bound.
 */
celix_array_list_entry_t celix_arrayList_getEntry(const celix_array_list_t *list, int index);

/**
 * @brief add pointer entry to the back of the array list.
 *
 * @param list The array list.
 * @param value The pointer value to add to the array list.
 * @return CELIX_SUCCESS if the value is added, CELIX_ENOMEM if the array list is out of memory.
 */
CELIX_UTILS_EXPORT
celix_status_t celix_arrayList_add(celix_array_list_t *list, void* value);

/**
 * @brief add pointer entry to the back of the array list.
 *
 * @param list The array list.
 * @param value The int value to add to the array list.
 * @return CELIX_SUCCESS if the value is added, CELIX_ENOMEM if the array list is out of memory.
 */
CELIX_UTILS_EXPORT
celix_status_t celix_arrayList_addInt(celix_array_list_t *list, int value);

/**
 * @brief add pointer entry to the back of the array list.
 *
 * @param list The array list.
 * @param value The long value to add to the array list.
 * @return CELIX_SUCCESS if the value is added, CELIX_ENOMEM if the array list is out of memory.
 */
CELIX_UTILS_EXPORT
celix_status_t celix_arrayList_addLong(celix_array_list_t *list, long value);

/**
 * @brief add pointer entry to the back of the array list.
 *
 * @param list The array list.
 * @param value The unsigned int value to add to the array list.
 * @return CELIX_SUCCESS if the value is added, CELIX_ENOMEM if the array list is out of memory.
 */
CELIX_UTILS_EXPORT
celix_status_t celix_arrayList_addUInt(celix_array_list_t *list, unsigned int value);

/**
 * @brief add pointer entry to the back of the array list.
 *
 * @param list The array list.
 * @param value The unsigned long value to add to the array list.
 * @return CELIX_SUCCESS if the value is added, CELIX_ENOMEM if the array list is out of memory.
 */
CELIX_UTILS_EXPORT
celix_status_t celix_arrayList_addULong(celix_array_list_t *list, unsigned long value);

/**
 * @brief add pointer entry to the back of the array list.
 *
 * @param list The array list.
 * @param value The float value to add to the array list.
 * @return CELIX_SUCCESS if the value is added, CELIX_ENOMEM if the array list is out of memory.
 */
CELIX_UTILS_EXPORT
celix_status_t celix_arrayList_addFloat(celix_array_list_t *list, float value);

/**
 * @brief add pointer entry to the back of the array list.
 *
 * @param list The array list.
 * @param value The double value to add to the array list.
 * @return CELIX_SUCCESS if the value is added, CELIX_ENOMEM if the array list is out of memory.
 */
CELIX_UTILS_EXPORT
celix_status_t celix_arrayList_addDouble(celix_array_list_t *list, double value);

/**
 * @brief add pointer entry to the back of the array list.
 *
 * @param list The array list.
 * @param value The bool value to add to the array list.
 * @return CELIX_SUCCESS if the value is added, CELIX_ENOMEM if the array list is out of memory.
 */
CELIX_UTILS_EXPORT
celix_status_t celix_arrayList_addBool(celix_array_list_t *list, bool value);

/**
 * @brief add pointer entry to the back of the array list.
 *
 * @param list The array list.
 * @param value The size_t value to add to the array list.
 * @return CELIX_SUCCESS if the value is added, CELIX_ENOMEM if the array list is out of memory.
 */
CELIX_UTILS_EXPORT
celix_status_t celix_arrayList_addSize(celix_array_list_t *list, size_t value);

/**
 * @brief add string pointer entry to the back of the array list.
 * @note The string will *not* be copied.
 *
 * @param list The array list.
 * @param value The string value to add to the array list.
 * @return CELIX_SUCCESS if the value is added, CELIX_ENOMEM if the array list is out of memory.
 */
CELIX_UTILS_EXPORT celix_status_t celix_arrayList_addString(celix_array_list_t *list, const char* value);

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
CELIX_UTILS_EXPORT
int celix_arrayList_indexOf(celix_array_list_t *list, celix_array_list_entry_t entry);

/**
 * @brief Removes an entry at the provided index.
 * If the provided index < 0 or out of bound, nothing will be removed.
 */
CELIX_UTILS_EXPORT
void celix_arrayList_removeAt(celix_array_list_t *list, int index);

/**
 * @brief Clear all entries in the array list.
 *
 * @note If a (simple) removed callback is configured, the callback will be called for every array list entry.
 */
CELIX_UTILS_EXPORT
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
CELIX_UTILS_EXPORT
void celix_arrayList_removeEntry(celix_array_list_t *list, celix_array_list_entry_t entry);

/**
 * @brief Remove the first pointer entry from array list which matches the provided value.
 *
 * The equals callback provided when the array list was created will be used to find the entry.
 * If there was no equals callback provided a direct memory compare will be done.
 */
CELIX_UTILS_EXPORT
void celix_arrayList_remove(celix_array_list_t *list, void *value);

/**
 * @brief Remove the first int entry from array list which matches the provided value.
 *
 * The equals callback provided when the array list was created will be used to find the entry.
 * If there was no equals callback provided a direct memory compare will be done.
 */
CELIX_UTILS_EXPORT
void celix_arrayList_removeInt(celix_array_list_t *list, int value);

/**
 * @brief Remove the first long entry from array list which matches the provided value.
 *
 * The equals callback provided when the array list was created will be used to find the entry.
 * If there was no equals callback provided a direct memory compare will be done.
 */
CELIX_UTILS_EXPORT
void celix_arrayList_removeLong(celix_array_list_t *list, long value);

/**
 * @brief Remove the first unsigned int entry from array list which matches the provided value.
 *
 * The equals callback provided when the array list was created will be used to find the entry.
 * If there was no equals callback provided a direct memory compare will be done.
 */
CELIX_UTILS_EXPORT
void celix_arrayList_removeUInt(celix_array_list_t *list, unsigned int value);

/**
 * @brief Remove the first unsigned long entry from array list which matches the provided value.
 *
 * The equals callback provided when the array list was created will be used to find the entry.
 * If there was no equals callback provided a direct memory compare will be done.
 */
CELIX_UTILS_EXPORT
void celix_arrayList_removeULong(celix_array_list_t *list, unsigned long value);

/**
 * @brief Remove the first float entry from array list which matches the provided value.
 *
 * The equals callback provided when the array list was created will be used to find the entry.
 * If there was no equals callback provided a direct memory compare will be done.
 */
CELIX_UTILS_EXPORT
void celix_arrayList_removeFloat(celix_array_list_t *list, float value);

/**
 * @brief Remove the first double entry from array list which matches the provided value.
 *
 * The equals callback provided when the array list was created will be used to find the entry.
 * If there was no equals callback provided a direct memory compare will be done.
 */
CELIX_UTILS_EXPORT
void celix_arrayList_removeDouble(celix_array_list_t *list, double value);

/**
 * @brief Remove the first bool entry from array list which matches the provided value.
 *
 * The equals callback provided when the array list was created will be used to find the entry.
 * If there was no equals callback provided a direct memory compare will be done.
 */
CELIX_UTILS_EXPORT
void celix_arrayList_removeBool(celix_array_list_t *list, bool value);

/**
 * @brief Remove the first size entry from array list which matches the provided value.
 *
 * The equals callback provided when the array list was created will be used to find the entry.
 * If there was no equals callback provided a direct memory compare will be done.
 */
CELIX_UTILS_EXPORT
void celix_arrayList_removeSize(celix_array_list_t *list, size_t value);

/**
 * @brief Remove the first size entry from array list which matches the provided value.
 *
 * The equals callback provided when the array list was created will be used to find the entry.
 * If there was no equals callback provided a direct memory compare will be done.
 */
CELIX_UTILS_EXPORT
void celix_arrayList_removeString(celix_array_list_t *list, const char* value);

/**
 * @brief Sort the array list using the provided sort function.
 */
CELIX_UTILS_EXPORT
void celix_arrayList_sortEntries(celix_array_list_t *list, celix_array_list_compare_entries_fp compare);

/**
 * @warning Never use this function with array of doubles, since on some 32-bit platform (sizeof(double)==8 && sizeof(void*)==4)
 * @deprecated This function is deprecated, use celix_arrayList_sortEntries instead.
 */
CELIX_UTILS_DEPRECATED_EXPORT
void celix_arrayList_sort(celix_array_list_t *list, celix_arrayList_sort_fp sortFp);

#ifdef __cplusplus
}
#endif

#undef CELIX_OPTS_INIT

#endif /* CELIX_ARRAY_LIST_H_ */
