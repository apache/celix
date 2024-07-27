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
 * configuration_store.h
 *
 *  \date       Aug 12, 2013
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */


#ifndef CONFIGURATION_STORE_H_
#define CONFIGURATION_STORE_H_

/* celix.utils.public*/
#include "array_list.h"
/* celix.framework */
#include "bundle_context.h"
#include "bundle_private.h"
#include "filter.h"
/* celix.config_admin.public */
#include "configuration.h"
/* celix.config_admin.private */
#include "configuration_admin_factory.h"

typedef struct configuration_store *configuration_store_pt;

/* METHODS */
celix_status_t configurationStore_create(bundle_context_pt context, configuration_admin_factory_pt factory, configuration_store_pt *store);
celix_status_t configurationStore_destroy(configuration_store_pt store);

celix_status_t configurationStore_lock(configuration_store_pt store);
celix_status_t configurationStore_unlock(configuration_store_pt store);

celix_status_t configurationStore_saveConfiguration(configuration_store_pt store, char *pid, configuration_pt configuration);
celix_status_t configurationStore_removeConfiguration(configuration_store_pt store, char *pid);

celix_status_t configurationStore_getConfiguration(configuration_store_pt store, char *pid, char *location, configuration_pt *configuration);
celix_status_t configurationStore_createFactoryConfiguration(configuration_store_pt store, char *factoryPid, char *location, configuration_pt *configuration);

celix_status_t configurationStore_findConfiguration(configuration_store_pt store, char *pid, configuration_pt *configuration);

celix_status_t configurationStore_getFactoryConfigurations(configuration_store_pt store, char *factoryPid, configuration_pt *configuration);

celix_status_t configurationStore_listConfigurations(configuration_store_pt store, filter_pt filter, array_list_pt *configurations);

celix_status_t configurationStore_unbindConfigurations(configuration_store_pt store, bundle_pt bundle);

#endif /* CONFIGURATION_STORE_H_ */
