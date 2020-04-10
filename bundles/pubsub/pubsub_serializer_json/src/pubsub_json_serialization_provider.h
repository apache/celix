/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */

#ifndef CELIX_PUBSUB_JSON_SERIALIZATION_PROVIDER_H
#define CELIX_PUBSUB_JSON_SERIALIZATION_PROVIDER_H

#include "celix_bundle_context.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pubsub_json_serialization_provider pubsub_json_serialization_provider_t; //opaque

/**
 * Creates JSON Serialization Provider.
 *
 * The provider monitors bundles and creates pubsub message serialization services for every unique descriptor found.
 *
 * Descriptors can be DFI descriptors or AVPR descriptors (FIXE #158).
 *
 * The provider will look for descriptors in META-INF/descriptors directory of every installed bundle.
 * If a framework config CELIX_FRAMEWORK_EXTENDER_PATH is set, this path will also be used to search for descriptor files.
 *
 * For every unique and valid descriptor found a pubsub_message_serialization_service will be registered for the 'json' serialization type.
 * The provider will also register a single pubsub_serialization_marker service which 'marks' the existing of the a json serialization support.
 *
 * Lastly a celix command shell will be register with the command name 'celix::json_message_serialization' which can be used to interactively
 * query the provided serialization services.
 *
 * @param ctx   The bundle context.
 * @return      A JSON Serialization Provider.
 */
pubsub_json_serialization_provider_t *pubsub_jsonSerializationProvider_create(celix_bundle_context_t *ctx);

/**
 * Destroys the provided JSON Serialization Provider.
 */
void pubsub_jsonSerializationProvider_destroy(pubsub_json_serialization_provider_t *provider);


/**
 * Returns the number of valid entries.
 */
size_t pubsub_jsonSerializationProvider_nrOfEntries(pubsub_json_serialization_provider_t *provider);

/**
 * Returns the number of invalid entries.
 */
size_t pubsub_jsonSerializationProvider_nrOfInvalidEntries(pubsub_json_serialization_provider_t *provider);

#ifdef __cplusplus
};
#endif

#endif //CELIX_PUBSUB_JSON_SERIALIZATION_PROVIDER_H
