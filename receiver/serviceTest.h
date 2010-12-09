/*
 * listenerTest.h
 *
 *  Created on: Apr 20, 2010
 *      Author: dk489
 */

#ifndef SERVICETEST_H_
#define SERVICETEST_H_

#define SERVICE_TEST_NAME "serviceTest"

typedef struct serviceDataType * SERVICE_DATA_TYPE;

struct serviceTest {
	SERVICE_DATA_TYPE handle;
	void (*doo)(SERVICE_DATA_TYPE handle);
};

typedef struct serviceTest * SERVICE_TEST;

#endif /* SERVICETEST_H_ */
