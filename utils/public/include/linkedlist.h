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
 *  \date       Jul 16, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef LINKEDLIST_H_
#define LINKEDLIST_H_

#include <apr_pools.h>

#include "celixbool.h"
#include "celix_errno.h"
#include "exports.h"

typedef struct linkedListEntry * LINKED_LIST_ENTRY;
typedef struct linkedList * LINKED_LIST;

UTILS_EXPORT celix_status_t linkedList_create(apr_pool_t *pool, LINKED_LIST *list);
UTILS_EXPORT celix_status_t linkedList_clone(LINKED_LIST list, apr_pool_t *pool, LINKED_LIST *clone);
UTILS_EXPORT void * linkedList_getFirst(LINKED_LIST list);
UTILS_EXPORT void * linkedList_getLast(LINKED_LIST list);
UTILS_EXPORT void * linkedList_removeFirst(LINKED_LIST list);
UTILS_EXPORT void * linkedList_removeLast(LINKED_LIST list);
UTILS_EXPORT void linkedList_addFirst(LINKED_LIST list, void * element);
UTILS_EXPORT void linkedList_addLast(LINKED_LIST list, void * element);
UTILS_EXPORT bool linkedList_contains(LINKED_LIST list, void * element);
UTILS_EXPORT int linkedList_size(LINKED_LIST list);
UTILS_EXPORT bool linkedList_isEmpty(LINKED_LIST list);
UTILS_EXPORT bool linkedList_addElement(LINKED_LIST list, void * element);
UTILS_EXPORT bool linkedList_removeElement(LINKED_LIST list, void * element);
UTILS_EXPORT void linkedList_clear(LINKED_LIST list);
UTILS_EXPORT void * linkedList_get(LINKED_LIST list, int index);
UTILS_EXPORT void * linkedList_set(LINKED_LIST list, int index, void * element);
UTILS_EXPORT void linkedList_addIndex(LINKED_LIST list, int index, void * element);
UTILS_EXPORT void * linkedList_removeIndex(LINKED_LIST list, int index);
UTILS_EXPORT LINKED_LIST_ENTRY linkedList_entry(LINKED_LIST list, int index);
UTILS_EXPORT int linkedList_indexOf(LINKED_LIST list, void * element);
UTILS_EXPORT LINKED_LIST_ENTRY linkedList_addBefore(LINKED_LIST list, void * element, LINKED_LIST_ENTRY entry);
UTILS_EXPORT void * linkedList_removeEntry(LINKED_LIST list, LINKED_LIST_ENTRY entry);


#endif /* LINKEDLIST_H_ */
