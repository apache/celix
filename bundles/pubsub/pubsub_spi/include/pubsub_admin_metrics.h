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

#ifndef PUBSUB_ADMIN_METRICS_H_
#define PUBSUB_ADMIN_METRICS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <uuid/uuid.h>
#include <sys/time.h>
#include "celix_array_list.h"

#define PUBSUB_ADMIN_METRICS_SERVICE_NAME   "pubsub_admin_metrics"

#define PUBSUB_AMDIN_METRICS_NAME_MAX       1024

typedef struct pubsub_admin_sender_msg_type_metrics {
    long bndId;
    char typeFqn[PUBSUB_AMDIN_METRICS_NAME_MAX];
    unsigned int typeId;
    unsigned long nrOfMessagesSend;
    unsigned long nrOfMessagesSendFailed;
    unsigned long nrOfSerializationErrors;
    struct timespec lastMessageSend;
    double averageTimeBetweenMessagesInSeconds;
    double averageSerializationTimeInSeconds;
} pubsub_admin_sender_msg_type_metrics_t;

typedef struct pubsub_admin_sender_metrics {
    char scope[PUBSUB_AMDIN_METRICS_NAME_MAX];
    char topic[PUBSUB_AMDIN_METRICS_NAME_MAX];
    unsigned long nrOfUnknownMessagesRetrieved;
    unsigned int nrOfmsgMetrics;
    pubsub_admin_sender_msg_type_metrics_t *msgMetrics; //size = nrOfMessageTypes
} pubsub_admin_sender_metrics_t;

typedef struct pubsub_admin_receiver_metrics {
    char scope[PUBSUB_AMDIN_METRICS_NAME_MAX];
    char topic[PUBSUB_AMDIN_METRICS_NAME_MAX];
    unsigned long nrOfMsgTypes;
    struct {
        unsigned int typeId;
        char typeFqn[PUBSUB_AMDIN_METRICS_NAME_MAX];
        int nrOfOrigins;
        struct {
            uuid_t originUUID;
            unsigned long nrOfMessagesReceived;
            unsigned long nrOfSerializationErrors;
            unsigned long nrOfMissingSeqNumbers;
            struct timespec lastMessageReceived;
            double averageTimeBetweenMessagesInSeconds;
            double averageSerializationTimeInSeconds;
            double averageDelayInSeconds;
            double minDelayInSeconds;
            double maxDelayInSeconds;
        } *origins;
    } *msgTypes;
} pubsub_admin_receiver_metrics_t;


typedef struct pubsub_admin_metrics {
    char psaType[PUBSUB_AMDIN_METRICS_NAME_MAX];

    celix_array_list_t *senders; //entry type = pubsub_admin_sender_metrics_t
    celix_array_list_t *receivers;//entry type = pubsub_admin_receiver_metrics_t

} pubsub_admin_metrics_t;

/**
 * A metrics service for a PubSubAdmin. This is an optional service.
 *
 * Expected service properties: PUBSUB_ADMIN_SERVICE_TYPE
 */
struct pubsub_admin_metrics_service {
    void *handle;

    /**
     * Creates a metrics struct for the PSA. The caller is owner of the data and
     * should use pubsub_freePubSubAdminMetrics (part of pubsub_spi) to release the data.
     * @param handle
     * @return The metrics or NULL if no metrics can be created.
     */
    pubsub_admin_metrics_t* (*metrics)(void *handle);
};

void pubsub_freePubSubAdminMetrics(pubsub_admin_metrics_t *metrics);

typedef struct pubsub_admin_metrics_service pubsub_admin_metrics_service_t;

#ifdef __cplusplus
}
#endif
#endif /* PUBSUB_ADMIN_METRICS_H_ */


