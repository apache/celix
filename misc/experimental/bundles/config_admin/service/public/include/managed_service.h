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
 * managed_service.h
 *
 *  \date       Aug 12, 2013
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef MANAGED_SERVICE_H_
#define MANAGED_SERVICE_H_


/* celix.framework */
#include "bundle_context.h"
#include "celix_errno.h"
#include "properties.h"

/* Name of the class */
#define MANAGED_SERVICE_SERVICE_NAME "org.osgi.service.cm.ManagedService"


typedef struct managed_service *managed_service_pt;
typedef struct managed_service_service *managed_service_service_pt;

struct managed_service_service{

	managed_service_pt managedService;
	/* METHODS */
	celix_status_t (*updated)(managed_service_pt managedService, properties_pt properties);

};

celix_status_t managedService_create(bundle_context_pt context, managed_service_service_pt *service);
celix_status_t managedService_destroy(managed_service_service_pt service);

#endif /* MANAGED_SERVICE_H_ */
