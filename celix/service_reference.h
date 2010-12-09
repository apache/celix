/*
 * service_reference.h
 *
 *  Created on: Jul 20, 2010
 *      Author: alexanderb
 */

#ifndef SERVICE_REFERENCE_H_
#define SERVICE_REFERENCE_H_

#include <stdbool.h>

#include "headers.h"

bool serviceReference_isAssignableTo(SERVICE_REFERENCE reference, BUNDLE requester, char * serviceName);

#endif /* SERVICE_REFERENCE_H_ */
