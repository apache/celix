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
 * linked_list_iterator.h
 *
 *  Created on: Jul 16, 2010
 *      Author: alexanderb
 */

#ifndef LINKED_LIST_ITERATOR_H_
#define LINKED_LIST_ITERATOR_H_

#include "celixbool.h"

#include "linkedlist.h"

typedef struct linkedListIterator * LINKED_LIST_ITERATOR;

LINKED_LIST_ITERATOR linkedListIterator_create(LINKED_LIST list, int index);
void linkedListIterator_destroy(LINKED_LIST_ITERATOR iterator);
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
