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

#include "pubsub_serialization_provider.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
* Creates a JSON Serialization Provider.
*/
pubsub_serialization_provider_t* pubsub_jsonSerializationProvider_create(celix_bundle_context_t *ctx);

/**
 * Destroys the provided JSON Serialization Provider.
 */
void pubsub_jsonSerializationProvider_destroy(pubsub_serialization_provider_t *provider);

#ifdef __cplusplus
};
#endif

#endif //CELIX_PUBSUB_JSON_SERIALIZATION_PROVIDER_H
