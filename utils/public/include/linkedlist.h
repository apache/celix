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

typedef struct linkedListEntry * linked_list_entry_t;
typedef struct linkedList * linked_list_t;

UTILS_EXPORT celix_status_t linkedList_create(apr_pool_t *pool, linked_list_t *list);
UTILS_EXPORT celix_status_t linkedList_clone(linked_list_t list, apr_pool_t *pool, linked_list_t *clone);
UTILS_EXPORT void * linkedList_getFirst(linked_list_t list);
UTILS_EXPORT void * linkedList_getLast(linked_list_t list);
UTILS_EXPORT void * linkedList_removeFirst(linked_list_t list);
UTILS_EXPORT void * linkedList_removeLast(linked_list_t list);
UTILS_EXPORT void linkedList_addFirst(linked_list_t list, void * element);
UTILS_EXPORT void linkedList_addLast(linked_list_t list, void * element);
UTILS_EXPORT bool linkedList_contains(linked_list_t list, void * element);
UTILS_EXPORT int linkedList_size(linked_list_t list);
UTILS_EXPORT bool linkedList_isEmpty(linked_list_t list);
UTILS_EXPORT bool linkedList_addElement(linked_list_t list, void * element);
UTILS_EXPORT bool linkedList_removeElement(linked_list_t list, void * element);
UTILS_EXPORT void linkedList_clear(linked_list_t list);
UTILS_EXPORT void * linkedList_get(linked_list_t list, int index);
UTILS_EXPORT void * linkedList_set(linked_list_t list, int index, void * element);
UTILS_EXPORT void linkedList_addIndex(linked_list_t list, int index, void * element);
UTILS_EXPORT void * linkedList_removeIndex(linked_list_t list, int index);
UTILS_EXPORT linked_list_entry_t linkedList_entry(linked_list_t list, int index);
UTILS_EXPORT int linkedList_indexOf(linked_list_t list, void * element);
UTILS_EXPORT linked_list_entry_t linkedList_addBefore(linked_list_t list, void * element, linked_list_entry_t entry);
UTILS_EXPORT void * linkedList_removeEntry(linked_list_t list, linked_list_entry_t entry);


#endif /* LINKEDLIST_H_ */
