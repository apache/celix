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

#ifndef PUBSUB_MESSAGE_SERIALIZATION_MARKER_H_
#define PUBSUB_MESSAGE_SERIALIZATION_MARKER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "hash_map.h"
#include "version.h"
#include "celix_bundle.h"
#include "sys/uio.h"

#define PUBSUB_MESSAGE_SERIALIZATION_MARKER_NAME      "pubsub_message_serialization_marker"
#define PUBSUB_MESSAGE_SERIALIZATION_MARKER_VERSION   "1.0.0"
#define PUBSUB_MESSAGE_SERIALIZATION_MARKER_RANGE     "[1,2)"

/**
 * @brief Service property (named "serialization.type") identifying the serialization type (e.g json, avrobin, etc)
 */
#define PUBSUB_MESSAGE_SERIALIZATION_MARKER_SERIALIZATION_TYPE_PROPERTY                 "serialization.type"

/**
 * @brief Service property (named "serialization.backwards.compatible") identifying whether the serialization is
 * backwards compatible (i.e. for json a extra - new - field can be safely ignored and is thus backwards compatible.
 *
 * Type if boolean. If service property is not present, false will be used.
 */
#define PUBSUB_MESSAGE_SERIALIZATION_MARKER_SERIALIZATION_BACKWARDS_COMPATIBLE       "serialization.backwards.compatible"

/**
 * @brief Marker interface - interface with no methods - to indicate that a serialization.type is available.
 *
 * This marker interface is used to indicate that a serialization type is available without the need to rely on
 * pubsub message serialization service per msg type.
 * The service.ranking of this marker interface must be used to select a serialization type if no serialization type is
 * configured. The service.ranking of individual pubsub_messge_serization_service is used to override serializers per
 * type.
 *
 * The properties serialization.type is mandatory
 */
typedef struct pubsub_message_serialization_marker {
    void* handle;
} pubsub_message_serialization_marker_t;

#ifdef __cplusplus
}
#endif
#endif /* PUBSUB_MESSAGE_SERIALIZATION_MARKER_H_ */
