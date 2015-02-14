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

#include "service_reference.h"

struct serviceReference {
	bundle_pt bundle;
	struct serviceRegistration * registration;
};

celix_status_t serviceReference_create(bundle_pt bundle, service_registration_pt registration, service_reference_pt *reference);
celix_status_t serviceReference_destroy(service_reference_pt *reference);

celix_status_t serviceReference_invalidate(service_reference_pt reference);
celix_status_t serviceRefernce_isValid(service_reference_pt reference, bool *result);

celix_status_t serviceReference_getServiceRegistration(service_reference_pt reference, service_registration_pt *registration);

#endif /* SERVICE_REFERENCE_PRIVATE_H_ */
