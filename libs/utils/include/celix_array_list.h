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

#include "celix_utils_export.h"
#include "celix_cleanup.h"
#include "celix_errno.h"
#include "celix_version_type.h"

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

/**
 * @enum celix_array_list_element_type_t
 * @brief An enumeration of the types of elements that can be stored in a Celix array list.
 */
typedef enum celix_array_list_element_type {
    CELIX_ARRAY_LIST_ELEMENT_TYPE_UNDEFINED = 0, /**< Represents an undefined element type. */
    CELIX_ARRAY_LIST_ELEMENT_TYPE_POINTER = 1,   /**< Represents a pointer element type. */
    CELIX_ARRAY_LIST_ELEMENT_TYPE_STRING = 2, /**< Represents a string element type where the array list is the owner */
    CELIX_ARRAY_LIST_ELEMENT_TYPE_LONG = 3,   /**< Represents a long integer element type. */
    CELIX_ARRAY_LIST_ELEMENT_TYPE_DOUBLE = 4, /**< Represents a double element type. */
    CELIX_ARRAY_LIST_ELEMENT_TYPE_BOOL = 5,  /**< Represents a boolean element type. */
    CELIX_ARRAY_LIST_ELEMENT_TYPE_VERSION = 6, /**< Represents a celix_version_t* element type. */
} celix_array_list_element_type_t;

/**
 * @union celix_array_list_entry
 * @brief A union representing an entry in a Celix array list.
 *
 * This union can hold different types of values, including pointers, strings, integers, long integers,
 * unsigned integers, unsigned long integers, doubles, floats, booleans, and size_t values.
 */
typedef union celix_array_list_entry {
    void* voidPtrVal;      /**< A pointer value when the element type is CELIX_ARRAY_LIST_ELEMENT_TYPE_PTR or
                              CELIX_ARRAY_LIST_ELEMENT_TYPE_UNDEFINED. */
    const char* stringVal; /**< A string value when the element type is CELIX_ARRAY_LIST_ELEMENT_TYPE_STRING,
                              CELIX_ARRAY_LIST_ELEMENT_TYPE_STRING_REF or CELIX_ARRAY_LIST_ELEMENT_TYPE_UNDEFINED. */
    long int longVal;      /**< A long integer value when the element type is CELIX_ARRAY_LIST_ELEMENT_TYPE_LONG or
                              CELIX_ARRAY_LIST_ELEMENT_TYPE_UNDEFINED. */
    double doubleVal;       /**< A double value when the element type is CELIX_ARRAY_LIST_ELEMENT_TYPE_DOUBLE or
                               CELIX_ARRAY_LIST_ELEMENT_TYPE_UNDEFINED. */
    bool boolVal;           /**< A boolean value when the element type is CELIX_ARRAY_LIST_ELEMENT_TYPE_BOOL or
                               CELIX_ARRAY_LIST_ELEMENT_TYPE_UNDEFINED. */
    const celix_version_t* versionVal; /**< A celix_version_t* value when the element type is
                                   CELIX_ARRAY_LIST_ELEMENT_TYPE_VERSION or CELIX_ARRAY_LIST_ELEMENT_TYPE_UNDEFINED. */
} celix_array_list_entry_t;

/**
 * @brief A celix array list, which can store a list of undefined elements.
 */
typedef struct celix_array_list celix_array_list_t;

/**
 * @brief Equals function for array list entries, can be provided when creating a array list.
 */
typedef bool (*celix_arrayList_equals_fp)(celix_array_list_entry_t, celix_array_list_entry_t);

/**
 * @brief Compare function for array list entries, which can be used to sort a array list.
 */
typedef int (*celix_array_list_compare_entries_fp)(celix_array_list_entry_t a, celix_array_list_entry_t b);

/**
 * @brief Copy function for array list entries, which can be used to copy a array list entry.
 * @return CELIX_SUCCESS if the entry is copied, CELIX_ENOMEM if the entry could not be copied.
 */
typedef celix_status_t (*celix_array_list_copy_entry_fp)(celix_array_list_entry_t src, celix_array_list_entry_t* dst);

/**
 * @brief Creates a new empty array list with an undefined element type.
 * @deprecated Use celix_arrayList_createWithOptions or celix_arrayList_create<Type>Array instead.
 *
 * The remove, equals and compare callback will be NULL.
 *
 * @return A new empty array list or NULL if the array list could not be created. If NULL is returned an error message
 * is logged to celix_err.
 */
CELIX_UTILS_DEPRECATED_EXPORT
celix_array_list_t* celix_arrayList_create();

