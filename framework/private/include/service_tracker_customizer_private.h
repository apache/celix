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
 * service_tracker_customizer_private.h
 *
 *  \date       Feb 7, 2013
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */


#ifndef SERVICE_TRACKER_CUSTOMIZER_PRIVATE_H_
#define SERVICE_TRACKER_CUSTOMIZER_PRIVATE_H_

#include "service_reference.h"

#include "service_tracker_customizer.h"


struct serviceTrackerCustomizer {
	void * handle;
	celix_status_t (*addingService)(void * handle, service_reference_pt reference, void **service);
	celix_status_t (*addedService)(void * handle, service_reference_pt reference, void * service);
	celix_status_t (*modifiedService)(void * handle, service_reference_pt reference, void * service);

	/*TODO rename to removingService. because it is invoke during remove not after!*/
	celix_status_t (*removedService)(void * handle, service_reference_pt reference, void * service);

	/*TODO add removed function ? invoked after the remove ?? */
};


#endif /* SERVICE_TRACKER_CUSTOMIZER_PRIVATE_H_ */
