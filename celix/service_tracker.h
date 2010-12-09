/*
 * service_tracker.h
 *
 *  Created on: Apr 20, 2010
 *      Author: alexanderb
 */

#ifndef SERVICE_TRACKER_H_
#define SERVICE_TRACKER_H_

#include "headers.h"

struct fwServiceTracker {
	SERVICE_TRACKER tracker;
	SERVICE_TRACKER_CUSTOMIZER customizer;
	SERVICE_LISTENER listener;
	ARRAY_LIST tracked;
};

typedef struct fwServiceTracker * FW_SERVICE_TRACKER;

struct tracked {
	SERVICE_REFERENCE reference;
	void * service;
};

typedef struct tracked * TRACKED;

SERVICE_TRACKER tracker_create(BUNDLE_CONTEXT context, char * className, SERVICE_TRACKER_CUSTOMIZER customizer);

void tracker_open(SERVICE_TRACKER tracker);
void tracker_close(SERVICE_TRACKER tracker);
void tracker_destroy(SERVICE_TRACKER tracker);

SERVICE_REFERENCE tracker_getServiceReference(SERVICE_TRACKER tracker);
ARRAY_LIST tracker_getServiceReferences(SERVICE_TRACKER tracker);

void * tracker_getService(SERVICE_TRACKER tracker);
ARRAY_LIST tracker_getServices(SERVICE_TRACKER tracker);
void * tracker_getServiceByReference(SERVICE_TRACKER tracker, SERVICE_REFERENCE reference);

void tracker_serviceChanged(SERVICE_LISTENER listener, SERVICE_EVENT event);

FW_SERVICE_TRACKER findFwServiceTracker(SERVICE_TRACKER tracker);

#endif /* SERVICE_TRACKER_H_ */
