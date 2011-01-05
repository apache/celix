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
 * linked_list_private.h
 *
 *  Created on: Jul 16, 2010
 *      Author: alexanderb
 */

#ifndef LINKED_LIST_PRIVATE_H_
#define LINKED_LIST_PRIVATE_H_

#include "linkedlist.h"

struct linkedListEntry {
	void * element;
	struct linkedListEntry * next;
	struct linkedListEntry * previous;
};

struct linkedList {
	LINKED_LIST_ENTRY header;
	size_t size;
	int modificationCount;
};

#endif /* LINKED_LIST_PRIVATE_H_ */
