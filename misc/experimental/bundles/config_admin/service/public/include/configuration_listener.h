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
 * configuration_listener.h
 *
 *  \date       Aug 12, 2013
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef CONFIGURATION_LISTENER_H_
#define CONFIGURATION_LISTENER_H_


/* celix.utils.public.include*/
#include "array_list.h"
/* celix.framework.public.include */
#include "celix_errno.h"
/* celix.config_admin.public.include */
#include "configuration_event.h"

/* Name of the class */
#define CONFIGURATION_LISTENER_SERVICE_NAME "org.osgi.service.cm.ConfigurationListener"


typedef struct configuration_listener *configuration_listener_t;
typedef struct configuration_listener_service *configuration_listener_service_t;


struct configuration_listener_service {

	/* INSTANCE */
	configuration_listener_t configListener;

	/* METHOD */
	celix_status_t (*configurationEvent)(configuration_listener_t configListener, configuration_event_t event);

};


#endif /* CONFIGURATION_LISTENER_H_ */
