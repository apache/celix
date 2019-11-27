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
 * example_managed_service_impl.h
 *
 *  \date       Aug 12, 2013
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef MANAGED_SERVICE_IMPL_H_
#define MANAGED_SERVICE_IMPL_H_


/* celix.utils*/
#include "properties.h"
/* celix.framework */
#include "celix_errno.h"
#include "service_registration.h"
#include "bundle_context.h"
/* celix.config_admin.ManagedService */
#include "managed_service.h"

struct test_managed_service{

	bundle_context_pt 			context;

	service_registration_pt 	registration;
	properties_pt 				properties;
	void 						*handle;
	void (*store_props)(void *handle, char* type, char* second_type);

};

#define TST_SERVICE_NAME "tst_service"

struct tst_service {
    void *handle;
    int (*get_type)(void *handle, char *value);
    int (*get_second_type)(void *handle, char *value);
};

typedef struct tst_service *tst_service_pt;

celix_status_t managedServiceImpl_create(bundle_context_pt context, managed_service_pt *instance);
celix_status_t managedServiceImpl_updated(managed_service_pt instance, properties_pt properties);
celix_status_t managedServiceImpl_destroy(managed_service_pt *instance);



#endif /* MANAGED_SERVICE_IMPL_H_ */
