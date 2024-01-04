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
/**
 * array_list.c
 *
 *  \date       Aug 4, 2010
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "array_list.h"
#include "celix_array_list.h"

#include "array_list_private.h"
#include "celix_build_assert.h"
#include "celix_err.h"
#include "celix_stdio_cleanup.h"
#include "celix_stdlib_cleanup.h"

#define CELIX_ARRAY_LIST_DEFAULT_CAPACITY 10

static celix_status_t arrayList_elementEquals(const void *a, const void *b, bool *equals) {
    *equals = (a == b);
    return CELIX_SUCCESS;
}

static bool celix_arrayList_defaultEquals(celix_array_list_entry_t a, celix_array_list_entry_t b) {
    return memcmp(&a, &b, sizeof(a)) == 0;
}

static bool celix_arrayList_equalsForElement(celix_array_list_t *list, celix_array_list_entry_t a, celix_array_list_entry_t b) {
    bool equals = false;
    if (list != NULL) {
        if (list->equalsDeprecated != NULL) {
            list->equalsDeprecated(a.voidPtrVal, b.voidPtrVal, &equals);
        } else if (list->equals != NULL) {
            equals = list->equals(a, b);
        }
    }
    return equals;
}

static void celix_arrayList_callRemovedCallback(celix_array_list_t *list, int index) {
    celix_array_list_entry_t entry = list->elementData[index];
    if (list->simpleRemovedCallback != NULL) {
        list->simpleRemovedCallback(entry.voidPtrVal);
    } else if (list->removedCallback != NULL) {
        list->removedCallback(list->removedCallbackData, entry);
    }
}

celix_status_t arrayList_create(array_list_pt *list) {
    return arrayList_createWithEquals(arrayList_elementEquals, list);
}

celix_status_t arrayList_createWithEquals(array_list_element_equals_pt equals, array_list_pt *list) {
    *list = celix_arrayList_create();
    if (*list != NULL) {
        (*list)->equalsDeprecated = equals;

    }
    return CELIX_SUCCESS;
}

void arrayList_destroy(array_list_pt list) {
    celix_arrayList_destroy(list);
}

void arrayList_trimToSize(array_list_pt list) {
    list->modCount++;
    size_t oldCapacity = list->capacity;
    if (list->size < oldCapacity) {
        celix_array_list_entry_t * newList = realloc(list->elementData, sizeof(celix_array_list_entry_t) * list->size);
        list->capacity = list->size;
        list->elementData = newList;
    }
}

celix_status_t arrayList_ensureCapacity(array_list_pt list, int capacity) {
    celix_status_t status = CELIX_SUCCESS;
    celix_array_list_entry_t *newList;
    list->modCount++;
    size_t oldCapacity = list->capacity;
    if (capacity > oldCapacity) {
        size_t newCapacity = (oldCapacity * 3) / 2 + 1;
        if (newCapacity < capacity) {
            newCapacity = capacity;
        }
        newList = realloc(list->elementData, sizeof(celix_array_list_entry_t) * newCapacity);
        if (newList != NULL) {
            list->capacity = newCapacity;
            list->elementData = newList;
        }
        status = newList == NULL ? CELIX_ENOMEM : CELIX_SUCCESS;
    }
    return status;
}

unsigned int arrayList_size(array_list_pt list) {
    return (int)list->size;
}

bool arrayList_isEmpty(array_list_pt list) {
    return list->size == 0;
}

bool arrayList_contains(array_list_pt list, void * element) {
    int index = arrayList_indexOf(list, element);
    return index >= 0;
}

int arrayList_indexOf(array_list_pt list, void * element) {
    if (element == NULL) {
        unsigned int i = 0;
        for (i = 0; i < list->size; i++) {
            if (list->elementData[i].voidPtrVal == NULL) {
                return i;
            }
        }
    } else {
        unsigned int i = 0;
        for (i = 0; i < list->size; i++) {
            celix_array_list_entry_t entry;
            memset(&entry, 0, sizeof(entry));
            entry.voidPtrVal = element;
            bool equals = celix_arrayList_equalsForElement(list, entry, list->elementData[i]);
            if (equals) {
                return i;
            }
        }
    }
    return -1;
}

int arrayList_lastIndexOf(array_list_pt list, void * element) {
    if (element == NULL) {
        int i = 0;
        int size = (int)list->size;
        for (i = size - 1; i >= 0; i--) {
            if (list->elementData[i].voidPtrVal == NULL) {
                return (int)i;
            }
        }
    } else {
        int i = 0;
        int size = (int)list->size;
        for (i = size - 1; i >= 0; i--) {
            celix_array_list_entry_t entry;
            memset(&entry, 0, sizeof(entry));
            entry.voidPtrVal = element;
            bool equals = celix_arrayList_equalsForElement(list, entry, list->elementData[i]);
            if (equals) {
                return (int)i;
            }
        }
    }
    return -1;
}

void * arrayList_get(array_list_pt list, unsigned int index) {
    if (index >= list->size) {
        return NULL;
    }

    return list->elementData[index].voidPtrVal;
}

void * arrayList_set(array_list_pt list, unsigned int index, void * element) {
    void * oldElement;
    if (index >= list->size) {
        return NULL;
    }

    oldElement = list->elementData[index].voidPtrVal;
    memset(&list->elementData[index], 0, sizeof(celix_array_list_entry_t));
    list->elementData[index].voidPtrVal = element;
    return oldElement;
}

bool arrayList_add(array_list_pt list, void * element) {
    arrayList_ensureCapacity(list, (int)list->size + 1);
    memset(&list->elementData[list->size], 0, sizeof(celix_array_list_entry_t));
    list->elementData[list->size++].voidPtrVal = element;
    return true;
}

int arrayList_addIndex(array_list_pt list, unsigned int index, void * element) {
    size_t numMoved;
    if (index > list->size) {
        return -1;
    }
    arrayList_ensureCapacity(list, (int)list->size+1);
    numMoved = list->size - index;
    memmove(list->elementData+(index+1), list->elementData+index, sizeof(celix_array_list_entry_t) * numMoved);

    list->elementData[index].voidPtrVal = element;
    list->size++;
    return 0;
}

void * arrayList_remove(array_list_pt list, unsigned int index) {
    void * oldElement;
    size_t numMoved;
    if (index >= list->size) {
        return NULL;
    }

    list->modCount++;
    oldElement = list->elementData[index].voidPtrVal;
    numMoved = list->size - index - 1;
    memmove(list->elementData+index, list->elementData+index+1, sizeof(celix_array_list_entry_t) * numMoved);
    memset(&list->elementData[--list->size], 0, sizeof(celix_array_list_entry_t));

    return oldElement;
}

void arrayList_fastRemove(array_list_pt list, unsigned int index) {
    size_t numMoved;
    list->modCount++;

    numMoved = list->size - index - 1;
    memmove(list->elementData+index, list->elementData+index+1, sizeof(celix_array_list_entry_t) * numMoved);
    memset(&list->elementData[--list->size], 0, sizeof(celix_array_list_entry_t));
}

bool arrayList_removeElement(array_list_pt list, void * element) {
    if (element == NULL) {
        unsigned int i = 0;
        for (i = 0; i < list->size; i++) {
            if (list->elementData[i].voidPtrVal == NULL) {
                arrayList_fastRemove(list, i);
                return true;
            }
        }
    } else {
        unsigned int i = 0;
        for (i = 0; i < list->size; i++) {
            celix_array_list_entry_t entry;
            memset(&entry, 0, sizeof(entry));
            entry.voidPtrVal = element;
            bool equals = celix_arrayList_equalsForElement(list, entry, list->elementData[i]);
            if (equals) {
                arrayList_fastRemove(list, i);
                return true;
            }
        }
    }
    return false;
}

void arrayList_clear(array_list_pt list) {
    celix_arrayList_clear(list);
}

bool arrayList_addAll(array_list_pt list, array_list_pt toAdd) {
    unsigned int i;
    unsigned int size = arrayList_size(toAdd);
    arrayList_ensureCapacity(list, list->size + size);
//    memcpy(list->elementData+list->size, *toAdd->elementData, size);
//    list->size += size;
    for (i = 0; i < arrayList_size(toAdd); i++) {
        arrayList_add(list, arrayList_get(toAdd, i));
    }
    return size != 0;
}

array_list_pt arrayList_clone(array_list_pt list) {
    unsigned int i;
    array_list_pt new = NULL;
    arrayList_create(&new);
//    arrayList_ensureCapacity(new, list->size);
//    memcpy(new->elementData, list->elementData, list->size);
//    new->size = list->size;

    for (i = 0; i < arrayList_size(list); i++) {
        arrayList_add(new, arrayList_get(list, i));
    }
    new->modCount = 0;
    return new;
}

array_list_iterator_pt arrayListIterator_create(array_list_pt list) {
    array_list_iterator_pt iterator = (array_list_iterator_pt) malloc(sizeof(*iterator));

    iterator->lastReturned = -1;
    iterator->cursor = 0;
    iterator->list = list;
    iterator->expectedModificationCount = list->modCount;

    return iterator;
}

void arrayListIterator_destroy(array_list_iterator_pt iterator) {
    iterator->lastReturned = -1;
    iterator->cursor = 0;
    iterator->expectedModificationCount = 0;
    iterator->list = NULL;
    free(iterator);
}

bool arrayListIterator_hasNext(array_list_iterator_pt iterator) {
    return iterator->cursor != iterator->list->size;
}

void * arrayListIterator_next(array_list_iterator_pt iterator) {
    void * next;
    if (iterator->expectedModificationCount != iterator->list->modCount) {
        return NULL;
    }
    next = arrayList_get(iterator->list, iterator->cursor);
    iterator->lastReturned = iterator->cursor++;
    return next;
}

bool arrayListIterator_hasPrevious(array_list_iterator_pt iterator) {
    return iterator->cursor != 0;
}

void * arrayListIterator_previous(array_list_iterator_pt iterator) {
    int i;
    void * previous;
    if (iterator->expectedModificationCount != iterator->list->modCount) {
        return NULL;
    }
    i = iterator->cursor - 1;
    previous = arrayList_get(iterator->list, i);
    iterator->lastReturned = iterator->cursor = i;
    return previous;
}

void arrayListIterator_remove(array_list_iterator_pt iterator) {
    if (iterator->lastReturned == -1) {
        return;
    }
    if (iterator->expectedModificationCount != iterator->list->modCount) {
        return;
    }
    if (arrayList_remove(iterator->list, iterator->lastReturned) != NULL) {
        if (iterator->lastReturned < iterator->cursor) {
            iterator->cursor--;
        }
        iterator->lastReturned = -1;
        iterator->expectedModificationCount = iterator->list->modCount;
    }
}




/**********************************************************************************************************************
 **********************************************************************************************************************
 * Updated API
 **********************************************************************************************************************
 **********************************************************************************************************************/

