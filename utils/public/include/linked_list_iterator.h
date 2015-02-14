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
 *  \date       Jul 16, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef LINKED_LIST_ITERATOR_H_
#define LINKED_LIST_ITERATOR_H_

#include "celixbool.h"

#include "linked_list.h"
#include "exports.h"

typedef struct linkedListIterator * linked_list_iterator_pt;

UTILS_EXPORT linked_list_iterator_pt linkedListIterator_create(linked_list_pt list, unsigned int index);
UTILS_EXPORT void linkedListIterator_destroy(linked_list_iterator_pt iterator);
UTILS_EXPORT bool linkedListIterator_hasNext(linked_list_iterator_pt iterator);
UTILS_EXPORT void * linkedListIterator_next(linked_list_iterator_pt iterator);
UTILS_EXPORT bool linkedListIterator_hasPrevious(linked_list_iterator_pt iterator);
UTILS_EXPORT void * linkedListIterator_previous(linked_list_iterator_pt iterator);
UTILS_EXPORT int linkedListIterator_nextIndex(linked_list_iterator_pt iterator);
UTILS_EXPORT int linkedListIterator_previousIndex(linked_list_iterator_pt iterator);
UTILS_EXPORT void linkedListIterator_remove(linked_list_iterator_pt iterator);
UTILS_EXPORT void linkedListIterator_set(linked_list_iterator_pt iterator, void * element);
UTILS_EXPORT void linkedListIterator_add(linked_list_iterator_pt iterator, void * element);


#endif /* LINKED_LIST_ITERATOR_H_ */
