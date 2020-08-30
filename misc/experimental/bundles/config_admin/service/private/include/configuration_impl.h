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
 * configuration_impl.h
 *
 *  \date       Aug 12, 2013
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */


#ifndef CONFIGURATION_IMPL_H_
#define CONFIGURATION_IMPL_H_

/*celix.config_admin.Configuration */
#include "configuration.h"

/* celix.framework */
#include "bundle_context.h"
/* celix.config_admin.private*/
#include "configuration_admin_factory.h"
#include "configuration_store.h"

typedef struct configuration_impl *configuration_impl_pt;
//typedef struct configuration_impl *configuration_pt;

#if 0







celix_status_t configuration_getFactoryPid2(configuration_pt configuration, bool checkDeleted, char **factoryPid);
celix_status_t configuration_getPid2(configuration_pt configuration, bool checkDeleted, char **pid);

celix_status_t configuration_isDeleted(configuration_pt configuration, bool *isDeleted);
#endif
celix_status_t configuration_lock(configuration_impl_pt configuration);
celix_status_t configuration_unlock(configuration_impl_pt configuration);
celix_status_t configuration_create( configuration_admin_factory_pt factory, configuration_store_pt store,
									 char *factoryPid, char *pid, char *bundleLocation,
									 configuration_pt *configuration);
celix_status_t configuration_create2(configuration_admin_factory_pt factory, configuration_store_pt store,
									 properties_pt dictionary,
									 configuration_pt *configuration);
celix_status_t configuration_getBundleLocation2(configuration_impl_pt configuration, bool checkPermission, char **location);
celix_status_t configuration_bind(configuration_impl_pt configuration, bundle_pt bundle, bool *isBind);
celix_status_t configuration_unbind(configuration_impl_pt configuration, bundle_pt bundle);
celix_status_t configuration_checkLocked(configuration_impl_pt configuration);
celix_status_t configuration_getAllProperties(configuration_impl_pt configuration, properties_pt *properties);

celix_status_t configuration_getPid(void *handle, char **pid);
celix_status_t configuration_getProperties(void *handle, properties_pt *properties);
#endif /* CONFIGURATION_IMPL_H_ */
