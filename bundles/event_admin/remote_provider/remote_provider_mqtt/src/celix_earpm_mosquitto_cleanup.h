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

#ifndef CELIX_EARPM_MOSQUITTO_CLEANUP_H
#define CELIX_EARPM_MOSQUITTO_CLEANUP_H
#ifdef __cplusplus
extern "C" {
#endif

#include <mosquitto.h>

#include "celix_cleanup.h"

static inline void celix_earpm_mosquittoPropertyDestroy(mosquitto_property *prop) {
    if (prop != NULL) {
        mosquitto_property_free_all(&prop);
    }
}

CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(mosquitto_property, celix_earpm_mosquittoPropertyDestroy)

typedef struct mosquitto mosquitto;

CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(mosquitto, mosquitto_destroy)

#ifdef __cplusplus
}
#endif

#endif //CELIX_EARPM_MOSQUITTO_CLEANUP_H
