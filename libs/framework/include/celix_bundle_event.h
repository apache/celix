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

#ifndef CELIX_BUNDLE_EVENT_H_
#define CELIX_BUNDLE_EVENT_H_

#include "celix_bundle.h"
#include "celix_framework_export.h"

#ifdef __cplusplus
extern "C" {
#endif

enum celix_bundle_event_type {
    CELIX_BUNDLE_EVENT_INSTALLED =          1,
    CELIX_BUNDLE_EVENT_STARTED =            2,
    CELIX_BUNDLE_EVENT_STOPPED =            3,
    CELIX_BUNDLE_EVENT_UPDATED =            4,
    CELIX_BUNDLE_EVENT_UNINSTALLED =        5,
    CELIX_BUNDLE_EVENT_RESOLVED =           6,
    CELIX_BUNDLE_EVENT_UNRESOLVED =         7,
    CELIX_BUNDLE_EVENT_STARTING =           8,
    CELIX_BUNDLE_EVENT_STOPPING =           9
};
typedef enum celix_bundle_event_type celix_bundle_event_type_e;

typedef struct celix_bundle_event {
    celix_bundle_t* bnd;
    char *bundleSymbolicName;
    celix_bundle_event_type_e type;
} celix_bundle_event_t;


#ifdef __cplusplus
}
#endif

#endif /* CELIX_BUNDLE_EVENT_H_ */
