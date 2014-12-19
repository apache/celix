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
 * tracker.h
 *
 *  \date       Mar 7, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef TRACKER_H_
#define TRACKER_H_

#include "dm_component.h"
#include "log_service.h"

struct data {
	dm_component_pt service;
	bundle_context_pt context;
	array_list_pt publishers;
	pthread_t sender;
	bool running;
	log_service_pt logger;
};

celix_status_t tracker_addedServ(void * handle, service_reference_pt ref, void * service);
celix_status_t tracker_modifiedServ(void * handle, service_reference_pt ref, void * service);
celix_status_t tracker_removedServ(void * handle, service_reference_pt ref, void * service);

celix_status_t tracker_addLog(void * handle, service_reference_pt ref, void * service);
celix_status_t tracker_modifiedLog(void * handle, service_reference_pt ref, void * service);
celix_status_t tracker_removeLog(void * handle, service_reference_pt ref, void * service);

celix_status_t service_init(void * userData);
celix_status_t service_start(void * userData);
celix_status_t service_stop(void * userData);
celix_status_t service_destroy(void * userData);


#endif /* TRACKER_H_ */
