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
 *  Created on: Mar 7, 2011
 *      Author: alexanderb
 */

#ifndef TRACKER_H_
#define TRACKER_H_

#include "service_component.h"
#include "log_service.h"

struct data {
	SERVICE service;
	BUNDLE_CONTEXT context;
	ARRAY_LIST publishers;
	pthread_t sender;
	bool running;
	log_service_t logger;
};

void tracker_addedServ(void * handle, SERVICE_REFERENCE ref, void * service);
void tracker_modifiedServ(void * handle, SERVICE_REFERENCE ref, void * service);
void tracker_removedServ(void * handle, SERVICE_REFERENCE ref, void * service);

void tracker_addLog(void * handle, SERVICE_REFERENCE ref, void * service);
void tracker_modifiedLog(void * handle, SERVICE_REFERENCE ref, void * service);
void tracker_removeLog(void * handle, SERVICE_REFERENCE ref, void * service);


#endif /* TRACKER_H_ */
