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
 * service_registry_private.h
 *
 *  \date       Feb 7, 2013
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */


#ifndef SERVICE_REGISTRY_PRIVATE_H_
#define SERVICE_REGISTRY_PRIVATE_H_

#include "registry_callback_private.h"
#include "service_registry.h"

struct celix_serviceRegistry {
	framework_pt framework;
	registry_callback_t callback;

	hash_map_pt serviceRegistrations; //key = bundle (reg owner), value = list ( registration )
	hash_map_pt serviceReferences; //key = bundle, value = map (key = serviceId, value = reference)

	bool checkDeletedReferences; //If enabled. check if provided service references are still valid
	hash_map_pt deletedServiceReferences; //key = ref pointer, value = bool

	serviceChanged_function_pt serviceChanged;
	unsigned long currentServiceId;

	array_list_pt listenerHooks;

	celix_thread_rwlock_t lock;
};

typedef enum reference_status_enum {
	REF_ACTIVE,
	REF_DELETED,
	REF_UNKNOWN
} reference_status_t;

struct usageCount {
	unsigned int count;
	service_reference_pt reference;
	void * service;
	service_registration_pt registration;
};

typedef struct usageCount * usage_count_pt;

#endif /* SERVICE_REGISTRY_PRIVATE_H_ */
