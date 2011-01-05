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
