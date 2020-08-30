/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
/**
 * configuration_event.h
 *
 *  \date       Aug 12, 2013
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef CONFIGURATION_EVENT_H_
#define CONFIGURATION_EVENT_H_


/* celix.framework.public.include */
#include "celix_errno.h"
#include "service_reference.h"


#define CONFIGURATION_EVENT_CM_DELETED 2
#define CONFIGURATION_EVENT_CM_LOCATION_CHANGED 3
#define CONFIGURATION_EVENT_CM_UPDATED 1


typedef struct configuration_event *configuration_event_t;


/* METHODS */
celix_status_t configurationEvent_create(
		service_reference_pt referenceConfigAdmin,
		int type, char* factoryPid, char *pid,
		configuration_event_t *event);

celix_status_t configurationEvent_getFactoryPid(configuration_event_t event, char **factoryPid);
celix_status_t configurationEvent_getPid(configuration_event_t event, char **pid);
celix_status_t configurationEvent_getReference(configuration_event_t event, service_reference_pt *referenceConfigAdmin);
celix_status_t configurationEvent_getType(configuration_event_t event, int *type);


#endif /* CONFIGURATION_EVENT_H_ */
