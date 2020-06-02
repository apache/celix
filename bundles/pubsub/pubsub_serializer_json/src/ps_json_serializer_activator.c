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
#include "celix_api.h"
#include "pubsub_serializer_impl.h"

typedef struct psjs_activator {
    pubsub_json_serializer_t* serializer;
    pubsub_serialization_provider_t *provider;

    pubsub_serializer_service_t serializerSvc;
    long serializerSvcId;
} psjs_activator_t;

static int psjs_start(psjs_activator_t *act, celix_bundle_context_t *ctx) {
    act->serializerSvcId = -1L;

    celix_status_t status = pubsubSerializer_create(ctx, &(act->serializer));
    act->provider = pubsub_jsonSerializationProvider_create(ctx);
    if (status == CELIX_SUCCESS) {
        act->serializerSvc.handle = act->serializer;
        act->serializerSvc.createSerializerMap = pubsubSerializer_createSerializerMap;
        act->serializerSvc.destroySerializerMap = pubsubSerializer_destroySerializerMap;
        /* Set serializer type */
        celix_properties_t *props = celix_properties_create();
        celix_properties_set(props, PUBSUB_SERIALIZER_TYPE_KEY, PUBSUB_JSON_SERIALIZER_TYPE);
        act->serializerSvcId = celix_bundleContext_registerService(ctx, &act->serializerSvc, PUBSUB_SERIALIZER_SERVICE_NAME, props);

    }
    return status;
}

static int psjs_stop(psjs_activator_t *act, celix_bundle_context_t *ctx) {
    celix_bundleContext_unregisterService(ctx, act->serializerSvcId);
    pubsubSerializer_destroy(act->serializer);
    pubsub_jsonSerializationProvider_destroy(act->provider);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(psjs_activator_t, psjs_start, psjs_stop)