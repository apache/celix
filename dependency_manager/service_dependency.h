/*
 * service_dependency.h
 *
 *  Created on: Aug 9, 2010
 *      Author: alexanderb
 */

#ifndef SERVICE_DEPENDENCY_H_
#define SERVICE_DEPENDENCY_H_

#include <stdbool.h>

#include "headers.h"

struct serviceDependency {
	char * interface;
	void (*added)(void * handle, SERVICE_REFERENCE reference, void *);
	void (*changed)(void * handle, SERVICE_REFERENCE reference, void *);
	void (*removed)(void * handle, SERVICE_REFERENCE reference, void *);
	void ** autoConfigureField;

	bool started;
	bool available;
	bool required;
	SERVICE_TRACKER tracker;
	SERVICE service;
	SERVICE_REFERENCE reference;
	BUNDLE_CONTEXT context;
	void * serviceInstance;
	char * trackedServiceName;
};

typedef struct serviceDependency * SERVICE_DEPENDENCY;

SERVICE_DEPENDENCY serviceDependency_create(BUNDLE_CONTEXT context);
void * serviceDependency_getService(SERVICE_DEPENDENCY dependency);

SERVICE_DEPENDENCY serviceDependency_setRequired(SERVICE_DEPENDENCY dependency, bool required);
SERVICE_DEPENDENCY serviceDependency_setService(SERVICE_DEPENDENCY dependency, char * serviceName, char * filter);
SERVICE_DEPENDENCY serviceDependency_setCallbacks(SERVICE_DEPENDENCY dependency, void (*added)(void * handle, SERVICE_REFERENCE reference, void *),
		void (*changed)(void * handle, SERVICE_REFERENCE reference, void *),
		void (*removed)(void * handle, SERVICE_REFERENCE reference, void *));
SERVICE_DEPENDENCY serviceDependency_setAutoConfigure(SERVICE_DEPENDENCY dependency, void ** field);

void serviceDependency_start(SERVICE_DEPENDENCY dependency, SERVICE service);
void serviceDependency_stop(SERVICE_DEPENDENCY dependency, SERVICE service);

void serviceDependency_invokeAdded(SERVICE_DEPENDENCY dependency);
void serviceDependency_invokeRemoved(SERVICE_DEPENDENCY dependency);


#endif /* SERVICE_DEPENDENCY_H_ */
