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
 *  \date       Aug 4, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef ARRAY_LIST_H_
#define ARRAY_LIST_H_

#include <apr_general.h>

#include "celixbool.h"
#include "exports.h"
#include "celix_errno.h"

typedef struct arrayList * array_list_t;

typedef struct arrayListIterator * array_list_iterator_t;

typedef celix_status_t (*arrayListElementEqualsFunction)(void *, void *, bool *equals);

UTILS_EXPORT celix_status_t arrayList_create(apr_pool_t *pool, array_list_t *list);
UTILS_EXPORT celix_status_t arrayList_createWithEquals(apr_pool_t *pool, arrayListElementEqualsFunction equals, array_list_t *list);

UTILS_EXPORT void arrayList_destroy(array_list_t list);
UTILS_EXPORT void arrayList_trimToSize(array_list_t list);
UTILS_EXPORT void arrayList_ensureCapacity(array_list_t list, int capacity);
UTILS_EXPORT unsigned int arrayList_size(array_list_t list);
UTILS_EXPORT bool arrayList_isEmpty(array_list_t list);
UTILS_EXPORT bool arrayList_contains(array_list_t list, void * element);
UTILS_EXPORT int arrayList_indexOf(array_list_t list, void * element);
UTILS_EXPORT int arrayList_lastIndexOf(array_list_t list, void * element);
UTILS_EXPORT void * arrayList_get(array_list_t list, unsigned int index);
UTILS_EXPORT void * arrayList_set(array_list_t list, unsigned int index, void * element);
UTILS_EXPORT bool arrayList_add(array_list_t list, void * element);
UTILS_EXPORT int arrayList_addIndex(array_list_t list, unsigned int index, void * element);
UTILS_EXPORT bool arrayList_addAll(array_list_t list, array_list_t toAdd);
UTILS_EXPORT void * arrayList_remove(array_list_t list, unsigned int index);
UTILS_EXPORT bool arrayList_removeElement(array_list_t list, void * element);
UTILS_EXPORT void arrayList_clear(array_list_t list);
UTILS_EXPORT array_list_t arrayList_clone(apr_pool_t *pool, array_list_t list);

UTILS_EXPORT array_list_iterator_t arrayListIterator_create(array_list_t list);
UTILS_EXPORT void arrayListIterator_destroy(array_list_iterator_t iterator);
UTILS_EXPORT bool arrayListIterator_hasNext(array_list_iterator_t iterator);
UTILS_EXPORT void * arrayListIterator_next(array_list_iterator_t iterator);
UTILS_EXPORT bool arrayListIterator_hasPrevious(array_list_iterator_t iterator);
UTILS_EXPORT void * arrayListIterator_previous(array_list_iterator_t iterator);
UTILS_EXPORT void arrayListIterator_remove(array_list_iterator_t iterator);

#endif /* ARRAY_LIST_H_ */
