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

#ifndef CELIX_MQTT_BROKER_INFO_SERVICE_H
#define CELIX_MQTT_BROKER_INFO_SERVICE_H
#ifdef __cplusplus
extern "C" {
#endif


#define CELIX_MQTT_BROKER_INFO_SERVICE_NAME "celix_mqtt_broker_info_service"

#define CELIX_MQTT_BROKER_INFO_SERVICE_VERSION "1.0.0"
#define CELIX_MQTT_BROKER_INFO_SERVICE_RANGE "[1.0.0,2)"

#define CELIX_MQTT_BROKER_ADDRESS "celix.mqtt.broker.address"
#define CELIX_MQTT_BROKER_PORT "celix.mqtt.broker.port"

typedef struct celix_mqtt_broker_info_service {
    void *handle;
} celix_mqtt_broker_info_service_t;

#ifdef __cplusplus
}
#endif

#endif //CELIX_MQTT_BROKER_INFO_SERVICE_H
