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
 * configuration_plugin.h
 *
 *  \date       Aug 12, 2013
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef CONFIGURATION_PLUGIN_H_
#define CONFIGURATION_PLUGIN_H_


/* celix.utils.public.include*/
#include "hash_map.h"
/* celix.framework.public.include */
#include "celix_errno.h"
#include "service_reference.h"


/* Name of the class */
#define CONFIGURATION_PLUGIN_SERVICE_NAME "org.osgi.service.cm.ConfigurationPlugin"
/* Service properties*/
#define CONFIGURATION_PLUGIN_CM_RANKING "service.cmRanking"
#define CONFIGURATION_PLUGIN_CM_TARGET "cm.target"

typedef struct configuration_plugin *configuration_plugin_t;
typedef struct configuration_plugin_service *configuration_plugin_service_t;


struct configuration_plugin_service {

	/* INSTANCE */
	configuration_plugin_t configPlugin;

	/* METHOD */
	// reference to Managed Service or Managed Service Factory
	celix_status_t (*modifyConfiguration)(configuration_plugin_t configPlugin, service_reference_pt reference, hash_map_pt properties);

};



#endif /* CONFIGURATION_PLUGIN_H_ */