celix_array_list_t* celix_arrayList_createWithOptions(const celix_array_list_create_options_t* opts) {
    celix_autofree celix_array_list_t *list = calloc(1, sizeof(*list));
    if (!list) {
        celix_err_push("Failed to create array list. Out of memory.");
        return NULL;
    }

    list->capacity = opts->initialCapacity == 0 ? CELIX_ARRAY_LIST_DEFAULT_CAPACITY : opts->initialCapacity;
    list->elementData = malloc(sizeof(celix_array_list_entry_t) * list->capacity);
    list->equals = opts->equalsCallback == NULL ? celix_arrayList_defaultEquals : opts->equalsCallback;
    list->simpleRemovedCallback = opts->simpleRemovedCallback;
    list->removedCallbackData = opts->removedCallbackData;
    list->removedCallback = opts->removedCallback;

    if (!list->elementData) {
        celix_err_push("Failed to create array list. Out of memory.");
        return NULL;
    }

    return celix_steal_ptr(list);
}

celix_array_list_t* celix_arrayList_create() {
    celix_array_list_create_options_t opts = CELIX_EMPTY_ARRAY_LIST_CREATE_OPTIONS;
    return celix_arrayList_createWithOptions(&opts);
}

celix_array_list_t* celix_arrayList_createWithEquals(celix_arrayList_equals_fp equals) {
    celix_array_list_create_options_t opts = CELIX_EMPTY_ARRAY_LIST_CREATE_OPTIONS;
    opts.equalsCallback = equals;
    return celix_arrayList_createWithOptions(&opts);
}