/**
 * @brief Creates a new empty array list with a pointer element type where the array list is not the owner of the
 * pointers.
 *
 * The remove, equals, compare and copy callback will be NULL.
 *
 * @return A new empty array list or NULL if the array list could not be created. If NULL is returned an error message
 * is logged to celix_err.
 */
CELIX_UTILS_EXPORT
celix_array_list_t* celix_arrayList_createPointerArray();

/**
 * @brief Creates a new empty array list with a string element type where the array list is the owner of the strings.
 *
 * The remove callback will be configured to free the string, the equals and compare callback will be configured for
 * string comparison and the copy callback will be a callback to uses celix_utils_strdup.
 *
 * @return A new empty array list or NULL if the array list could not be created. If NULL is returned an error message
 * is logged to celix_err.
 */
CELIX_UTILS_EXPORT
celix_array_list_t* celix_arrayList_createStringArray();

/**
 * @brief Creates a new empty array list with a long integer element type.
 *
 * The remove callback will be configured to NULL, the equals and compare callback will be configured for
 * integer comparison and the copy callback will be configured to NULL.
 *
 * @return A new empty array list or NULL if the array list could not be created. If NULL is returned an error message
 * is logged to celix_err.
 */
CELIX_UTILS_EXPORT
celix_array_list_t* celix_arrayList_createLongArray();

/**
 * @brief Creates a new empty array list with a double element type.
 *
 * The remove callback will be configured to NULL, the equals and compare callback will be configured for
 * double comparison and the copy callback will be configured to NULL.
 *
 * @return A new empty array list or NULL if the array list could not be created. If NULL is returned an error message
 * is logged to celix_err.
 */
CELIX_UTILS_EXPORT
celix_array_list_t* celix_arrayList_createDoubleArray();

/**
 * @brief Creates a new empty array list with a boolean element type.
 *
 * The remove callback will be configured to NULL, the equals and compare callback will be configured for
 * boolean comparison and the copy callback will be configured to NULL.
 *
 * @return A new empty array list or NULL if the array list could not be created. If NULL is returned an error message
 * is logged to celix_err.
 */
CELIX_UTILS_EXPORT
celix_array_list_t* celix_arrayList_createBoolArray();

/**
 * @brief Creates a new empty array list with a celix_version_t* element type.
 *
 * The remove callback will be configured to free a celix version, the equals and compare callback will be configured
 * for celix version comparison and the copy callback will a callback that uses celix_version_copy.
 *
 * @return A new empty array list or NULL if the array list could not be created. If NULL is returned an error message
 * is logged to celix_err.
 */
CELIX_UTILS_EXPORT
celix_array_list_t* celix_arrayList_createVersionArray();

/**
 * Additional create options when creating a array list.
 */
typedef struct celix_array_list_create_options {
    /**
     * The element type of the array list. Default is CELIX_ARRAY_LIST_ELEMENT_TYPE_UNDEFINED.
     */
    celix_array_list_element_type_t elementType CELIX_OPTS_INIT;

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
    celix_arrayList_equals_fp equalsCallback CELIX_OPTS_INIT;

    /**
     * Compare callback used when sorting the array list.
     */
    celix_array_list_compare_entries_fp compareCallback CELIX_OPTS_INIT;

    /**
     * The copy callback used to copy a array list entry.
     * If callback is NULL, a celix_arrayList_copy will result in a shallow copy.
     */
    celix_array_list_copy_entry_fp copyCallback CELIX_OPTS_INIT;

    /**
     * Initial capacity of the array list. If 0, the default capacity will be used.
     */
    size_t initialCapacity CELIX_OPTS_INIT;

} celix_array_list_create_options_t;

#ifndef __cplusplus
/**
 * @brief C Macro to create a empty celix_array_list_create_options_t type.
 */
#define CELIX_EMPTY_ARRAY_LIST_CREATE_OPTIONS                                                                          \
    {                                                                                                                  \
        .elementType = CELIX_ARRAY_LIST_ELEMENT_TYPE_UNDEFINED, .simpleRemovedCallback = NULL,                         \
        .removedCallbackData = NULL, .removedCallback = NULL, .equalsCallback = NULL,                                  \
        .compareCallback = NULL, .copyCallback = NULL,                                                                 \
    }
#endif

