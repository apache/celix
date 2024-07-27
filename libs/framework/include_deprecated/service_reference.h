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

#ifndef SERVICE_REFERENCE_H_
#define SERVICE_REFERENCE_H_

#include <stdbool.h>

#include "celix_types.h"
#include "service_registration.h"
#include "celix_bundle.h"
#include "celix_framework_export.h"

#ifdef __cplusplus
extern "C" {
#endif

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t serviceReference_getBundle(service_reference_pt reference, celix_bundle_t **bundle);

CELIX_FRAMEWORK_DEPRECATED_EXPORT bool
serviceReference_isAssignableTo(service_reference_pt reference, celix_bundle_t *requester, const char *serviceName);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t
serviceReference_getProperty(service_reference_pt reference, const char *key, const char **value);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t
serviceReference_getPropertyWithDefault(service_reference_pt reference, const char *key, const char* def, const char **value);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t
serviceReference_getPropertyKeys(service_reference_pt reference, char **keys[], unsigned int *size);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t
serviceReference_getServiceRegistration(service_reference_pt reference, service_registration_pt *registration);

CELIX_FRAMEWORK_DEPRECATED_EXPORT
long serviceReference_getServiceId(service_reference_pt reference);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t
serviceReference_equals(service_reference_pt reference, service_reference_pt compareTo, bool *equal);

CELIX_FRAMEWORK_DEPRECATED_EXPORT unsigned int serviceReference_hashCode(const void *referenceP);

CELIX_FRAMEWORK_DEPRECATED_EXPORT int serviceReference_equals2(const void *reference1, const void *reference2);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t
serviceReference_compareTo(service_reference_pt reference, service_reference_pt compareTo, int *compare);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t serviceReference_getUsingBundles(service_reference_pt ref, celix_array_list_t **out);

#ifdef __cplusplus
}
#endif

#endif /* SERVICE_REFERENCE_H_ */
