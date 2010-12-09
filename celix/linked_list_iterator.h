/*
 * linked_list_iterator.h
 *
 *  Created on: Jul 16, 2010
 *      Author: alexanderb
 */

#ifndef LINKED_LIST_ITERATOR_H_
#define LINKED_LIST_ITERATOR_H_

#include <stdbool.h>

#include "linkedlist.h"

typedef struct linkedListIterator * LINKED_LIST_ITERATOR;

LINKED_LIST_ITERATOR linkedListIterator_create(LINKED_LIST list, int index);
bool linkedListIterator_hasNext(LINKED_LIST_ITERATOR iterator);
void * linkedListIterator_next(LINKED_LIST_ITERATOR iterator);
bool linkedListIterator_hasPrevious(LINKED_LIST_ITERATOR iterator);
void * linkedListIterator_previous(LINKED_LIST_ITERATOR iterator);
int linkedListIterator_nextIndex(LINKED_LIST_ITERATOR iterator);
int linkedListIterator_previousIndex(LINKED_LIST_ITERATOR iterator);
void linkedListIterator_remove(LINKED_LIST_ITERATOR iterator);
void linkedListIterator_set(LINKED_LIST_ITERATOR iterator, void * element);
void linkedListIterator_add(LINKED_LIST_ITERATOR iterator, void * element);


#endif /* LINKED_LIST_ITERATOR_H_ */
