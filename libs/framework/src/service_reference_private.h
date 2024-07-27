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
#include "celix_ref.h"
#include "celix_threads.h"

struct serviceReference {
    struct celix_ref refCount;
    registry_callback_t callback; // read-only
	bundle_pt referenceOwner; // read-only
	struct serviceRegistration * registration; // read-only, this registration is always accessible within the reference, though the service embedded might be unregistered by its provider
    bundle_pt registrationBundle; // read-only
    celix_thread_rwlock_t lock; // protect below
    const void* serviceCache;
    size_t usageCount;
};

celix_status_t serviceReference_create(registry_callback_t callback, bundle_pt referenceOwner, service_registration_pt registration, service_reference_pt *reference);

celix_status_t serviceReference_retain(service_reference_pt ref);
celix_status_t serviceReference_release(service_reference_pt ref, bool *destroyed);

celix_status_t serviceReference_invalidateCache(service_reference_pt reference);

celix_status_t serviceReference_getUsageCount(service_reference_pt reference, size_t *count);
celix_status_t serviceReference_getReferenceCount(service_reference_pt reference, size_t *count);

celix_status_t serviceReference_getService(service_reference_pt ref, const void **service);
celix_status_t serviceReference_ungetService(service_reference_pt ref, bool *result);

celix_status_t serviceReference_getOwner(service_reference_pt reference, bundle_pt *owner);



#endif /* SERVICE_REFERENCE_PRIVATE_H_ */
