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
