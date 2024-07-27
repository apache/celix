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
 * configuration_admin_factory.h
 *
 *  \date       Aug 12, 2013
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef CONFIGURATION_ADMIN_FACTORY_H_
#define CONFIGURATION_ADMIN_FACTORY_H_

#include <stdbool.h>

/* celix.framework */
#include "bundle_context.h"
#include "bundle_private.h"
#include "celix_errno.h"
#include "properties.h"
#include "service_factory.h"
#include "service_reference.h"
/* celix.config_admin.public*/
#include "configuration_admin.h"


typedef struct configuration_admin_factory *configuration_admin_factory_pt;


/* METHODS */

celix_status_t configurationAdminFactory_create(bundle_context_pt context, service_factory_pt *factory, configuration_admin_factory_pt *instance);
celix_status_t configurationAdminFactory_destroy( bundle_context_pt context, configuration_admin_factory_pt instance);
celix_status_t configurationAdminFactory_start(configuration_admin_factory_pt factory);
celix_status_t configurationAdminFactory_stop(configuration_admin_factory_pt factory);

// public void bundleChanged(BundleEvent event)

celix_status_t configurationAdminFactory_checkConfigurationPermission(configuration_admin_factory_pt factory);

celix_status_t configurationAdminFactory_dispatchEvent(configuration_admin_factory_pt factory, int type, char *factoryPid, char *pid);

celix_status_t configurationAdminFactory_notifyConfigurationUpdated(configuration_admin_factory_pt factory, configuration_pt configuration, bool isFactory);
celix_status_t configurationAdminFactory_notifyConfigurationDeleted(configuration_admin_factory_pt factory, configuration_pt configuration, bool isFactory);

celix_status_t configurationAdminFactory_modifyConfiguration(configuration_admin_factory_pt factory, service_reference_pt reference, properties_pt properties);

#endif /* CONFIGURATION_ADMIN_FACTORY_H_ */
