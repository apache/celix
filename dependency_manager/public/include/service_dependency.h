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
 *  \date       Aug 9, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef SERVICE_DEPENDENCY_H_
#define SERVICE_DEPENDENCY_H_

#include <celixbool.h>

#include "service_reference.h"
#include "service_tracker.h"

struct serviceDependency {
	char * interface;
	void (*added)(void * handle, service_reference_pt reference, void *);
	void (*changed)(void * handle, service_reference_pt reference, void *);
	void (*removed)(void * handle, service_reference_pt reference, void *);
	void ** autoConfigureField;

	bool started;
	bool available;
	bool required;
	service_tracker_pt tracker;
	service_pt service;
	service_reference_pt reference;
	bundle_context_pt context;
	void * serviceInstance;
	char * trackedServiceName;
	char * trackedServiceFilter;
};

typedef struct serviceDependency * service_dependency_pt;

service_dependency_pt serviceDependency_create(bundle_context_pt context);
void * serviceDependency_getService(service_dependency_pt dependency);

service_dependency_pt serviceDependency_setRequired(service_dependency_pt dependency, bool required);
service_dependency_pt serviceDependency_setService(service_dependency_pt dependency, char * serviceName, char * filter);
service_dependency_pt serviceDependency_setCallbacks(service_dependency_pt dependency, void (*added)(void * handle, service_reference_pt reference, void *),
		void (*changed)(void * handle, service_reference_pt reference, void *),
		void (*removed)(void * handle, service_reference_pt reference, void *));
service_dependency_pt serviceDependency_setAutoConfigure(service_dependency_pt dependency, void ** field);

void serviceDependency_start(service_dependency_pt dependency, service_pt service);
void serviceDependency_stop(service_dependency_pt dependency, service_pt service);

void serviceDependency_invokeAdded(service_dependency_pt dependency);
void serviceDependency_invokeRemoved(service_dependency_pt dependency);


#endif /* SERVICE_DEPENDENCY_H_ */
