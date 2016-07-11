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
 * service_reference_private.h
 *
 *  \date       Feb 6, 2013
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */


#ifndef SERVICE_REFERENCE_PRIVATE_H_
#define SERVICE_REFERENCE_PRIVATE_H_

#include "registry_callback_private.h"
#include "service_reference.h"


struct serviceReference {
    registry_callback_t callback;
	bundle_pt referenceOwner;
	struct serviceRegistration * registration;
    bundle_pt registrationBundle;
    const void* service;

	size_t refCount;
    size_t usageCount;

    celix_thread_rwlock_t lock;
};

celix_status_t serviceReference_create(registry_callback_t callback, bundle_pt referenceOwner, service_registration_pt registration, service_reference_pt *reference);

celix_status_t serviceReference_retain(service_reference_pt ref);
celix_status_t serviceReference_release(service_reference_pt ref, bool *destroyed);

celix_status_t serviceReference_increaseUsage(service_reference_pt ref, size_t *updatedCount);
celix_status_t serviceReference_decreaseUsage(service_reference_pt ref, size_t *updatedCount);

celix_status_t serviceReference_invalidate(service_reference_pt reference);
celix_status_t serviceReference_isValid(service_reference_pt reference, bool *result);

celix_status_t serviceReference_getUsageCount(service_reference_pt reference, size_t *count);
celix_status_t serviceReference_getReferenceCount(service_reference_pt reference, size_t *count);

celix_status_t serviceReference_setService(service_reference_pt ref, const void *service);
celix_status_t serviceReference_getService(service_reference_pt reference, void **service);

celix_status_t serviceReference_getOwner(service_reference_pt reference, bundle_pt *owner);



#endif /* SERVICE_REFERENCE_PRIVATE_H_ */
