/*
 * tracker.c
 *
 *  Created on: Mar 7, 2011
 *      Author: alexanderb
 */
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

#include "service.h"
#include "publisher.h"
#include "tracker.h"

void * dp_send(void * handle) {
	struct data * data = (struct data *) handle;
	while (data->running) {
		int i;
		for (i = 0; i < arrayList_size(data->publishers); i++) {
			PUBLISHER_SERVICE pub = (PUBLISHER_SERVICE) arrayList_get(data->publishers, i);
			pub->invoke(pub->publisher, "Tracker message");
		}
		sleep(1);
	}
	pthread_exit(NULL);
	return NULL;
}

void service_init(void * userData) {

}

void service_start(void * userData) {
	struct data * data = (struct data *) userData;
	data->running = true;
	pthread_create(&data->sender, NULL, dp_send, data);
}

void service_stop(void * userData) {
	struct data * data = (struct data *) userData;
	data->running = false;
	pthread_join(data->sender, NULL);
}

void service_destroy(void * userData) {

}

void tracker_addedServ(void * handle, SERVICE_REFERENCE ref, void * service) {
	struct data * data = (struct data *) handle;
	arrayList_add(data->publishers, service);
	printf("Service Added\n");
}

void tracker_modifiedServ(void * handle, SERVICE_REFERENCE ref, void * service) {
	struct data * data = (struct data *) handle;
	printf("Service Changed\n");
}

void tracker_removedServ(void * handle, SERVICE_REFERENCE ref, void * service) {
	struct data * data = (struct data *) handle;
	arrayList_removeElement(data->publishers, service);
	printf("Service Removed\n");
}

