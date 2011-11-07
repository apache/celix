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

#include <apr_general.h>

#include "celixbool.h"
#include "exports.h"
#include "celix_errno.h"

typedef struct arrayList * ARRAY_LIST;

typedef struct arrayListIterator * ARRAY_LIST_ITERATOR;

celix_status_t arrayList_create(apr_pool_t *pool, ARRAY_LIST *list);
UTILS_EXPORT void arrayList_destroy(ARRAY_LIST list);
UTILS_EXPORT void arrayList_trimToSize(ARRAY_LIST list);
UTILS_EXPORT void arrayList_ensureCapacity(ARRAY_LIST list, int capacity);
UTILS_EXPORT unsigned int arrayList_size(ARRAY_LIST list);
UTILS_EXPORT bool arrayList_isEmpty(ARRAY_LIST list);
UTILS_EXPORT bool arrayList_contains(ARRAY_LIST list, void * element);
UTILS_EXPORT int arrayList_indexOf(ARRAY_LIST list, void * element);
UTILS_EXPORT int arrayList_lastIndexOf(ARRAY_LIST list, void * element);
UTILS_EXPORT void * arrayList_get(ARRAY_LIST list, unsigned int index);
UTILS_EXPORT void * arrayList_set(ARRAY_LIST list, unsigned int index, void * element);
UTILS_EXPORT bool arrayList_add(ARRAY_LIST list, void * element);
UTILS_EXPORT int arrayList_addIndex(ARRAY_LIST list, unsigned int index, void * element);
UTILS_EXPORT bool arrayList_addAll(ARRAY_LIST list, ARRAY_LIST toAdd);
UTILS_EXPORT void * arrayList_remove(ARRAY_LIST list, unsigned int index);
UTILS_EXPORT bool arrayList_removeElement(ARRAY_LIST list, void * element);
UTILS_EXPORT void arrayList_clear(ARRAY_LIST list);
ARRAY_LIST arrayList_clone(apr_pool_t *pool, ARRAY_LIST list);

UTILS_EXPORT ARRAY_LIST_ITERATOR arrayListIterator_create(ARRAY_LIST list);
UTILS_EXPORT void arrayListIterator_destroy(ARRAY_LIST_ITERATOR iterator);
UTILS_EXPORT bool arrayListIterator_hasNext(ARRAY_LIST_ITERATOR iterator);
UTILS_EXPORT void * arrayListIterator_next(ARRAY_LIST_ITERATOR iterator);
UTILS_EXPORT bool arrayListIterator_hasPrevious(ARRAY_LIST_ITERATOR iterator);
UTILS_EXPORT void * arrayListIterator_previous(ARRAY_LIST_ITERATOR iterator);
UTILS_EXPORT void arrayListIterator_remove(ARRAY_LIST_ITERATOR iterator);

#endif /* ARRAY_LIST_H_ */