void celix_arrayList_destroy(celix_array_list_t *list) {
    if (list != NULL) {
        celix_arrayList_clear(list);
        free(list->elementData);
        free(list);
    }
}

int celix_arrayList_size(const celix_array_list_t* list) {
    return (int)list->size;
}

celix_array_list_t* celix_arrayList_copy(const celix_array_list_t *list) {
    if (!list) {
        return NULL; //silently ignore
    }
    celix_array_list_create_options_t opts = CELIX_EMPTY_ARRAY_LIST_CREATE_OPTIONS;
    opts.equalsCallback = list->equals;
    opts.initialCapacity = list->size;
    celix_array_list_t* copy = celix_arrayList_createWithOptions(&opts);
    if (!copy) {
        celix_err_push("Failed to copy array list. Out of memory.");
        return NULL;
    }
    copy->size = list->size;
    memcpy(copy->elementData, list->elementData, sizeof(celix_array_list_entry_t) * list->size);
    return copy;
}


static celix_array_list_entry_t arrayList_getEntry(const celix_array_list_t *list, int index) {
    celix_array_list_entry_t entry;
    memset(&entry, 0, sizeof(entry));
    if (index < list->size) {
        entry = list->elementData[index];
    }
    return entry;
}

void* celix_arrayList_get(const celix_array_list_t *list, int index) {
    return arrayList_getEntry(list, index).voidPtrVal;
}

