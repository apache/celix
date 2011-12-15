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
 * linkedlist.c
 *
 *  Created on: Jul 16, 2010
 *      Author: alexanderb
 */
#include "celixbool.h"
#include <stdio.h>
#include <stdlib.h>

#include "linkedlist.h"
#include "linked_list_private.h"

celix_status_t linkedList_create(apr_pool_t *pool, LINKED_LIST *list) {
	LINKED_LIST linked_list = (LINKED_LIST) apr_pcalloc(pool, sizeof(*linked_list));
	if (linked_list) {
        linked_list->header = (LINKED_LIST_ENTRY) apr_pcalloc(pool, sizeof(*(linked_list->header)));
        if (linked_list->header) {
            linked_list->header->element = NULL;
            linked_list->header->next = linked_list->header;
            linked_list->header->previous = linked_list->header;
            linked_list->size = 0;
            linked_list->modificationCount = 0;
            linked_list->memory_pool = pool;

            *list = linked_list;

            return CELIX_SUCCESS;
        }
	}

	return CELIX_ENOMEM;
}

celix_status_t linkedList_clone(LINKED_LIST list, apr_pool_t *pool, LINKED_LIST *clone) {
	celix_status_t status = CELIX_SUCCESS;

	status = linkedList_create(pool, clone);
	if (status == CELIX_SUCCESS) {
        for (struct linkedListEntry *e = list->header->next; e != list->header; e = e->next) {
            linkedList_addElement(*clone, e->element);
        }
	}

	return status;
}

void * linkedList_getFirst(LINKED_LIST list) {
	if (list->size == 0) {
		return NULL;
	}
	return list->header->next->element;
}

void * linkedList_getLast(LINKED_LIST list) {
	if (list->size == 0) {
		return NULL;
	}
	return list->header->previous->element;
}

void * linkedList_removeFirst(LINKED_LIST list) {
	return linkedList_removeEntry(list, list->header->next);
}

void * linkedList_removeLast(LINKED_LIST list) {
	return linkedList_removeEntry(list, list->header->previous);
}

void linkedList_addFirst(LINKED_LIST list, void * element) {
	linkedList_addBefore(list, element, list->header->next);
}

void linkedList_addLast(LINKED_LIST list, void * element) {
	linkedList_addBefore(list, element, list->header);
}

bool linkedList_contains(LINKED_LIST list, void * element) {
	return linkedList_indexOf(list, element) != 0;
}

int linkedList_size(LINKED_LIST list) {
	return list->size;
}

bool linkedList_isEmpty(LINKED_LIST list) {
	return linkedList_size(list) == 0;
}

bool linkedList_addElement(LINKED_LIST list, void * element) {
	linkedList_addBefore(list, element, list->header);
	return true;
}

bool linkedList_removeElement(LINKED_LIST list, void * element) {
	if (element == NULL) {
		LINKED_LIST_ENTRY entry;
		for (entry = list->header->next; entry != list->header; entry = entry->next) {
			if (entry->element == NULL) {
				linkedList_removeEntry(list, entry);
				return true;
			}
		}
	} else {
		LINKED_LIST_ENTRY entry;
		for (entry = list->header->next; entry != list->header; entry = entry->next) {
			if (element == entry->element) {
				linkedList_removeEntry(list, entry);
				return true;
			}
		}
	}
	return false;
}

void linkedList_clear(LINKED_LIST list) {
	LINKED_LIST_ENTRY entry = list->header->next;
	while (entry != list->header) {
		LINKED_LIST_ENTRY next = entry->next;
		entry->next = entry->previous = NULL;
		// free(entry->element);
		entry->element = NULL;
		free(entry);
		entry = next;
	}
	list->header->next = list->header->previous = list->header;
	list->size = 0;
	list->modificationCount++;
}

void * linkedList_get(LINKED_LIST list, int index) {
	return linkedList_entry(list, index)->element;
}
void * linkedList_set(LINKED_LIST list, int index, void * element) {
	LINKED_LIST_ENTRY entry = linkedList_entry(list, index);
	void * old = entry->element;
	entry->element = element;
	return old;
}

void linkedList_addIndex(LINKED_LIST list, int index, void * element) {
	linkedList_addBefore(list, element, (index == list->size ? list->header : linkedList_entry(list, index)));
}

void * linkedList_removeIndex(LINKED_LIST list, int index) {
	return linkedList_removeEntry(list, linkedList_entry(list, index));
}

LINKED_LIST_ENTRY linkedList_entry(LINKED_LIST list, int index) {
	LINKED_LIST_ENTRY entry;
	int i;
	if (index < 0 || index >= list->size) {
		return NULL;
	}

	entry = list->header;
	if (index < (list->size >> 1)) {
		for (i = 0; i <= index; i++) {
			entry = entry->next;
		}
	} else {
		for (i = list->size; i > index; i--) {
			entry = entry->previous;
		}
	}
	return entry;
}

int linkedList_indexOf(LINKED_LIST list, void * element) {
	int index = 0;
	if (element == NULL) {
		LINKED_LIST_ENTRY entry;
		for (entry = list->header->next; entry != list->header; entry = entry->next) {
			if (entry->element == NULL) {
				return index;
			}
			index++;
		}
	} else {
		LINKED_LIST_ENTRY entry;
		for (entry = list->header->next; entry != list->header; entry = entry->next) {
			if (element == entry->element) {
				return index;
			}
			index++;
		}
	}
	return -1;
}

LINKED_LIST_ENTRY linkedList_addBefore(LINKED_LIST list, void * element, LINKED_LIST_ENTRY entry) {
    LINKED_LIST_ENTRY new = NULL;
    apr_pool_t *sub_pool = NULL;

    if (apr_pool_create(&sub_pool, list->memory_pool) == APR_SUCCESS) {
        new = (LINKED_LIST_ENTRY) apr_palloc(sub_pool, sizeof(*new));
        new->memory_pool = sub_pool;
        new->element = element;
        new->next = entry;
        new->previous = entry->previous;

        new->previous->next = new;
        new->next->previous = new;

        list->size++;
        list->modificationCount++;
    }

	return new;
}

void * linkedList_removeEntry(LINKED_LIST list, LINKED_LIST_ENTRY entry) {
    void * result;
	if (entry == list->header) {
		return NULL;
	}

	result = entry->element;

	entry->previous->next = entry->next;
	entry->next->previous = entry->previous;

	entry->next = entry->previous = NULL;
	apr_pool_destroy(entry->memory_pool);

	list->size--;
	list->modificationCount++;

	return result;
}
