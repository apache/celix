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
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef REMOTE_PROXY_H_
#define REMOTE_PROXY_H_

#include "endpoint_listener.h"

#define OSGI_RSA_REMOTE_PROXY "remote_proxy"

typedef celix_status_t (*sendToHandle)(void *handler, endpoint_description_pt endpointDescription, char *methodSignature, char *request, char **reply, int* replyStatus);

struct remote_proxy_service {
	void *proxy;
	celix_status_t (*setEndpointDescription)(void *proxy, endpoint_description_pt service);
	celix_status_t (*setHandler)(void *proxy, void *handler);
	celix_status_t (*setCallback)(void *proxy, sendToHandle callback);
};

typedef struct remote_proxy_service *remote_proxy_service_pt;

#endif /* REMOTE_PROXY_H_ */
