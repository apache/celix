/*
 * array_list_private.h
 *
 *  Created on: Aug 4, 2010
 *      Author: alexanderb
 */

#ifndef ARRAY_LIST_PRIVATE_H_
#define ARRAY_LIST_PRIVATE_H_

#include "array_list.h"

struct arrayList {
	void ** elementData;
	unsigned int size;
	unsigned int capacity;

	unsigned int modCount;
};

struct arrayListIterator {
	ARRAY_LIST list;
	unsigned int cursor;
	int lastReturned;
	unsigned int expectedModificationCount;
};

void * arrayList_remove(ARRAY_LIST list, unsigned int index);


#endif /* ARRAY_LIST_PRIVATE_H_ */
