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
 *  \date       Jul 16, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include "celixbool.h"
#include <stdio.h>
#include <stdlib.h>

#include "linked_list.h"
#include "linked_list_private.h"

celix_status_t linkedList_create(linked_list_pt *list) {
	linked_list_pt linked_list = malloc(sizeof(*linked_list));
	if (linked_list) {
        linked_list->header = (linked_list_entry_pt) malloc(sizeof(*(linked_list->header)));
        if (linked_list->header) {
            linked_list->header->element = NULL;
            linked_list->header->next = linked_list->header;
            linked_list->header->previous = linked_list->header;
            linked_list->size = 0;
            linked_list->modificationCount = 0;

            *list = linked_list;

            return CELIX_SUCCESS;
        }
	}

	return CELIX_ENOMEM;
}

celix_status_t linkedList_clone(linked_list_pt list, linked_list_pt *clone) {
	celix_status_t status = CELIX_SUCCESS;

	status = linkedList_create(clone);
	if (status == CELIX_SUCCESS) {
		struct linked_list_entry *e;
        for (e = list->header->next; e != list->header; e = e->next) {
            linkedList_addElement(*clone, e->element);
        }
	}

	return status;
}

void * linkedList_getFirst(linked_list_pt list) {
	if (list->size == 0) {
		return NULL;
	}
	return list->header->next->element;
}

void * linkedList_getLast(linked_list_pt list) {
	if (list->size == 0) {
		return NULL;
	}
	return list->header->previous->element;
}

void * linkedList_removeFirst(linked_list_pt list) {
	return linkedList_removeEntry(list, list->header->next);
}

void * linkedList_removeLast(linked_list_pt list) {
	return linkedList_removeEntry(list, list->header->previous);
}

void linkedList_addFirst(linked_list_pt list, void * element) {
	linkedList_addBefore(list, element, list->header->next);
}

void linkedList_addLast(linked_list_pt list, void * element) {
	linkedList_addBefore(list, element, list->header);
}

bool linkedList_contains(linked_list_pt list, void * element) {
	return linkedList_indexOf(list, element) != 0;
}

int linkedList_size(linked_list_pt list) {
	return list->size;
}

bool linkedList_isEmpty(linked_list_pt list) {
	return linkedList_size(list) == 0;
}

bool linkedList_addElement(linked_list_pt list, void * element) {
	linkedList_addBefore(list, element, list->header);
	return true;
}

bool linkedList_removeElement(linked_list_pt list, void * element) {
	if (element == NULL) {
		linked_list_entry_pt entry;
		for (entry = list->header->next; entry != list->header; entry = entry->next) {
			if (entry->element == NULL) {
				linkedList_removeEntry(list, entry);
				return true;
			}
		}
	} else {
		linked_list_entry_pt entry;
		for (entry = list->header->next; entry != list->header; entry = entry->next) {
			if (element == entry->element) {
				linkedList_removeEntry(list, entry);
				return true;
			}
		}
	}
	return false;
}

void linkedList_clear(linked_list_pt list) {
	linked_list_entry_pt entry = list->header->next;
	while (entry != list->header) {
		linked_list_entry_pt next = entry->next;
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

void * linkedList_get(linked_list_pt list, int index) {
	return linkedList_entry(list, index)->element;
}
void * linkedList_set(linked_list_pt list, int index, void * element) {
	linked_list_entry_pt entry = linkedList_entry(list, index);
	void * old = entry->element;
	entry->element = element;
	return old;
}

void linkedList_addIndex(linked_list_pt list, int index, void * element) {
	linkedList_addBefore(list, element, (index == list->size ? list->header : linkedList_entry(list, index)));
}

void * linkedList_removeIndex(linked_list_pt list, int index) {
	return linkedList_removeEntry(list, linkedList_entry(list, index));
}

linked_list_entry_pt linkedList_entry(linked_list_pt list, int index) {
	linked_list_entry_pt entry;
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

int linkedList_indexOf(linked_list_pt list, void * element) {
	int index = 0;
	if (element == NULL) {
		linked_list_entry_pt entry;
		for (entry = list->header->next; entry != list->header; entry = entry->next) {
			if (entry->element == NULL) {
				return index;
			}
			index++;
		}
	} else {
		linked_list_entry_pt entry;
		for (entry = list->header->next; entry != list->header; entry = entry->next) {
			if (element == entry->element) {
				return index;
			}
			index++;
		}
	}
	return -1;
}

linked_list_entry_pt linkedList_addBefore(linked_list_pt list, void * element, linked_list_entry_pt entry) {
    linked_list_entry_pt new = NULL;

    new = malloc(sizeof(*new));
    if (new != NULL) {

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

void * linkedList_removeEntry(linked_list_pt list, linked_list_entry_pt entry) {
    void * result;
	if (entry == list->header) {
		return NULL;
	}

	result = entry->element;

	entry->previous->next = entry->next;
	entry->next->previous = entry->previous;

	entry->next = entry->previous = NULL;
	free(entry);

	list->size--;
	list->modificationCount++;

	return result;
}
