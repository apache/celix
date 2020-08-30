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
 * consuming_driver_private.h
 *
 *  \date       Jun 20, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef CONSUMING_DRIVER_PRIVATE_H_
#define CONSUMING_DRIVER_PRIVATE_H_

#include <celix_errno.h>
#include <service_reference.h>
#include <driver.h>

#define CONSUMING_DRIVER_ID "CONSUMING_DRIVER"

typedef struct consuming_driver *consuming_driver_pt;

celix_status_t consumingDriver_create(bundle_context_pt context, consuming_driver_pt *driver);
celix_status_t consumingDriver_createService(consuming_driver_pt driver, driver_service_pt *service);

celix_status_t consumingDriver_attach(void *driver, service_reference_pt reference, char **result);
celix_status_t consumingDriver_match(void *driver, service_reference_pt reference, int *value);

#endif /* CONSUMING_DRIVER_PRIVATE_H_ */
