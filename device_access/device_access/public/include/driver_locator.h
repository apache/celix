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

#ifndef DRIVER_LOCATOR_H_
#define DRIVER_LOCATOR_H_

#include "celix_errno.h"
#include "properties.h"
#include "array_list.h"

#define DRIVER_LOCATOR_SERVICE_NAME "driver_locator"

typedef struct driver_locator *driver_locator_t;

struct driver_locator_service {
	driver_locator_t locator;
	celix_status_t(*findDrivers)(driver_locator_t loc, apr_pool_t *pool, PROPERTIES props, ARRAY_LIST *drivers);
	celix_status_t(*loadDriver)(driver_locator_t loc, apr_pool_t *pool, char *id, char **driver);
};

typedef struct driver_locator_service *driver_locator_service_t;


#endif /* DRIVER_LOCATOR_H_ */
