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

#include <stdlib.h>
#include <pubsub_constants.h>

#include "celix_api.h"
#include "pubsub_avrobin_serializer_impl.h"
#include "pubsub_avrobin_serialization_provider.h"

typedef struct psav_activator {
    pubsub_avrobin_serializer_t *serializer;
    pubsub_serializer_service_t serializerSvc;
    pubsub_serialization_provider_t* avrobinSerializationProvider;
    long serializerSvcId;
} psav_activator_t;

static int psav_start(psav_activator_t *act, celix_bundle_context_t *ctx) {
    act->serializerSvcId = -1L;

    celix_status_t status = pubsubAvrobinSerializer_create(ctx, &(act->serializer));
    act->avrobinSerializationProvider = pubsub_avrobinSerializationProvider_create(ctx);
    if (status == CELIX_SUCCESS) {
        act->serializerSvc.handle = act->serializer;

        act->serializerSvc.createSerializerMap = pubsubAvrobinSerializer_createSerializerMap;
        act->serializerSvc.destroySerializerMap = pubsubAvrobinSerializer_destroySerializerMap;

        /* Set serializer type */
        celix_properties_t *props = celix_properties_create();
        celix_properties_set(props, PUBSUB_SERIALIZER_TYPE_KEY, PUBSUB_AVROBIN_SERIALIZER_TYPE);

        act->serializerSvcId = celix_bundleContext_registerService(ctx, &act->serializerSvc, PUBSUB_SERIALIZER_SERVICE_NAME, props);
    }
    return status;
}

static int psav_stop(psav_activator_t *act, celix_bundle_context_t *ctx) {
    celix_bundleContext_unregisterService(ctx, act->serializerSvcId);
    act->serializerSvcId = -1L;
    pubsubAvrobinSerializer_destroy(act->serializer);
    pubsub_avrobinSerializationProvider_destroy(act->avrobinSerializationProvider);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(psav_activator_t, psav_start, psav_stop)
