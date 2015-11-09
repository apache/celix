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
 * service_registration.h
 *
 *  \date       Aug 6, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef SERVICE_REGISTRATION_H_
#define SERVICE_REGISTRATION_H_

#include "celixbool.h"

typedef struct serviceRegistration * service_registration_pt;

#include "service_registry.h"
#include "array_list.h"
#include "bundle.h"
#include "framework_exports.h"


FRAMEWORK_EXPORT celix_status_t serviceRegistration_unregister(service_registration_pt registration);
FRAMEWORK_EXPORT celix_status_t serviceRegistration_getProperties(service_registration_pt registration, properties_pt *properties);
FRAMEWORK_EXPORT celix_status_t serviceRegistration_setProperties(service_registration_pt registration, properties_pt properties);
FRAMEWORK_EXPORT celix_status_t serviceRegistration_getServiceReferences(service_registration_pt registration, array_list_pt *references);

#endif /* SERVICE_REGISTRATION_H_ */
