/*
 * service_registration.c
 *
 *  Created on: Aug 6, 2010
 *      Author: alexanderb
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "service_registration.h"
#include "constants.h"

SERVICE_REGISTRATION serviceRegistration_create(SERVICE_REGISTRY registry, BUNDLE bundle, char * serviceName, long serviceId, void * serviceObject, PROPERTIES dictionary) {
	SERVICE_REGISTRATION registration = (SERVICE_REGISTRATION) malloc(sizeof(*registration));

	registration->registry = registry;
	registration->className = serviceName;

	if (dictionary == NULL) {
		dictionary = createProperties();
	}

	char sId[sizeof(serviceId) + 1];
	sprintf(sId, "%ld", serviceId);
	setProperty(dictionary, (char *) SERVICE_ID, strdup(sId));
	setProperty(dictionary, (char *) OBJECTCLASS, serviceName);

	registration->properties = dictionary;

	registration->serviceId = serviceId;
	registration->svcObj = serviceObject;

	SERVICE_REFERENCE reference = (SERVICE_REFERENCE) malloc(sizeof(*reference));
	reference->bundle = bundle;
	reference->registration = registration;

	registration->reference = reference;

	registration->isUnregistering = false;
	pthread_mutex_init(&registration->mutex, NULL);

	return registration;
}

void serviceRegistration_destroy(SERVICE_REGISTRATION registration) {
	registration->className = NULL;
	registration->registry = NULL;

	registration->reference->bundle = NULL;
	registration->reference->registration = NULL;
	free(registration->reference);

	pthread_mutex_destroy(&registration->mutex);

	free(registration);
}

bool serviceRegistration_isValid(SERVICE_REGISTRATION registration) {
	return registration->svcObj != NULL;
}

void serviceRegistration_unregister(SERVICE_REGISTRATION registration) {
	pthread_mutex_lock(&registration->mutex);
	if (!serviceRegistration_isValid(registration) || registration->isUnregistering) {
		printf("Service is already unregistered\n");
		return;
	}
	registration->isUnregistering = true;
	pthread_mutex_unlock(&registration->mutex);

	serviceRegistry_unregisterService(registration->registry, registration->reference->bundle, registration);

	pthread_mutex_lock(&registration->mutex);
	registration->svcObj = NULL;
	pthread_mutex_unlock(&registration->mutex);
}
