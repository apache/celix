/*
 * service_registration.h
 *
 *  Created on: Aug 6, 2010
 *      Author: alexanderb
 */

#ifndef SERVICE_REGISTRATION_H_
#define SERVICE_REGISTRATION_H_

#include <stdbool.h>

#include "headers.h"
#include "service_registry.h"

SERVICE_REGISTRATION serviceRegistration_create(SERVICE_REGISTRY registry, BUNDLE bundle, char * serviceName, long serviceId, void * serviceObject, PROPERTIES dictionary);
bool serviceRegistration_isValid(SERVICE_REGISTRATION registration);
void serviceRegistration_unregister(SERVICE_REGISTRATION registration);

#endif /* SERVICE_REGISTRATION_H_ */
