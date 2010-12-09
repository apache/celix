/*
 * array_list.h
 *
 *  Created on: Aug 4, 2010
 *      Author: alexanderb
 */

#ifndef ARRAY_LIST_H_
#define ARRAY_LIST_H_

#include <stdbool.h>

typedef struct arrayList * ARRAY_LIST;

typedef struct arrayListIterator * ARRAY_LIST_ITERATOR;

ARRAY_LIST arrayList_create(void);
void arrayList_trimToSize(ARRAY_LIST list);
void arrayList_ensureCapacity(ARRAY_LIST list, int capacity);
unsigned int arrayList_size(ARRAY_LIST list);
bool arrayList_isEmpty(ARRAY_LIST list);
bool arrayList_contains(ARRAY_LIST list, void * element);
int arrayList_indexOf(ARRAY_LIST list, void * element);
int arrayList_lastIndexOf(ARRAY_LIST list, void * element);
void * arrayList_get(ARRAY_LIST list, unsigned int index);
void * arrayList_set(ARRAY_LIST list, unsigned int index, void * element);
bool arrayList_add(ARRAY_LIST list, void * element);
int arrayList_addIndex(ARRAY_LIST list, unsigned int index, void * element);
void * arrayList_remove(ARRAY_LIST list, unsigned int index);
bool arrayList_removeElement(ARRAY_LIST list, void * element);
void arrayList_clear(ARRAY_LIST list);
ARRAY_LIST arrayList_clone(ARRAY_LIST list);

ARRAY_LIST_ITERATOR arrayListIterator_create(ARRAY_LIST list);
bool arrayListIterator_hasNext(ARRAY_LIST_ITERATOR iterator);
void * arrayListIterator_next(ARRAY_LIST_ITERATOR iterator);
bool arrayListIterator_hasPrevious(ARRAY_LIST_ITERATOR iterator);
void * arrayListIterator_previous(ARRAY_LIST_ITERATOR iterator);
void arrayListIterator_remove(ARRAY_LIST_ITERATOR iterator);

#endif /* ARRAY_LIST_H_ */
