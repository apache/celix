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
 * configuration_admin_impl.h
 *
 *  \date       Aug 12, 2013
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef CONFIGURATION_ADMIN_IMP_H_
#define CONFIGURATION_ADMIN_IMP_H_


/* config_admin.ConfigAdmin*/
#include "configuration_admin.h"

/* celix.framework */
#include "bundle_private.h"
/* celix.config_admin.public */
#include "configuration.h"
/* celix.config_admin.private */
#include "configuration_admin_factory.h"
#include "configuration_store.h"


struct configuration_admin{

	bundle_pt bundle;

	configuration_admin_factory_pt configurationAdminFactory;
	configuration_store_pt configurationStore;
};


/* METHODS: celix.config_admin.private */
celix_status_t configurationAdmin_create(configuration_admin_factory_pt factory, configuration_store_pt store, bundle_pt bundle,
										 configuration_admin_service_pt *service);
celix_status_t configurationAdmin_destroy(configuration_admin_service_pt *service);

/* METHODS: celix.config_admin.public */
celix_status_t configurationAdmin_createFactoryConfiguration(configuration_admin_pt configAdmin, char *factoryPid, configuration_pt *configuration);
celix_status_t configurationAdmin_createFactoryConfiguration2(configuration_admin_pt configAdmin, char *factoryPid, char *location, configuration_pt *configuration);

celix_status_t configurationAdmin_getConfiguration(configuration_admin_pt configAdmin, char *pid, configuration_pt *configuration);
celix_status_t configurationAdmin_getConfiguration2(configuration_admin_pt configAdmin, char *pid, char *location, configuration_pt *configuration);

celix_status_t configurationAdmin_listConfigurations(configuration_admin_pt configAdmin, char *filter, array_list_pt *configurations);


#endif /* CONFIGURATION_ADMIN_IMP_H_ */

