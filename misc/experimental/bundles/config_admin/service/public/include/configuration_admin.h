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
 * configuration_admin.h
 *
 *  \date       Aug 12, 2013
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef CONFIGURATION_ADMIN_H_
#define CONFIGURATION_ADMIN_H_


/* celix.utils.public.include*/
#include "array_list.h"
/* celix.framework.public.include */
#include "celix_errno.h"
/* celix.config_admin.public.include */
#include "configuration.h"


/* Name of the class */
#define CONFIGURATION_ADMIN_SERVICE_NAME "org.osgi.service.cm.ConfigurationAdmin"
/* Configuration properties*/
#define SERVICE_BUNDLELOCATION "service.bundleLocation"
#define SERVICE_FACTORYPID "service.factoryPid"


typedef struct configuration_admin *configuration_admin_pt;
typedef struct configuration_admin_service *configuration_admin_service_pt;


struct configuration_admin_service {
	/* INSTANCE */
	configuration_admin_pt configAdmin;

	/* METHODS */
	celix_status_t (*createFactoryConfiguration)(configuration_admin_pt configAdmin, char *factoryPid, configuration_pt *configuration);
	celix_status_t (*createFactoryConfiguration2)(configuration_admin_pt configAdmin, char *factoryPid, char *location, configuration_pt *configuration);
	celix_status_t (*getConfiguration)(configuration_admin_pt configAdmin, char *pid, configuration_pt *configuration);
	celix_status_t (*getConfiguration2)(configuration_admin_pt configAdmin, char *pid, char *location, configuration_pt *configuration);
	celix_status_t (*listConfigurations)(configuration_admin_pt configAdmin, char *filter, array_list_pt *configurations);
};

#endif /* CONFIGURATION_ADMIN_H_ */
