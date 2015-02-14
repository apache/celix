/*
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
 * listener_hook_service.h
 *
 *  \date       Oct 28, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef LISTENER_HOOK_SERVICE_H_
#define LISTENER_HOOK_SERVICE_H_


typedef struct listener_hook *listener_hook_pt;
typedef struct listener_hook_info *listener_hook_info_pt;
typedef struct listener_hook_service *listener_hook_service_pt;

#include "bundle_context.h"

#define OSGI_FRAMEWORK_LISTENER_HOOK_SERVICE_NAME "listener_hook_service"

struct listener_hook_info {
	bundle_context_pt context;
	char *filter;
	bool removed;
};

struct listener_hook_service {
	void *handle;
	celix_status_t (*added)(void *hook, array_list_pt listeners);
	celix_status_t (*removed)(void *hook, array_list_pt listeners);
};

#endif /* LISTENER_HOOK_SERVICE_H_ */
