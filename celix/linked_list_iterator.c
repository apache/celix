/*
 * linked_list_iterator.c
 *
 *  Created on: Jul 16, 2010
 *      Author: alexanderb
 */
#include <stdio.h>
#include <stdlib.h>

#include "linked_list_iterator.h"
#include "linked_list_private.h"

struct linkedListIterator {
	LINKED_LIST_ENTRY lastReturned;
	LINKED_LIST_ENTRY next;
	int nextIndex;
	LINKED_LIST list;
	int expectedModificationCount;
};

LINKED_LIST_ITERATOR linkedListIterator_create(LINKED_LIST list, int index) {
	if (index < 0 || index > list->size) {
		return NULL;
	}
	LINKED_LIST_ITERATOR iterator = (LINKED_LIST_ITERATOR) malloc(sizeof(*iterator));
	iterator->lastReturned = list->header;
	iterator->list = list;
	iterator->expectedModificationCount = list->modificationCount;
	if (index < (list->size >> 1)) {
		iterator->next = iterator->list->header->next;
		for (iterator->nextIndex = 0; iterator->nextIndex < index; iterator->nextIndex++) {
			iterator->next = iterator->next->next;
		}
	} else {
		iterator->next = list->header;
		for (iterator->nextIndex = list->size; iterator->nextIndex > index; iterator->nextIndex--) {
			iterator->next = iterator->next->previous;
		}
	}
	return iterator;
}

bool linkedListIterator_hasNext(LINKED_LIST_ITERATOR iterator) {
	return iterator->nextIndex != iterator->list->size;
}

void * linkedListIterator_next(LINKED_LIST_ITERATOR iterator) {
	if (iterator->list->modificationCount != iterator->expectedModificationCount) {
		return NULL;
	}
	if (iterator->nextIndex == iterator->list->size) {
		return NULL;
	}
	iterator->lastReturned = iterator->next;
	iterator->next = iterator->next->next;
	iterator->nextIndex++;
	return iterator->lastReturned->element;
}

bool linkedListIterator_hasPrevious(LINKED_LIST_ITERATOR iterator) {
	return iterator->nextIndex != 0;
}

void * linkedListIterator_previous(LINKED_LIST_ITERATOR iterator) {
	if (iterator->nextIndex == 0) {
		return NULL;
	}

	iterator->lastReturned = iterator->next = iterator->next->previous;
	iterator->nextIndex--;

	if (iterator->list->modificationCount != iterator->expectedModificationCount) {
		return NULL;
	}

	return iterator->lastReturned->element;
}

int linkedListIterator_nextIndex(LINKED_LIST_ITERATOR iterator) {
	return iterator->nextIndex;
}

int linkedListIterator_previousIndex(LINKED_LIST_ITERATOR iterator) {
	return iterator->nextIndex-1;
}

void linkedListIterator_remove(LINKED_LIST_ITERATOR iterator) {
	if (iterator->list->modificationCount != iterator->expectedModificationCount) {
		return;
	}
	LINKED_LIST_ENTRY lastNext = iterator->lastReturned->next;
	if (linkedList_removeEntry(iterator->list, iterator->lastReturned) == NULL) {
		return;
	}
	if (iterator->next == iterator->lastReturned) {
		iterator->next = lastNext;
	} else {
		iterator->nextIndex--;
	}
	iterator->lastReturned = iterator->list->header;
	iterator->expectedModificationCount++;
}

void linkedListIterator_set(LINKED_LIST_ITERATOR iterator, void * element) {
	if (iterator->lastReturned == iterator->list->header) {
		return;
	}
	if (iterator->list->modificationCount != iterator->expectedModificationCount) {
		return;
	}
	iterator->lastReturned->element = element;
}

void linkedListIterator_add(LINKED_LIST_ITERATOR iterator, void * element) {
	if (iterator->list->modificationCount != iterator->expectedModificationCount) {
		return;
	}
	iterator->lastReturned = iterator->list->header;
	linkedList_addBefore(iterator->list, element, iterator->next);
	iterator->nextIndex++;
	iterator->expectedModificationCount++;
}

