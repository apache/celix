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

#define OSGI_RSA_REMOTE_PROXY_FACTORY 	"remote_proxy_factory"
#define OSGI_RSA_REMOTE_PROXY_TIMEOUT   "remote_proxy_timeout"

typedef celix_status_t (*sendToHandle)(remote_service_admin_pt remote_service_admin_ptr, endpoint_description_pt endpointDescription, char *request, char **reply, int* replyStatus);
typedef celix_status_t (*createProxyService)(void *handle, endpoint_description_pt endpointDescription, remote_service_admin_pt rsa, sendToHandle sendToCallback, properties_pt properties, void **service);
typedef celix_status_t (*destroyProxyService)(void *handle, void *service);

typedef struct remote_proxy_factory *remote_proxy_factory_pt;
typedef struct remote_proxy_factory_service *remote_proxy_factory_service_pt;

struct remote_proxy_factory {
	bundle_context_pt context_ptr;
	char *service;

	remote_proxy_factory_service_pt remote_proxy_factory_service_ptr;
	properties_pt properties;
	service_registration_pt registration;

	hash_map_pt proxy_instances;

	void *handle;

	createProxyService create_proxy_service_ptr;
	destroyProxyService destroy_proxy_service_ptr;
};

struct remote_proxy_factory_service {
	remote_proxy_factory_pt factory;
	celix_status_t (*registerProxyService)(remote_proxy_factory_pt proxyFactoryService, endpoint_description_pt endpoint, remote_service_admin_pt remote_service_admin_ptr, sendToHandle callback);
	celix_status_t (*unregisterProxyService)(remote_proxy_factory_pt proxyFactoryService, endpoint_description_pt endpoint);
};

celix_status_t remoteProxyFactory_create(bundle_context_pt context, char *service, void *handle,
		createProxyService create, destroyProxyService destroy,
		remote_proxy_factory_pt *remote_proxy_factory_ptr);
celix_status_t remoteProxyFactory_destroy(remote_proxy_factory_pt *remote_proxy_factory_ptr);

celix_status_t remoteProxyFactory_register(remote_proxy_factory_pt remote_proxy_factory_ptr);
celix_status_t remoteProxyFactory_unregister(remote_proxy_factory_pt remote_proxy_factory_ptr);




#endif /* REMOTE_PROXY_H_ */
