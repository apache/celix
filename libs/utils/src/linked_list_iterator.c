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
 * linked_list_iterator.c
 *
 *  \date       Jul 16, 2010
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <stdio.h>
#include <stdlib.h>

#include "linked_list_iterator.h"
#include "linked_list_private.h"

struct linkedListIterator {
    linked_list_entry_pt lastReturned;
    linked_list_entry_pt next;
    int nextIndex;
    linked_list_pt list;
    int expectedModificationCount;
};

linked_list_iterator_pt linkedListIterator_create(linked_list_pt list, unsigned int index) {
    linked_list_iterator_pt iterator;
    if (index > list->size) {
        return NULL;
    }
    iterator = (linked_list_iterator_pt) malloc(sizeof(*iterator));
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

void linkedListIterator_destroy(linked_list_iterator_pt iterator) {
    iterator->expectedModificationCount = 0;
    iterator->lastReturned = NULL;
    iterator->list = NULL;
    iterator->next = NULL;
    iterator->nextIndex = 0;
    free(iterator);
}

bool linkedListIterator_hasNext(linked_list_iterator_pt iterator) {
    return iterator->nextIndex != iterator->list->size;
}

void * linkedListIterator_next(linked_list_iterator_pt iterator) {
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

bool linkedListIterator_hasPrevious(linked_list_iterator_pt iterator) {
    return iterator->nextIndex != 0;
}

void * linkedListIterator_previous(linked_list_iterator_pt iterator) {
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

int linkedListIterator_nextIndex(linked_list_iterator_pt iterator) {
    return iterator->nextIndex;
}

int linkedListIterator_previousIndex(linked_list_iterator_pt iterator) {
    return iterator->nextIndex-1;
}

void linkedListIterator_remove(linked_list_iterator_pt iterator) {
    linked_list_entry_pt lastNext;
    if (iterator->list->modificationCount != iterator->expectedModificationCount) {
        return;
    }
    lastNext = iterator->lastReturned->next;
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

void linkedListIterator_set(linked_list_iterator_pt iterator, void * element) {
    if (iterator->lastReturned == iterator->list->header) {
        return;
    }
    if (iterator->list->modificationCount != iterator->expectedModificationCount) {
        return;
    }
    iterator->lastReturned->element = element;
}

void linkedListIterator_add(linked_list_iterator_pt iterator, void * element) {
    if (iterator->list->modificationCount != iterator->expectedModificationCount) {
        return;
    }
    iterator->lastReturned = iterator->list->header;
    linkedList_addBefore(iterator->list, element, iterator->next);
    iterator->nextIndex++;
    iterator->expectedModificationCount++;
}

