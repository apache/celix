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

#include "celix_types.h"

#ifndef CELIX_SERVICE_EVENT_H_
#define CELIX_SERVICE_EVENT_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum celix_service_event_type {
	OSGI_FRAMEWORK_SERVICE_EVENT_REGISTERED = 0x00000001,
	OSGI_FRAMEWORK_SERVICE_EVENT_MODIFIED = 0x00000002,
	OSGI_FRAMEWORK_SERVICE_EVENT_UNREGISTERING = 0x00000004,
	OSGI_FRAMEWORK_SERVICE_EVENT_MODIFIED_ENDMATCH = 0x00000008,
} celix_service_event_type_t;

typedef struct celix_service_event {
	service_reference_pt reference;
	celix_service_event_type_t type;
} celix_service_event_t;

#ifdef __cplusplus
}
#endif

#endif /* CELIX_SERVICE_EVENT_H_ */
