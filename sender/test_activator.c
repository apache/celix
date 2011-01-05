/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */
/*
 * activator.c
 *
 *  Created on: Apr 20, 2010
 *      Author: dk489
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bundle_context.h"
#include "listenerTest.h"
#include "service_tracker.h"
#include "serviceTest.h"
#include "dependency_activator_base.h"
#include "dependency_manager.h"
#include "service_dependency.h"
#include "service_component_private.h"

SERVICE_LISTENER listener;
BUNDLE_CONTEXT m_context;
SERVICE_TRACKER tracker;

void * addingServ(void * handle, SERVICE_REFERENCE ref);

void addedServ(void * handle, SERVICE_REFERENCE ref, void * service);

void modifiedServ(void * handle, SERVICE_REFERENCE ref, void * service);

void removedServ(void * handle, SERVICE_REFERENCE ref, void * service);

struct sender {
	SERVICE_TEST service;
};

typedef struct sender * SENDER;

void * dm_create() {
	SENDER s = (SENDER) malloc(sizeof(*s));
	return s;
}

void dm_init(void * userData, BUNDLE_CONTEXT context, DEPENDENCY_MANAGER manager) {
	SENDER s = (SENDER) userData;
	SERVICE service = dependencyActivatorBase_createService(manager);
	serviceComponent_setImplementation(service, userData);
	SERVICE_DEPENDENCY dep = dependencyActivatorBase_createServiceDependency(manager);
	serviceDependency_setRequired(dep, true);
	serviceDependency_setService(dep, SERVICE_TEST_NAME, NULL);
	serviceDependency_setAutoConfigure(dep, (void**) &s->service);
	serviceComponent_addServiceDependency(service, dep);
	dependencyManager_add(manager, service);
}

void dm_destroy(void * userData, BUNDLE_CONTEXT context, DEPENDENCY_MANAGER manager) {

}

void service_init(void * userData) {

}
void service_start(void * userData) {
	SENDER s = (SENDER) userData;
	s->service->doo(s->service->handle);
}
void service_stop(void * userData) {

}
void service_destroy(void * userData) {

}

//void bundleActivator_start(void * userData, BUNDLE_CONTEXT context) {
//	ARRAY_LIST refs;
//
//	SERVICE_TEST st;
//	SERVICE_TRACKER_CUSTOMIZER cust = (SERVICE_TRACKER_CUSTOMIZER) malloc(sizeof(*cust));
//	m_context = context;
//	cust->addedService = addedServ;
//	cust->addingService = addingServ;
//	cust->modifiedService = modifiedServ;
//	cust->removedService = removedServ;
//
//	listener = (SERVICE_LISTENER) malloc(sizeof(*listener));
//	listener->serviceChanged = (void *) serviceChanged;
//	addServiceListener(context, listener, "(test=test)");
//	tracker = tracker_createServiceTracker(context, (char *) SERVICE_TEST_NAME, cust);
//	tracker_open(tracker);
//	refs = tracker_getServices(tracker);
//	printf("Size: %i\n", arrayList_size(refs));
//	//arrayList_clear(refs);
//	//free(refs);
//	if (arrayList_size(refs) > 0) {
//		st = (SERVICE_TEST) arrayList_get(refs, 0);
//		st->doo(st->handle);
//	}
//	//refs2 = getServiceReferences(context, (char *) SERVICE_TEST_NAME, strdup("(test=t*st)"));
//	//printf("Size of getServiceRefs for TEST: %i\n", arrayList_size(refs2));
//	//refs2 = getServiceReferences(context, (char *) SERVICE_TEST_NAME, strdup("(!(test=testa))"));
//	//printf("Size of getServiceRefs for TEST: %i\n", arrayList_size(refs2));
//	//refs2 = getServiceReferences(context, (char *) SERVICE_TEST_NAME, NULL);
//	//printf("Size of getServiceRefs for TEST: %i\n", arrayList_size(refs2));
//}

//void bundleActivator_stop(void * userData, BUNDLE_CONTEXT context) {
//	// removeServiceListener(context, listener);
//	tracker_close(tracker);
//	tracker_destroy(tracker);
//
//}

void * addingServ(void * handle, SERVICE_REFERENCE ref) {
	printf("Adding\n");
	return bundleContext_getService(m_context, ref);
}

void addedServ(void * handle, SERVICE_REFERENCE ref, void * service) {
	printf("Added\n");
}

void modifiedServ(void * handle, SERVICE_REFERENCE ref, void * service) {
	printf("mod\n");
}

void removedServ(void * handle, SERVICE_REFERENCE ref, void * service) {
	printf("rem\n");
}
