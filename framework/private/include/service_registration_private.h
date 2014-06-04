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
 * service_registration_private.h
 *
 *  \date       Feb 11, 2013
 *  \author     <a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */


#ifndef SERVICE_REGISTRATION_PRIVATE_H_
#define SERVICE_REGISTRATION_PRIVATE_H_

#include "service_registration.h"

struct service {
	char *name;
	void *serviceStruct;
};

struct serviceRegistration {
	service_registry_pt registry;
	char * className;
	array_list_pt references;
	bundle_pt bundle;
	properties_pt properties;
	void * svcObj;
	long serviceId;

	celix_thread_mutex_t mutex;
	bool isUnregistering;

	bool isServiceFactory;
	void *serviceFactory;

	struct service *services;
	int nrOfServices;
};

#endif /* SERVICE_REGISTRATION_PRIVATE_H_ */
