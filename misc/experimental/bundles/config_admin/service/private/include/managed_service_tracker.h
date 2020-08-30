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
 * managed_service_tracker.h
 *
 *  \date       Aug 12, 2013
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */


#ifndef MANAGED_service_tracker_pt_H_
#define MANAGED_service_tracker_pt_H_

/* celix.framework */
#include "bundle_context.h"
#include "celix_errno.h"
#include "service_reference.h"
#include "service_tracker.h"
/* celix.config_admin.public */
#include "configuration.h"
/* celix.config_admin.private */
#include "configuration_admin_factory.h"
#include "configuration_store.h"


typedef struct managed_service_tracker *managed_service_tracker_pt;


celix_status_t managedServiceTracker_create(bundle_context_pt context,
											configuration_admin_factory_pt factory, configuration_store_pt store,
											managed_service_tracker_pt *trackerHandle, service_tracker_pt *tracker);
celix_status_t managedServiceTracker_destroy(bundle_context_pt context, managed_service_tracker_pt handle, service_tracker_pt tracker);

celix_status_t managedServiceTracker_notifyDeleted(managed_service_tracker_pt tracker, configuration_pt configuration);
celix_status_t managedServiceTracker_notifyUpdated(managed_service_tracker_pt tracker, configuration_pt configuration);

celix_status_t managedServiceTracker_addingService(void * handle, service_reference_pt reference, void **service);
celix_status_t managedServiceTracker_addedService(void * handle, service_reference_pt reference, void * service);
celix_status_t managedServiceTracker_modifiedService(void * handle, service_reference_pt reference, void * service);
celix_status_t managedServiceTracker_removedService(void * handle, service_reference_pt reference, void * service);


#endif /* MANAGED_service_tracker_pt_H_ */
