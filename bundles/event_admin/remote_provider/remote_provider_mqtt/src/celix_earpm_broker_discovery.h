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

#ifndef CELIX_EARPM_BROKER_DISCOVERY_H
#define CELIX_EARPM_BROKER_DISCOVERY_H
#ifdef __cplusplus
extern "C" {
#endif
#include "celix_bundle_context.h"
#include "celix_properties.h"
#include "celix_errno.h"

#define CELIX_EARPM_MQTT_BROKER_INFO_SERVICE_NAME "celix_mqtt_broker_info"

//Properties of broker info service endpoint
#define CELIX_EARPM_MQTT_BROKER_SERVICE_CONFIG_TYPE "celix.earpm.mqtt"
#define CELIX_EARPM_MQTT_BROKER_SOCKET_DOMAIN "celix.earpm.mqtt.socket_domain"
#define CELIX_EARPM_MQTT_BROKER_ADDRESS "celix.earpm.mqtt.address" //IP address or hostname or unix socket path
#define CELIX_EARPM_MQTT_BROKER_PORT "celix.earpm.mqtt.port"

typedef struct celix_earpm_broker_discovery celix_earpm_broker_discovery_t;

celix_earpm_broker_discovery_t* celix_earpmDiscovery_create(celix_bundle_context_t* ctx);

void celix_earpmDiscovery_destroy(celix_earpm_broker_discovery_t* discovery);

celix_status_t celix_earpmDiscovery_start(celix_earpm_broker_discovery_t* discovery);

celix_status_t celix_earpmDiscovery_stop(celix_earpm_broker_discovery_t* discovery);

celix_status_t celix_earpmDiscovery_addEndpointListener(void* handle, void* service, const celix_properties_t* properties);

celix_status_t celix_earpmDiscovery_removeEndpointListener(void* handle, void* service, const celix_properties_t* properties);

#ifdef __cplusplus
}
#endif

#endif //CELIX_EARPM_BROKER_DISCOVERY_H
