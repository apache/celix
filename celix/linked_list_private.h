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
