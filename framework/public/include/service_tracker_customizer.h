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
 *  \author     <a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */


#ifndef service_tracker_customizer_t_H_
#define service_tracker_customizer_t_H_

#include <apr_general.h>

#include "celix_errno.h"

typedef celix_status_t (*addingCallback)(void *handle, SERVICE_REFERENCE reference, void **service);
typedef celix_status_t (*addedCallback)(void * handle, SERVICE_REFERENCE reference, void * service);
typedef celix_status_t (*modifiedCallback)(void * handle, SERVICE_REFERENCE reference, void * service);
typedef celix_status_t (*removedCallback)(void * handle, SERVICE_REFERENCE reference, void * service);

typedef struct serviceTrackerCustomizer *service_tracker_customizer_t;

celix_status_t serviceTrackerCustomizer_create(apr_pool_t *pool, void *handle,
		addingCallback addingFunction, addedCallback addedFunction,
		modifiedCallback modifiedFunction, removedCallback removedFunction,
		service_tracker_customizer_t *customizer);

celix_status_t serviceTrackerCustomizer_getHandle(service_tracker_customizer_t customizer, void **handle);
celix_status_t serviceTrackerCustomizer_getAddingFunction(service_tracker_customizer_t customizer, addingCallback *function);
celix_status_t serviceTrackerCustomizer_getAddedFunction(service_tracker_customizer_t customizer, addedCallback *function);
celix_status_t serviceTrackerCustomizer_getModifiedFunction(service_tracker_customizer_t customizer, modifiedCallback *function);
celix_status_t serviceTrackerCustomizer_getRemovedFunction(service_tracker_customizer_t customizer, removedCallback *function);

#endif /* service_tracker_customizer_t_H_ */
