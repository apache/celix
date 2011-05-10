/*
 * tracker.h
 *
 *  Created on: Mar 7, 2011
 *      Author: alexanderb
 */

#ifndef TRACKER_H_
#define TRACKER_H_

#include "headers.h"
#include "service_component.h"

struct data {
	SERVICE service;
	BUNDLE_CONTEXT context;
	ARRAY_LIST publishers;
	pthread_t sender;
	bool running;
};

void tracker_addedServ(void * handle, SERVICE_REFERENCE ref, void * service);
void tracker_modifiedServ(void * handle, SERVICE_REFERENCE ref, void * service);
void tracker_removedServ(void * handle, SERVICE_REFERENCE ref, void * service);


#endif /* TRACKER_H_ */