/**
 * @brief Creates a new empty array list using using the provided array list create options.
 *
 * The callbacks in the options will override the default callbacks based on the provided element type.
 * The default callbacks (e.g. the remove, equals, compare and copy callback) correspond to the
 * celix_arrayList_create<Type>Array function.
 * For example if the elementType is CELIX_ARRAY_LIST_ELEMENT_TYPE_STRING, the default callbacks will be set to
 * handle string values.
 *
 * @param opts The create options, only used during the creation of the array list.
 * @return A new empty array list or NULL if the array list could not be created. If NULL is returned an error message
 * is logged to celix_err.
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
 * @brief Return the element type of the array list.
 */
CELIX_UTILS_EXPORT
celix_array_list_element_type_t celix_arrayList_getElementType(const celix_array_list_t *list);

/**
 * @brief Returns the size of the array list.
 */
CELIX_UTILS_EXPORT
int celix_arrayList_size(const celix_array_list_t *list);

/**
 * @brief Returns the value for the provided index.
 *
 * Can be used for array list with element type CELIX_ARRAY_LIST_ELEMENT_TYPE_POINTER or
 * CELIX_ARRAY_LIST_ELEMENT_TYPE_UNDEFINED.
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
 * Can be used for array list with element type CELIX_ARRAY_LIST_ELEMENT_TYPE_STRING
 * or CELIX_ARRAY_LIST_ELEMENT_TYPE_UNDEFINED.
 *
 * @param list The array list.
 * @param index The entry index to return.
 * @return Returns the string value for the index. Returns NULL if index is out of bound.
 */
CELIX_UTILS_EXPORT
const char* celix_arrayList_getString(const celix_array_list_t *list, int index);

/**
 * @brief Returns the value for the provided index.
 *
 * Can be used for array list with element type CELIX_ARRAY_LIST_ELEMENT_TYPE_LONG or
 * CELIX_ARRAY_LIST_ELEMENT_TYPE_UNDEFINED.
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
 * Can be used for array list with element type CELIX_ARRAY_LIST_ELEMENT_TYPE_DOUBLE or
 * CELIX_ARRAY_LIST_ELEMENT_TYPE_UNDEFINED.
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
 * Can be used for array list with element type CELIX_ARRAY_LIST_ELEMENT_TYPE_BOOL or
 * CELIX_ARRAY_LIST_ELEMENT_TYPE_UNDEFINED.
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
 * Can be used for array list with element type CELIX_ARRAY_LIST_ELEMENT_TYPE_VERSION,
 * or CELIX_ARRAY_LIST_ELEMENT_TYPE_UNDEFINED.
 *
 * @param list The array list.
 * @param index The entry index to return.
 * @return Returns the version value for the index. Returns NULL if index is out of bound.
 */
CELIX_UTILS_EXPORT
const celix_version_t* celix_arrayList_getVersion(const celix_array_list_t *list, int index);

/**
 * @brief Returns the entry for the provided index.
 *
 * @param list The array list.
 * @param index The entry index to return.
 * @return Returns the entry for the index. Returns NULL if index is out of bound.
 */
CELIX_UTILS_EXPORT
celix_array_list_entry_t celix_arrayList_getEntry(const celix_array_list_t *list, int index);

/**
 * @brief add pointer entry to the back of the array list.
 *
 * Can be used for array list with element type CELIX_ARRAY_LIST_ELEMENT_TYPE_POINTER or
 * CELIX_ARRAY_LIST_ELEMENT_TYPE_UNDEFINED.
 *
 * @param list The array list.
 * @param value The pointer value to add to the array list. Cannot be NULL.
 * @return CELIX_SUCCESS if the value is added, CELIX_ENOMEM if the array list is out of memory.
 */
CELIX_UTILS_EXPORT
celix_status_t celix_arrayList_add(celix_array_list_t* list, void* value);

/**
 * @brief Add a string entry to the back of the array list.
 *
 * Can be used for array list with element type CELIX_ARRAY_LIST_ELEMENT_TYPE_STRING
 * or CELIX_ARRAY_LIST_ELEMENT_TYPE_UNDEFINED.
 *
 * If the array list element type is CELIX_ARRAY_LIST_ELEMENT_TYPE_STRING, the string will be copied, but
 * if the array list element type is CELIX_ARRAY_LIST_ELEMENT_TYPE_UNDEFINED,
 * the string will be added as reference (as-is).
 *
 * @param list The array list.
 * @param value The string value to add to the array list. Cannot be NULL.
 * @return CELIX_SUCCESS if the value is added, CELIX_ENOMEM if the array list is out of memory.
 */
CELIX_UTILS_EXPORT
celix_status_t celix_arrayList_addString(celix_array_list_t* list, const char* value);

