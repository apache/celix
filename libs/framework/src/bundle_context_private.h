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

#ifndef BUNDLE_CONTEXT_PRIVATE_H_
#define BUNDLE_CONTEXT_PRIVATE_H_

#include "bundle_context.h"
#include "celix_log.h"
#include "bundle_listener.h"
#include "celix_bundle_context.h"
#include "listener_hook_service.h"

typedef struct celix_bundle_context_bundle_tracker_entry {
	celix_bundle_context_t *ctx;
	long trackerId;
	bundle_listener_t listener;
	celix_bundle_tracking_options_t opts;
} celix_bundle_context_bundle_tracker_entry_t;

typedef struct celix_bundle_context_service_tracker_tracker_entry {
	long trackerId;

	struct listener_hook_service hook;
	service_registration_t *hookReg;

	char *serviceName;
	void *callbackHandle;
	void (*add)(void *handle, const celix_service_tracker_info_t *info);
	void (*remove)(void *handle, const celix_service_tracker_info_t *info);
} celix_bundle_context_service_tracker_tracker_entry_t;

struct celix_bundle_context {
	celix_framework_t *framework;
	celix_bundle_t *bundle;

	celix_thread_mutex_t mutex; //protects fields below
	array_list_t *svcRegistrations;
	celix_dependency_manager_t *mng;
	long nextTrackerId;
	hash_map_t *bundleTrackers; //key = trackerId, value = celix_bundle_context_bundle_tracker_entry_t*
	hash_map_t *serviceTrackers; //key = trackerId, value = celix_service_tracker_t*
	hash_map_t *serviceTrackerTrackers; //key = trackerId, value = celix_bundle_context_service_tracker_tracker_entry_t*
};


#endif /* BUNDLE_CONTEXT_PRIVATE_H_ */
