/*
 * linkedlist.h
 *
 *  Created on: Jul 16, 2010
 *      Author: alexanderb
 */

#ifndef LINKEDLIST_H_
#define LINKEDLIST_H_

#include <stdbool.h>

typedef struct linkedListEntry * LINKED_LIST_ENTRY;
typedef struct linkedList * LINKED_LIST;

LINKED_LIST linkedList_create(void);
void * linkedList_getFirst(LINKED_LIST list);
void * linkedList_getLast(LINKED_LIST list);
void * linkedList_removeFirst(LINKED_LIST list);
void * linkedList_removeLast(LINKED_LIST list);
void linkedList_addFirst(LINKED_LIST list, void * element);
void linkedList_addLast(LINKED_LIST list, void * element);
bool linkedList_contains(LINKED_LIST list, void * element);
int linkedList_size(LINKED_LIST list);
bool linkedList_isEmpty(LINKED_LIST list);
bool linkedList_addElement(LINKED_LIST list, void * element);
bool linkedList_removeElement(LINKED_LIST list, void * element);
void linkedList_clear(LINKED_LIST list);
void * linkedList_get(LINKED_LIST list, int index);
void * linkedList_set(LINKED_LIST list, int index, void * element);
void linkedList_addIndex(LINKED_LIST list, int index, void * element);
void * linkedList_removeIndex(LINKED_LIST list, int index);
LINKED_LIST_ENTRY linkedList_entry(LINKED_LIST list, int index);
int linkedList_indexOf(LINKED_LIST list, void * element);
LINKED_LIST_ENTRY linkedList_addBefore(LINKED_LIST list, void * element, LINKED_LIST_ENTRY entry);
void * linkedList_removeEntry(LINKED_LIST list, LINKED_LIST_ENTRY entry);


#endif /* LINKEDLIST_H_ */
