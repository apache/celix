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

#ifndef PUBSUB_CONSTANTS_H_
#define PUBSUB_CONSTANTS_H_

#define PUBSUB_ADMIN_TYPE_KEY       "pubsub.config"
#define PUBSUB_SERIALIZER_TYPE_KEY  "pubsub.serializer"
#define PUBSUB_PROTOCOL_TYPE_KEY    "pubsub.protocol"

/**
 * Endpoints with the system visibility should be discoverable through the complete system
 */
#define PUBSUB_ENDPOINT_SYSTEM_VISIBILITY    "system"

/**
 * Endpoints with the system visibility are discoverable for a single host (i.e. IPC)
 */
#define PUBSUB_ENDPOINT_HOST_VISIBILITY      "host"

/**
 * Endpoints which are only visible within a single process
 */
#define PUBSUB_ENDPOINT_LOCAL_VISIBILITY     "local"

/**
 * Default scope, if not scope is specified endpoints are published using this scope
 */
#define PUBSUB_DEFAULT_ENDPOINT_SCOPE        "default"

#endif /* PUBSUB_CONSTANTS_H_ */