int celix_arrayList_getInt(const celix_array_list_t *list, int index) { return arrayList_getEntry(list, index).intVal; }
long int celix_arrayList_getLong(const celix_array_list_t *list, int index) { return arrayList_getEntry(list, index).longVal; }
unsigned int celix_arrayList_getUInt(const celix_array_list_t *list, int index) { return arrayList_getEntry(list, index).uintVal; }
unsigned long int celix_arrayList_getULong(const celix_array_list_t *list, int index) { return arrayList_getEntry(list, index).ulongVal; }
float celix_arrayList_getFloat(const celix_array_list_t *list, int index) { return arrayList_getEntry(list, index).floatVal; }
double celix_arrayList_getDouble(const celix_array_list_t *list, int index) { return arrayList_getEntry(list, index).doubleVal; }
bool celix_arrayList_getBool(const celix_array_list_t *list, int index) { return arrayList_getEntry(list, index).boolVal; }
size_t celix_arrayList_getSize(const celix_array_list_t *list, int index) { return arrayList_getEntry(list, index).sizeVal; }
celix_array_list_entry_t celix_arrayList_getEntry(const celix_array_list_t *list, int index) { return arrayList_getEntry(list, index); }

static celix_status_t celix_arrayList_addEntry(celix_array_list_t *list, celix_array_list_entry_t entry) {
    celix_status_t status = arrayList_ensureCapacity(list, (int)list->size + 1);
    if (status == CELIX_SUCCESS) {
        list->elementData[list->size++] = entry;
    }
    return status;
}