/**
 * @brief Add a string entry to the back of a string array list.
 *
 * The string will not be copied and the array list will take ownership of the string.
 *
 * Can only be used for array list with element type CELIX_ARRAY_LIST_ELEMENT_TYPE_STRING.
 *
 * @param list The array list.
 * @param value The string value to add to the array list. Cannot be NULL.
 * @return CELIX_SUCCESS if the value is added, CELIX_ENOMEM if the array list is out of memory. If an error is returned
 * the provided value is not added to the array list, but the value will be freed using free.
 */
CELIX_UTILS_EXPORT
celix_status_t celix_arrayList_assignString(celix_array_list_t* list, char* value);

/**
 * @brief add pointer entry to the back of the array list.
 *
 * Can be used for array list with element type CELIX_ARRAY_LIST_ELEMENT_TYPE_LONG or
 * CELIX_ARRAY_LIST_ELEMENT_TYPE_UNDEFINED.
 *
 * @param list The array list.
 * @param value The long value to add to the array list.
 * @return CELIX_SUCCESS if the value is added, CELIX_ENOMEM if the array list is out of memory.
 */
CELIX_UTILS_EXPORT
celix_status_t celix_arrayList_addLong(celix_array_list_t* list, long value);

/**
 * @brief add pointer entry to the back of the array list.
 *
 * Can be used for array list with element type CELIX_ARRAY_LIST_ELEMENT_TYPE_DOUBLE or
 * CELIX_ARRAY_LIST_ELEMENT_TYPE_UNDEFINED.
 *
 * @param list The array list.
 * @param value The double value to add to the array list.
 * @return CELIX_SUCCESS if the value is added, CELIX_ENOMEM if the array list is out of memory.
 */
CELIX_UTILS_EXPORT
celix_status_t celix_arrayList_addDouble(celix_array_list_t* list, double value);

/**
 * @brief add pointer entry to the back of the array list.
 *
 * Can be used for array list with element type CELIX_ARRAY_LIST_ELEMENT_TYPE_BOOL or
 * CELIX_ARRAY_LIST_ELEMENT_TYPE_UNDEFINED.
 *
 * @param list The array list.
 * @param value The bool value to add to the array list.
 * @return CELIX_SUCCESS if the value is added, CELIX_ENOMEM if the array list is out of memory.
 */
CELIX_UTILS_EXPORT
celix_status_t celix_arrayList_addBool(celix_array_list_t* list, bool value);

/**
 * @brief Add a version entry to the back of the version array list.
 *
 * Can be used for array list with element type CELIX_ARRAY_LIST_ELEMENT_TYPE_VERSION,
 * or CELIX_ARRAY_LIST_ELEMENT_TYPE_UNDEFINED.
 *
 * If the array list element type is CELIX_ARRAY_LIST_ELEMENT_TYPE_VERSION, the version will be copied, but
 * if the array list element type is CELIX_ARRAY_LIST_ELEMENT_TYPE_UNDEFINED,
 * the string will be added as reference (as-is).
 *
 * @param list The array list.
 * @param value The version value to add to the array list. Cannot be NULL.
 * @return CELIX_SUCCESS if the value is added, CELIX_ENOMEM if the array list is out of memory.
 */
CELIX_UTILS_EXPORT
celix_status_t celix_arrayList_addVersion(celix_array_list_t* list, const celix_version_t* value);

/**
 * @brief Add a version entry to the back of a version array list.
 *
 * Can be used for array list with element type CELIX_ARRAY_LIST_ELEMENT_TYPE_VERSION.
 * The version will not be copied and the array list will take ownership of the version.
 *
 *
 * @param list The array list.
 * @param value The version value to add to the array list. Cannot be NULL.
 * @return CELIX_SUCCESS if the value is added, CELIX_ENOMEM if the array list is out of memory. If an error is
 * returned, the provided value is not added to the array list, but the value will be freed using celix_version_destroy.
 */
CELIX_UTILS_EXPORT
celix_status_t celix_arrayList_assignVersion(celix_array_list_t* list, celix_version_t* value);

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
 * Can be used for array list with element type CELIX_ARRAY_LIST_ELEMENT_TYPE_POINTER or
 * CELIX_ARRAY_LIST_ELEMENT_TYPE_UNDEFINED.
 *
 * The equals callback provided when the array list was created will be used to find the entry.
 * If there was no equals callback provided a direct memory compare will be done.
 */
CELIX_UTILS_EXPORT
void celix_arrayList_remove(celix_array_list_t* list, void* value);

