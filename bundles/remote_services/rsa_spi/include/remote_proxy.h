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
 * remote_proxy.h
 *
 *  \date       Oct 13, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef REMOTE_PROXY_H_
#define REMOTE_PROXY_H_

#include "endpoint_listener.h"
#include "remote_service_admin.h"

#define CELIX_RSA_REMOTE_PROXY_FACTORY 	"remote_proxy_factory"
#define CELIX_RSA_REMOTE_PROXY_TIMEOUT   "remote_proxy_timeout"

typedef celix_status_t (*sendToHandle)(remote_service_admin_t *remote_service_admin_ptr, endpoint_description_t *endpointDescription, char *request, char **reply, int* replyStatus);
typedef celix_status_t (*createProxyService)(void *handle, endpoint_description_t *endpointDescription, remote_service_admin_t *rsa, sendToHandle sendToCallback, celix_properties_t *properties, void **service);
typedef celix_status_t (*destroyProxyService)(void *handle, void *service);

typedef struct remote_proxy_factory remote_proxy_factory_t;
typedef struct remote_proxy_factory_service remote_proxy_factory_service_t;

struct remote_proxy_factory {
	celix_bundle_context_t *context_ptr;
	char *service;

	remote_proxy_factory_service_t *remote_proxy_factory_service_ptr;
	celix_properties_t *properties;
	service_registration_t *registration;

	hash_map_pt proxy_instances;

	void *handle;

	createProxyService create_proxy_service_ptr;
	destroyProxyService destroy_proxy_service_ptr;
};

struct remote_proxy_factory_service {
	remote_proxy_factory_t *factory;
	celix_status_t (*registerProxyService)(remote_proxy_factory_t *proxyFactoryService, endpoint_description_t *endpoint, remote_service_admin_t *remote_service_admin_ptr, sendToHandle callback);
	celix_status_t (*unregisterProxyService)(remote_proxy_factory_t *proxyFactoryService, endpoint_description_t *endpoint);
};

celix_status_t remoteProxyFactory_create(celix_bundle_context_t *context, char *service, void *handle,
		createProxyService create, destroyProxyService destroy,
		remote_proxy_factory_t **remote_proxy_factory_ptr);
celix_status_t remoteProxyFactory_destroy(remote_proxy_factory_t **remote_proxy_factory_ptr);

celix_status_t remoteProxyFactory_register(remote_proxy_factory_t *remote_proxy_factory_ptr);
celix_status_t remoteProxyFactory_unregister(remote_proxy_factory_t *remote_proxy_factory_ptr);




#endif /* REMOTE_PROXY_H_ */
