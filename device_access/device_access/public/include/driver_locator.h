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
 * driver_locator.h
 *
 *  \date       Jun 20, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#ifndef DRIVER_LOCATOR_H_
#define DRIVER_LOCATOR_H_

#include "celix_errno.h"
#include "properties.h"
#include "array_list.h"

#define OSGI_DEVICEACCESS_DRIVER_LOCATOR_SERVICE_NAME "driver_locator"

typedef struct driver_locator *driver_locator_pt;

struct driver_locator_service {
	driver_locator_pt locator;
	celix_status_t(*findDrivers)(driver_locator_pt loc, apr_pool_t *pool, properties_pt props, array_list_pt *drivers);
	celix_status_t(*loadDriver)(driver_locator_pt loc, apr_pool_t *pool, char *id, char **driver);
};

typedef struct driver_locator_service *driver_locator_service_pt;


#endif /* DRIVER_LOCATOR_H_ */
