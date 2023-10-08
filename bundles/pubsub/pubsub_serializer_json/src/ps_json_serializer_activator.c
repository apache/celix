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

#include <stdlib.h>
#include <pubsub_constants.h>

#include "pubsub_json_serialization_provider.h"
#include "celix_bundle_activator.h"

typedef struct psjs_activator {
    pubsub_serialization_provider_t *provider;
    long serializerSvcId;
} psjs_activator_t;

static int psjs_start(psjs_activator_t *act, celix_bundle_context_t *ctx) {
    act->provider = pubsub_jsonSerializationProvider_create(ctx);
    return act->provider ? CELIX_SUCCESS : CELIX_BUNDLE_EXCEPTION;
}

static int psjs_stop(psjs_activator_t *act, celix_bundle_context_t *ctx __attribute__((unused))) {
    pubsub_jsonSerializationProvider_destroy(act->provider);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(psjs_activator_t, psjs_start, psjs_stop)