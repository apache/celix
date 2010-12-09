/*
 * linkedlist.c
 *
 *  Created on: Jul 16, 2010
 *      Author: alexanderb
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "linkedlist.h"
#include "linked_list_private.h"

LINKED_LIST linkedList_create(void) {
	LINKED_LIST list = (LINKED_LIST) malloc(sizeof(*list));
	list->header = (LINKED_LIST_ENTRY) malloc(sizeof(*(list->header)));
	list->header->element = NULL;
	list->header->next = list->header;
	list->header->previous = list->header;
	list->size = 0;
	list->modificationCount = 0;
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
		free(entry->element);
		entry->element = NULL;
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
	if (index < 0 || index >= list->size) {
		return NULL;
	}

	LINKED_LIST_ENTRY entry = list->header;
	int i;
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

// int linkedList_lastIndexOf(LINKED_LIST list, void * element);

LINKED_LIST_ENTRY linkedList_addBefore(LINKED_LIST list, void * element, LINKED_LIST_ENTRY entry) {
	LINKED_LIST_ENTRY new = (LINKED_LIST_ENTRY) malloc(sizeof(*new));
	new->element = element;
	new->next = entry;
	new->previous = entry->previous;

	new->previous->next = new;
	new->next->previous = new;

	list->size++;
	list->modificationCount++;

	return new;
}

void * linkedList_removeEntry(LINKED_LIST list, LINKED_LIST_ENTRY entry) {
	if (entry == list->header) {
		return NULL;
	}

	void * result = entry->element;

	entry->previous->next = entry->next;
	entry->next->previous = entry->previous;

	entry->next = entry->previous = NULL;
	free(entry);

	list->size--;
	list->modificationCount++;

	return result;
}
