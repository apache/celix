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

#ifndef CELIX_DM_EVENT_H_
#define CELIX_DM_EVENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "celix_types.h"
#include "celix_properties.h"

typedef enum celix_dm_event_type {
	CELIX_DM_EVENT_SVC_ADD,
	CELIX_DM_EVENT_SVC_REM,
    CELIX_DM_EVENT_SVC_SET
} celix_dm_event_type_e;

typedef struct celix_dm_event {
	celix_dm_service_dependency_t* dep;
    celix_dm_event_type_e eventType;
    void* svc;
    const celix_properties_t* props;
} celix_dm_event_t;

#ifdef __cplusplus
}
#endif

#endif /* CELIX_DM_EVENT_H_ */
