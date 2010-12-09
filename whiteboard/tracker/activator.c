/*
 * activator.c
 *
 *  Created on: Aug 23, 2010
 *      Author: alexanderb
 */
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

#include "bundle_activator.h"
#include "publisher.h"
#include "service_tracker.h"
#include "bundle_context.h"

struct data {
	BUNDLE_CONTEXT context;
	SERVICE_TRACKER tracker;
	ARRAY_LIST publishers;
	pthread_t sender;
	bool running;
};

void * trk_send(void * handle) {
	struct data * data = (struct data *) handle;
	while (data->running) {
		int i;
		for (i = 0; i < arrayList_size(data->publishers); i++) {
			PUBLISHER_SERVICE pub = arrayList_get(data->publishers, i);
			pub->invoke(pub->publisher, "test");
		}
		sleep(1);
	}
	pthread_exit(NULL);
}

void * addingServ(void * handle, SERVICE_REFERENCE ref) {
	struct data * data = (struct data *) handle;
	printf("Adding\n");
	return bundleContext_getService(data->context, ref);
}

void addedServ(void * handle, SERVICE_REFERENCE ref, void * service) {
	struct data * data = (struct data *) handle;
	arrayList_add(data->publishers, service);
	printf("Added\n");
}

void modifiedServ(void * handle, SERVICE_REFERENCE ref, void * service) {
	struct data * data = (struct data *) handle;
	printf("Modified\n");
}

void removedServ(void * handle, SERVICE_REFERENCE ref, void * service) {
	struct data * data = (struct data *) handle;
	arrayList_removeElement(data->publishers, service);
	printf("Removed\n");
}

void * bundleActivator_create() {
	struct data * data = malloc(sizeof(*data));
	data->publishers = arrayList_create();
	return data;
}

void bundleActivator_start(void * userData, BUNDLE_CONTEXT context) {
	struct data * data = (struct data *) userData;
	data->context = context;

	SERVICE_TRACKER_CUSTOMIZER cust = (SERVICE_TRACKER_CUSTOMIZER) malloc(sizeof(*cust));
	cust->handle = data;
	cust->addedService = addedServ;
	cust->addingService = addingServ;
	cust->modifiedService = modifiedServ;
	cust->removedService = removedServ;
	SERVICE_TRACKER tracker = tracker_create(context, (char *) PUBLISHER_NAME, cust);
	data->tracker = tracker;

	tracker_open(tracker);

	data->running = true;
	pthread_create(&data->sender, NULL, trk_send, data);
}

void bundleActivator_stop(void * userData, BUNDLE_CONTEXT context) {
	struct data * data = (struct data *) userData;
	tracker_close(data->tracker);
	data->running = false;
}

void bundleActivator_destroy(void * userData) {

}
