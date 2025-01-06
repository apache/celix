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

#ifndef CELIX_MOSQUITTO_EI_H
#define CELIX_MOSQUITTO_EI_H
#ifdef __cplusplus
extern "C" {
#endif
#include <mosquitto.h>
#include "celix_error_injector.h"

CELIX_EI_DECLARE(mosquitto_new, struct mosquitto*);
CELIX_EI_DECLARE(mosquitto_property_add_int32, int);
CELIX_EI_DECLARE(mosquitto_property_add_string_pair, int);
CELIX_EI_DECLARE(mosquitto_property_add_string, int);
CELIX_EI_DECLARE(mosquitto_property_add_binary, int);
CELIX_EI_DECLARE(mosquitto_int_option, int);
CELIX_EI_DECLARE(mosquitto_will_set_v5, int);
CELIX_EI_DECLARE(mosquitto_subscribe_v5, int);
CELIX_EI_DECLARE(mosquitto_unsubscribe, int);
CELIX_EI_DECLARE(mosquitto_publish_v5, int);
CELIX_EI_DECLARE(mosquitto_property_copy_all, int);
CELIX_EI_DECLARE(mosquitto_property_read_string, const mosquitto_property*);
CELIX_EI_DECLARE(mosquitto_property_read_binary, const mosquitto_property*);
CELIX_EI_DECLARE(mosquitto_property_read_string_pair, const mosquitto_property*);

#ifdef __cplusplus
}
#endif

#endif //CELIX_MOSQUITTO_EI_H
