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
 * array_list.c
 *
 *  \date       Aug 4, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "array_list.h"
#include "array_list_private.h"

static celix_status_t arrayList_elementEquals(void *a, void *b, bool *equals);

celix_status_t arrayList_create(array_list_pt *list) {
	return arrayList_createWithEquals(arrayList_elementEquals, list);
}

 celix_status_t arrayList_createWithEquals(array_list_element_equals_pt equals, array_list_pt *list) {
	*list = (array_list_pt) malloc(sizeof(**list));

	(*list)->equals = equals;
	(*list)->size = 0;
	(*list)->capacity = 10;
	(*list)->modCount = 0;
	(*list)->elementData = (void **) malloc(sizeof(void*) * (*list)->capacity);

	return CELIX_SUCCESS;
}

void arrayList_destroy(array_list_pt list) {
	list->size = 0;
	free(list->elementData);
	list->elementData = NULL;
	free(list);
	list = NULL;
}

static celix_status_t arrayList_elementEquals(void *a, void *b, bool *equals) {
	*equals = (a == b);
	return CELIX_SUCCESS;
}

void arrayList_trimToSize(array_list_pt list) {
	unsigned int oldCapacity;
	list->modCount++;
	oldCapacity = list->capacity;
	if (list->size < oldCapacity) {
		void ** newList = (void **) realloc(list->elementData, sizeof(void *) * list->size);
		list->capacity = list->size;
		list->elementData = newList;
	}
}

void arrayList_ensureCapacity(array_list_pt list, int capacity) {
	void ** newList;
	int oldCapacity;
	list->modCount++;
	oldCapacity = list->capacity;
	if (capacity > oldCapacity) {
		int newCapacity = (oldCapacity * 3) / 2 + 1;
		if (newCapacity < capacity) {
			newCapacity = capacity;
		}
		newList = (void **) realloc(list->elementData, sizeof(void *) * newCapacity);
		list->capacity = newCapacity;
		list->elementData = newList;
	}
}

unsigned int arrayList_size(array_list_pt list) {
	return list->size;
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
			if (list->elementData[i] == NULL) {
				return i;
			}
		}
	} else {
		unsigned int i = 0;
		for (i = 0; i < list->size; i++) {
			bool equals = false;
			list->equals(element, list->elementData[i], &equals);
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
		for (i = list->size - 1; i >= 0; i--) {
			if (list->elementData[i] == NULL) {
				return i;
			}
		}
	} else {
		int i = 0;
		for (i = list->size - 1; i >= 0; i--) {
			bool equals = false;
			list->equals(element, list->elementData[i], &equals);
			if (equals) {
				return i;
			}
		}
	}
	return -1;
}

void * arrayList_get(array_list_pt list, unsigned int index) {
	if (index >= list->size) {
		return NULL;
	}

	return list->elementData[index];
}

void * arrayList_set(array_list_pt list, unsigned int index, void * element) {
	void * oldElement;
	if (index >= list->size) {
		return NULL;
	}

	oldElement = list->elementData[index];
	list->elementData[index] = element;
	return oldElement;
}

bool arrayList_add(array_list_pt list, void * element) {
	arrayList_ensureCapacity(list, list->size + 1);
	list->elementData[list->size++] = element;
	return true;
}

int arrayList_addIndex(array_list_pt list, unsigned int index, void * element) {
	unsigned int numMoved;
	if (index > list->size) {
		return -1;
	}
	arrayList_ensureCapacity(list, list->size+1);
	numMoved = list->size - index;
	memmove(list->elementData+(index+1), list->elementData+index, sizeof(void *) * numMoved);

	list->elementData[index] = element;
	list->size++;
	return 0;
}

void * arrayList_remove(array_list_pt list, unsigned int index) {
	void * oldElement;
	unsigned int numMoved;
	if (index >= list->size) {
		return NULL;
	}

	list->modCount++;
	oldElement = list->elementData[index];
	numMoved = list->size - index - 1;
	memmove(list->elementData+index, list->elementData+index+1, sizeof(void *) * numMoved);
	list->elementData[--list->size] = NULL;

	return oldElement;
}

void arrayList_fastRemove(array_list_pt list, unsigned int index) {
	unsigned int numMoved;
	list->modCount++;

	numMoved = list->size - index - 1;
	memmove(list->elementData+index, list->elementData+index+1, sizeof(void *) * numMoved);
	list->elementData[--list->size] = NULL;
}

bool arrayList_removeElement(array_list_pt list, void * element) {
	if (element == NULL) {
		unsigned int i = 0;
		for (i = 0; i < list->size; i++) {
			if (list->elementData[i] == NULL) {
				arrayList_fastRemove(list, i);
				return true;
			}
		}
	} else {
		unsigned int i = 0;
		for (i = 0; i < list->size; i++) {
			bool equals = false;
			list->equals(element, list->elementData[i], &equals);
			if (equals) {
				arrayList_fastRemove(list, i);
				return true;
			}
		}
	}
	return false;
}

void arrayList_clear(array_list_pt list) {
	unsigned int i;
	list->modCount++;

	for (i = 0; i < list->size; i++) {
		// free(list->elementData[i]);
		list->elementData[i] = NULL;
	}
	list->size = 0;
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
//	arrayList_ensureCapacity(new, list->size);
//	memcpy(new->elementData, list->elementData, list->size);
//	new->size = list->size;
	
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
	iterator = NULL;
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




