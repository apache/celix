/*
 * serviceTest_private.h
 *
 *  Created on: Apr 22, 2010
 *      Author: dk489
 */

#ifndef SERVICETEST_PRIVATE_H_
#define SERVICETEST_PRIVATE_H_

#include "serviceTest.h"

SERVICE_DATA_TYPE serviceTest_construct(void);
void serviceTest_destruct(SERVICE_DATA_TYPE);

void doo(SERVICE_DATA_TYPE handle);


#endif /* SERVICETEST_PRIVATE_H_ */
