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
 * service_tracker_customizer.h
 *
 *  \date       Nov 15, 2012
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */


#ifndef service_tracker_customizer_t_H_
#define service_tracker_customizer_t_H_

#include <celix_errno.h>
#include <service_reference.h>

typedef celix_status_t (*adding_callback_pt)(void *handle, service_reference_pt reference, void **service);
typedef celix_status_t (*added_callback_pt)(void * handle, service_reference_pt reference, void * service);
typedef celix_status_t (*modified_callback_pt)(void * handle, service_reference_pt reference, void * service);
typedef celix_status_t (*removed_callback_pt)(void * handle, service_reference_pt reference, void * service);

typedef struct serviceTrackerCustomizer *service_tracker_customizer_pt;

FRAMEWORK_EXPORT celix_status_t serviceTrackerCustomizer_create(void *handle,
		adding_callback_pt addingFunction, added_callback_pt addedFunction,
		modified_callback_pt modifiedFunction, removed_callback_pt removedFunction,
		service_tracker_customizer_pt *customizer);
FRAMEWORK_EXPORT celix_status_t serviceTrackerCustomizer_destroy(service_tracker_customizer_pt customizer);

FRAMEWORK_EXPORT celix_status_t serviceTrackerCustomizer_getHandle(service_tracker_customizer_pt customizer, void **handle);
FRAMEWORK_EXPORT celix_status_t serviceTrackerCustomizer_getAddingFunction(service_tracker_customizer_pt customizer, adding_callback_pt *function);
FRAMEWORK_EXPORT celix_status_t serviceTrackerCustomizer_getAddedFunction(service_tracker_customizer_pt customizer, added_callback_pt *function);
FRAMEWORK_EXPORT celix_status_t serviceTrackerCustomizer_getModifiedFunction(service_tracker_customizer_pt customizer, modified_callback_pt *function);
FRAMEWORK_EXPORT celix_status_t serviceTrackerCustomizer_getRemovedFunction(service_tracker_customizer_pt customizer, removed_callback_pt *function);

#endif /* service_tracker_customizer_t_H_ */