/**
 * @brief Remove the first string entry from array list which matches the provided value.
 *
 * Can be used for array list with element type CELIX_ARRAY_LIST_ELEMENT_TYPE_STRING and
 * CELIX_ARRAY_LIST_ELEMENT_TYPE_UNDEFINED.
 *
 * The equals callback provided when the array list was created will be used to find the entry.
 * If there was no equals callback provided a direct memory compare will be done.
 */
CELIX_UTILS_EXPORT
void celix_arrayList_removeString(celix_array_list_t* list, const char* value);

/**
 * @brief Remove the first long entry from array list which matches the provided value.
 *
 * Can be used for array list with element type CELIX_ARRAY_LIST_ELEMENT_TYPE_LONG and
 * CELIX_ARRAY_LIST_ELEMENT_TYPE_UNDEFINED.
 *
 * The equals callback provided when the array list was created will be used to find the entry.
 * If there was no equals callback provided a direct memory compare will be done.
 */
CELIX_UTILS_EXPORT
void celix_arrayList_removeLong(celix_array_list_t* list, long value);

/**
 * @brief Remove the first double entry from array list which matches the provided value.
 *
 * Can be used for array list with element type CELIX_ARRAY_LIST_ELEMENT_TYPE_DOUBLE and
 * CELIX_ARRAY_LIST_ELEMENT_TYPE_UNDEFINED.
 *
 * The equals callback provided when the array list was created will be used to find the entry.
 * If there was no equals callback provided a direct memory compare will be done.
 */
CELIX_UTILS_EXPORT
void celix_arrayList_removeDouble(celix_array_list_t* list, double value);

/**
 * @brief Remove the first bool entry from array list which matches the provided value.
 *
 * Can be used for array list with element type CELIX_ARRAY_LIST_ELEMENT_TYPE_BOOL and
 * CELIX_ARRAY_LIST_ELEMENT_TYPE_UNDEFINED.
 *
 * The equals callback provided when the array list was created will be used to find the entry.
 * If there was no equals callback provided a direct memory compare will be done.
 */
CELIX_UTILS_EXPORT
void celix_arrayList_removeBool(celix_array_list_t* list, bool value);

/**
 * @brief Remove the first version entry from array list which matches the provided value.
 *
 * Can be used for array list with element type CELIX_ARRAY_LIST_ELEMENT_TYPE_VERSION and
 * CELIX_ARRAY_LIST_ELEMENT_TYPE_UNDEFINED.
 *
 * The equals callback provided when the array list was created will be used to find the entry.
 * If there was no equals callback provided a direct memory compare will be done.
 */
CELIX_UTILS_EXPORT
void celix_arrayList_removeVersion(celix_array_list_t* list, const celix_version_t* value);

/**
 * @brief Sort the array list using the provided sort function.
 */
CELIX_UTILS_EXPORT
void celix_arrayList_sortEntries(celix_array_list_t *list, celix_array_list_compare_entries_fp compare);

/**
 * @brief Sort the array list using the array list configured compare function.
 * Note that undefined the array list compare function can be NULL and in that case the array list will not be sorted.
 */
CELIX_UTILS_EXPORT
void celix_arrayList_sort(celix_array_list_t *list);

/**
 * @brief Check if the array list are equal.
 *
 * Equal is defined as:
 * - The array list have the same size
 * - The array list have the same element type
 * - The array list have the same equals callback
 * - The array list have the same values at the same index
 *
 * Note that the remove, compare and copy callbacks are ignored.
 *
 * If both array list are NULL, they are considered equal.
 *
 * @param listA The first array list.
 * @param listB The second array list.
 * @return true if the array list are equal, false otherwise.
 */
CELIX_UTILS_EXPORT
bool celix_arrayList_equals(const celix_array_list_t* listA, const celix_array_list_t* listB);

/**
 * @Brief Copy the array list to a new array list.
 *
 * The new array list will have the same element type and the same callbacks as the original array list.
 * For copying the array list entries the copy callback will be used, if the capy callback is NULL a shallow copy will
 * be done.
 *
 * If a NULL is returned and the original array list is not NULL, a error message is logged to celix_err.
 *
 * @param[in] list The array list to copy.
 * @return A new array list with the same element type and values as the original array list or NULL if the original
 * array list is NULL or out of memory.
 */
CELIX_UTILS_EXPORT
celix_array_list_t* celix_arrayList_copy(const celix_array_list_t* list);

#ifdef __cplusplus
}
#endif

#undef CELIX_OPTS_INIT

#endif /* CELIX_ARRAY_LIST_H_ */
