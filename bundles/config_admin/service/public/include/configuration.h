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
 * configuration.h
 *
 *  \date       Aug 12, 2013
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_


#include <stdbool.h>
/* celix.framework */
#include "celix_errno.h"
#include "properties.h"


// Note: the term interface is used because configuration is not a service, it is not
//       registered in the service registry but it is owned by the config_admin
struct configuration {
	void *handle; // pointer to configuration datastore

	/* METHODS */
	celix_status_t (*configuration_delete)(void *handle);

	celix_status_t (*configuration_equals)(void *thisConfiguration, void *otherConfiguration, bool *equals);

	celix_status_t (*configuration_getBundleLocation)(void *handle, char **bundleLocation);
	celix_status_t (*configuration_getFactoryPid)(void *handle, char **factoryPid);
	celix_status_t (*configuration_getPid)(void *handle, char **pid);
	celix_status_t (*configuration_getProperties)(void *handle, properties_pt *properties);

	celix_status_t (*configuration_hashCode)(void *handle, int *hashCode);

	celix_status_t (*configuration_setBundleLocation)(void *handle, char *bundleLocation);

	celix_status_t (*configuration_update)(void *handle, properties_pt properties);

};

typedef struct configuration *configuration_pt;

/* METHODS
celix_status_t configuration_delete(configuration_pt configuration);

celix_status_t configuration_equals(configuration_pt thisConfiguration, configuration_pt otherConfiguration, bool *equals);

celix_status_t configuration_getBundleLocation(configuration_pt configuration, char **bundleLocation);
celix_status_t configuration_getFactoryPid(configuration_pt configuration, char **factoryPid);
celix_status_t configuration_getPid(configuration_pt configuration, char **pid);
celix_status_t configuration_getProperties(configuration_pt configuration, properties_pt *properties);

celix_status_t configuration_hashCode(configuration_pt configuration, int *hashCode);

celix_status_t configuration_setBundleLocation(configuration_pt configuration, char *bundleLocation);

celix_status_t configuration_update(configuration_pt configuration, properties_pt properties);
*/

#endif /* CONFIGURATION_H_ */
