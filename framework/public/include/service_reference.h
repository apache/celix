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
 * service_reference.h
 *
 *  \date       Jul 20, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef SERVICE_REFERENCE_H_
#define SERVICE_REFERENCE_H_

typedef struct serviceReference * service_reference_pt;

#include "celixbool.h"
#include "array_list.h"
#include "service_registration.h"
#include "bundle.h"
#include "framework_exports.h"

FRAMEWORK_EXPORT celix_status_t serviceReference_getBundle(service_reference_pt reference, bundle_pt *bundle);

FRAMEWORK_EXPORT bool serviceReference_isAssignableTo(service_reference_pt reference, bundle_pt requester, char * serviceName);

FRAMEWORK_EXPORT celix_status_t serviceReference_getUsingBundles(service_reference_pt reference, array_list_pt *bundles);

FRAMEWORK_EXPORT celix_status_t serviceReference_getProperty(service_reference_pt reference, char *key, char **value);
FRAMEWORK_EXPORT celix_status_t serviceReference_getPropertyKeys(service_reference_pt reference, char **keys[], unsigned int *size);

FRAMEWORK_EXPORT celix_status_t serviceReference_equals(service_reference_pt reference, service_reference_pt compareTo, bool *equal);
FRAMEWORK_EXPORT unsigned int serviceReference_hashCode(void *referenceP);
FRAMEWORK_EXPORT int serviceReference_equals2(void *reference1, void *reference2);
FRAMEWORK_EXPORT celix_status_t serviceReference_compareTo(service_reference_pt reference, service_reference_pt compareTo, int *compare);

#endif /* SERVICE_REFERENCE_H_ */
