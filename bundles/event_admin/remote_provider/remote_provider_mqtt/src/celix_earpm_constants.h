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

#ifndef CELIX_EARPM_CONSTANTS_H
#define CELIX_EARPM_CONSTANTS_H
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Topic for the EventAdminMqtt
 * @{
 */
#define CELIX_EARPM_TOPIC_PREFIX "celix/EventAdminMqtt/"
#define CELIX_EARPM_HANDLER_INFO_TOPIC_PREFIX CELIX_EARPM_TOPIC_PREFIX"HandlerInfo/"
#define CELIX_EARPM_HANDLER_INFO_QUERY_TOPIC CELIX_EARPM_HANDLER_INFO_TOPIC_PREFIX"query"
#define CELIX_EARPM_HANDLER_INFO_UPDATE_TOPIC  CELIX_EARPM_HANDLER_INFO_TOPIC_PREFIX"update"
#define CELIX_EARPM_HANDLER_INFO_ADD_TOPIC  CELIX_EARPM_HANDLER_INFO_TOPIC_PREFIX"add"
#define CELIX_EARPM_HANDLER_INFO_REMOVE_TOPIC  CELIX_EARPM_HANDLER_INFO_TOPIC_PREFIX"remove"
#define CELIX_EARPM_SYNC_EVENT_ACK_TOPIC_PREFIX CELIX_EARPM_TOPIC_PREFIX"SyncEvent/ack/" // topic = topic prefix + requester framework UUID
#define CELIX_EARPM_SESSION_END_TOPIC CELIX_EARPM_TOPIC_PREFIX"session/end"

/** @}*///end of Topic for the EventAdminMqtt

/**
 * Configuration properties for the EventAdminMqtt
 * @{
 */

/**
 * @brief The profile of the MQTT broker, which is used to configure the MQTT broker. Default value is /etc/mosquitto.conf.
 */
#define CELIX_EARPM_BROKER_PROFILE "CELIX_EARPM_BROKER_PROFILE"
#define CELIX_EARPM_BROKER_PROFILE_DEFAULT "/etc/mosquitto.conf"

/**
 * @brief The default QoS for events. If the event does not specify a QoS with the CELIX_EVENT_REMOTE_QOS property, this QoS is used. Default value is CELIX_EARPM_QOS_AT_MOST_ONCE.
 * @relatedalso CELIX_EVENT_REMOTE_QOS
 */
#define CELIX_EARPM_EVENT_DEFAULT_QOS "CELIX_EARPM_EVENT_DEFAULT_QOS"
#define CELIX_EARPM_EVENT_DEFAULT_QOS_DEFAULT CELIX_EARPM_QOS_AT_MOST_ONCE

/**
 * @brief The capacity of the message queue. Default value is 256, and the maximum value is 1024*1024.
 * The message queue is used to cache the messages when processing the messages.
 */
#define CELIX_EARPM_MSG_QUEUE_CAPACITY "CELIX_EARPM_MSG_QUEUE_CAPACITY"
#define CELIX_EARPM_MSG_QUEUE_CAPACITY_DEFAULT 256
#define CELIX_EARPM_MSG_QUEUE_MAX_SIZE 2048

/**
 * @brief The capacity of processing messages in parallel. Default value is 20. The maximum value is CELIX_EARPM_MSG_QUEUE_CAPACITY.
 * It is used to limit the number of messages that are processed in parallel.
 */
#define CELIX_EARPM_PARALLEL_MSG_CAPACITY "CELIX_EARPM_PARALLEL_MSG_CAPACITY"
#define CELIX_EARPM_PARALLEL_MSG_CAPACITY_DEFAULT 20

/**
 * @brief The thread pool size for delivering the sync events. Default value is 5. The maximum value is 20.
 * It is used to deliver the remote sync events to the local event handlers.
 */
#define CELIX_EARPM_SYNC_EVENT_DELIVERY_THREADS "CELIX_EARPM_SYNC_EVENT_DELIVERY_THREADS"
#define CELIX_EARPM_SYNC_EVENT_DELIVERY_THREADS_DEFAULT 5

/**
 * @brief If remote celix framework instance does not acknowledge a sync event in specified time, and the exception count is larger than this value,
 * the event remote provider will not wait for the acknowledgment anymore, until receiving a new message from the remote celix framework instance. Default value is 10.
 */
#define CELIX_EARPM_SYNC_EVENT_CONTINUOUS_NO_ACK_THRESHOLD "CELIX_EARPM_SYNC_EVENT_CONTINUOUS_NO_ACK_THRESHOLD"
#define CELIX_EARPM_SYNC_EVENT_CONTINUOUS_NO_ACK_THRESHOLD_DEFAULT 10

/** @}*///end of Configuration properties for the EventAdminMqtt

/**
 * @brief The QoS MQTT
 */
typedef enum celix_earpm_qos {
    CELIX_EARPM_QOS_UNKNOWN = -1,
    CELIX_EARPM_QOS_AT_MOST_ONCE = 0,
    CELIX_EARPM_QOS_AT_LEAST_ONCE = 1,
    CELIX_EARPM_QOS_EXACTLY_ONCE = 2,
    CELIX_EARPM_QOS_MAX,
}celix_earpm_qos_e;

#ifdef __cplusplus
}
#endif

#endif //CELIX_EARPM_CONSTANTS_H