celix_status_t celix_arrayList_add(celix_array_list_t *list, void * element) {
    celix_array_list_entry_t entry;
    memset(&entry, 0, sizeof(entry));
    entry.voidPtrVal = element;
    return celix_arrayList_addEntry(list, entry);
}

celix_status_t celix_arrayList_addInt(celix_array_list_t *list, int val) {
    celix_array_list_entry_t entry;
    memset(&entry, 0, sizeof(entry));
    entry.intVal = val;
    return celix_arrayList_addEntry(list, entry);
}

celix_status_t celix_arrayList_addLong(celix_array_list_t *list, long val) {
    celix_array_list_entry_t entry;
    memset(&entry, 0, sizeof(entry));
    entry.longVal = val;
    return celix_arrayList_addEntry(list, entry);
}

celix_status_t celix_arrayList_addUInt(celix_array_list_t *list, unsigned int val) {
    celix_array_list_entry_t entry;
    memset(&entry, 0, sizeof(entry));
    entry.uintVal = val;
    return celix_arrayList_addEntry(list, entry);
}

celix_status_t celix_arrayList_addULong(celix_array_list_t *list, unsigned long val) {
    celix_array_list_entry_t entry;
    memset(&entry, 0, sizeof(entry));
    entry.ulongVal = val;
    return celix_arrayList_addEntry(list, entry);
}

celix_status_t celix_arrayList_addDouble(celix_array_list_t *list, double val) {
    celix_array_list_entry_t entry;
    memset(&entry, 0, sizeof(entry));
    entry.doubleVal = val;
    return celix_arrayList_addEntry(list, entry);
}

celix_status_t celix_arrayList_addFloat(celix_array_list_t *list, float val) {
    celix_array_list_entry_t entry;
    memset(&entry, 0, sizeof(entry));
    entry.floatVal = val;
    return celix_arrayList_addEntry(list, entry);
}

celix_status_t celix_arrayList_addBool(celix_array_list_t *list, bool val) {
    celix_array_list_entry_t entry;
    memset(&entry, 0, sizeof(entry));
    entry.boolVal = val;
    return celix_arrayList_addEntry(list, entry);
}

celix_status_t celix_arrayList_addSize(celix_array_list_t *list, size_t val) {
    celix_array_list_entry_t entry;
    memset(&entry, 0, sizeof(entry));
    entry.sizeVal = val;
    return celix_arrayList_addEntry(list, entry);
}

int celix_arrayList_indexOf(celix_array_list_t *list, celix_array_list_entry_t entry) {
    size_t size = celix_arrayList_size(list);
    int i;
    int index = -1;
    for (i = 0 ; i < size ; ++i) {
        bool eq = celix_arrayList_equalsForElement(list, entry, list->elementData[i]);
        if (eq) {
            index = i;
            break;
        }
    }
    return index;
}
void celix_arrayList_removeAt(celix_array_list_t *list, int index) {
    if (index >= 0 && index < list->size) {
        celix_arrayList_callRemovedCallback(list, index);
        list->modCount++;
        size_t numMoved = list->size - index - 1;
        memmove(list->elementData+index, list->elementData+index+1, sizeof(celix_array_list_entry_t) * numMoved);
        memset(&list->elementData[--list->size], 0, sizeof(celix_array_list_entry_t));
    }
}

void celix_arrayList_removeEntry(celix_array_list_t *list, celix_array_list_entry_t entry) {
    int index = celix_arrayList_indexOf(list, entry);
    celix_arrayList_removeAt(list, index);
}


void celix_arrayList_remove(celix_array_list_t *list, void *ptr) {
    celix_array_list_entry_t entry;
    memset(&entry, 0, sizeof(entry));
    entry.voidPtrVal = ptr;
    celix_arrayList_removeEntry(list, entry);
}

void celix_arrayList_removeInt(celix_array_list_t *list, int val) {
    celix_array_list_entry_t entry;
    memset(&entry, 0, sizeof(entry));
    entry.intVal = val;
    celix_arrayList_removeEntry(list, entry);
}

