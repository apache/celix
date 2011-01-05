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
