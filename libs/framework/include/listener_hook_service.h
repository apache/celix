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

#ifndef LISTENER_HOOK_SERVICE_H_
#define LISTENER_HOOK_SERVICE_H_

#include "bundle_context.h"

#define OSGI_FRAMEWORK_LISTENER_HOOK_SERVICE_NAME "listener_hook_service"
#ifdef __cplusplus
extern "C" {
#endif


typedef struct listener_hook_info *listener_hook_info_pt;
typedef struct listener_hook_info celix_listener_hook_info_t;
typedef struct listener_hook_service *listener_hook_service_pt;
typedef struct listener_hook_service celix_listener_hook_service_t;

struct listener_hook_info {
	celix_bundle_context_t *context;
	const char *filter;
	bool removed;
};

struct listener_hook_service {
	void *handle;

	/**
	 * Called when a new service listener / service tracker is added.
	 * @param handle The service handle.
	 * @param listeners A list of celix_listener_hook_info_t* entries.
	 */
	celix_status_t (*added)(void *handle, celix_array_list_t *listeners);

    /**
     * Called when a service listener / service tracker is removed.
     * @param handle The service handle.
     * @param listeners A list of celix_listener_hook_info_t* entries.
     */
	celix_status_t (*removed)(void *handle, celix_array_list_t *listeners);
};

#ifdef __cplusplus
}
#endif

#endif /* LISTENER_HOOK_SERVICE_H_ */