void celix_arrayList_removeLong(celix_array_list_t *list, long val) {
    celix_array_list_entry_t entry;
    memset(&entry, 0, sizeof(entry));
    entry.longVal = val;
    celix_arrayList_removeEntry(list, entry);
}

void celix_arrayList_removeUInt(celix_array_list_t *list, unsigned int val) {
    celix_array_list_entry_t entry;
    memset(&entry, 0, sizeof(entry));
    entry.uintVal = val;
    celix_arrayList_removeEntry(list, entry);
}

void celix_arrayList_removeULong(celix_array_list_t *list, unsigned long val) {
    celix_array_list_entry_t entry;
    memset(&entry, 0, sizeof(entry));
    entry.ulongVal = val;
    celix_arrayList_removeEntry(list, entry);
}

void celix_arrayList_removeFloat(celix_array_list_t *list, float val) {
    celix_array_list_entry_t entry;
    memset(&entry, 0, sizeof(entry));
    entry.floatVal = val;
    celix_arrayList_removeEntry(list, entry);
}

void celix_arrayList_removeDouble(celix_array_list_t *list, double val) {
    celix_array_list_entry_t entry;
    memset(&entry, 0, sizeof(entry));
    entry.doubleVal = val;
    celix_arrayList_removeEntry(list, entry);
}

void celix_arrayList_removeBool(celix_array_list_t *list, bool val) {
    celix_array_list_entry_t entry;
    memset(&entry, 0, sizeof(entry));
    entry.boolVal = val;
    celix_arrayList_removeEntry(list, entry);
}

void celix_arrayList_removeSize(celix_array_list_t *list, size_t val) {
    celix_array_list_entry_t entry;
    memset(&entry, 0, sizeof(entry));
    entry.sizeVal = val;
    celix_arrayList_removeEntry(list, entry);
}

void celix_arrayList_clear(celix_array_list_t *list) {
    list->modCount++;
    for (int i = 0; i < list->size; ++i) {
        celix_arrayList_callRemovedCallback(list, i);
        memset(&list->elementData[i], 0, sizeof(celix_array_list_entry_t));
    }
    list->size = 0;
}

#if defined(__APPLE__)
static int celix_arrayList_compareEntries(void* arg, const void* voidA, const void* voidB) {
#else
static int celix_arrayList_compareEntries(const void* voidA, const void* voidB, void* arg) {
#endif
    celix_array_list_compare_entries_fp compare = arg;
    const celix_array_list_entry_t* a = voidA;
    const celix_array_list_entry_t* b = voidB;
    return compare(*a, *b);
}

void celix_arrayList_sortEntries(celix_array_list_t *list, celix_array_list_compare_entries_fp compare) {
#if defined(__APPLE__)
    qsort_r(list->elementData, list->size, sizeof(celix_array_list_entry_t), compare, celix_arrayList_compareEntries);
#else
    qsort_r(list->elementData, list->size, sizeof(celix_array_list_entry_t), celix_arrayList_compareEntries, compare);
#endif
}

#if defined(__APPLE__)
static int celix_arrayList_compare(void *arg, const void * a, const void *b) {
#else
static int celix_arrayList_compare(const void * a, const void *b, void *arg) {
#endif
    const celix_array_list_entry_t *aEntry = a;
    const celix_array_list_entry_t *bEntry = b;

    celix_arrayList_sort_fp sort = arg;

    return sort(aEntry->voidPtrVal, bEntry->voidPtrVal);
}

void celix_arrayList_sort(celix_array_list_t *list, celix_arrayList_sort_fp sortFp) {
#if defined(__APPLE__)
    qsort_r(list->elementData, list->size, sizeof(celix_array_list_entry_t), sortFp, celix_arrayList_compare);
#else
    qsort_r(list->elementData, list->size, sizeof(celix_array_list_entry_t), celix_arrayList_compare, sortFp);
#endif
}
