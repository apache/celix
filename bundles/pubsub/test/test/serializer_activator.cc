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

#include "celix_constants.h"
#include <unistd.h>
#include <pubsub_message_serialization_service.h>
#include <pubsub_constants.h>

#include "celix_api.h"
#include "msg.h"

struct activator {
    long serId;
    pubsub_message_serialization_service_t svc;
};

celix_status_t bnd_start(struct activator *act, celix_bundle_context_t *ctx) {

#pragma GCC diagnostic ignored "-Wunused-parameter"
    act->svc.serialize = [](void* handle, const void* input, struct iovec** output, size_t* outputIovLen) {
        if (*output == nullptr) {
            *output = static_cast<iovec *>(calloc(1, sizeof(struct iovec)));
            *outputIovLen = 1;
            auto *val = new uint32_t;
            *val = ((msg_t *) input)->seqNr;
            (*output)->iov_base = val;
            (*output)->iov_len = sizeof(uint32_t);
        }

        return CELIX_SUCCESS;
    };

    act->svc.deserialize = [](void* handle, const struct iovec* input, size_t inputIovLen, void** out) {
        auto *msg = new msg_t;
        msg->seqNr = *(uint32_t*)(uintptr_t)input->iov_base;
        *out = msg;
        return CELIX_SUCCESS;
    };

    act->svc.freeSerializedMsg = [](void* handle, struct iovec* input, size_t inputIovLen) {
        delete (uint32_t*)input->iov_base;
        free(input);
    };

    act->svc.freeDeserializedMsg = [](void* handle, void* msg) {
        delete (msg_t*)msg;
    };

    auto* p = celix_properties_create();
    celix_properties_set(p, PUBSUB_SERIALIZER_TYPE_KEY, "json");
    celix_properties_set(p, PUBSUB_MESSAGE_SERIALIZATION_SERVICE_SERIALIZATION_TYPE_PROPERTY, "msg_serializer");
    celix_properties_set(p, PUBSUB_MESSAGE_SERIALIZATION_SERVICE_MSG_ID_PROPERTY, "20");
    celix_properties_set(p, PUBSUB_MESSAGE_SERIALIZATION_SERVICE_MSG_FQN_PROPERTY, MSG_NAME);
    celix_properties_set(p, PUBSUB_MESSAGE_SERIALIZATION_SERVICE_MSG_VERSION_PROPERTY, "1.0.0");
    celix_properties_setLong(p, OSGI_FRAMEWORK_SERVICE_RANKING, 20);
    celix_service_registration_options_t opts;
    opts.svc = &act->svc;
    opts.properties = p;
    opts.serviceName = PUBSUB_MESSAGE_SERIALIZATION_SERVICE_NAME;
    opts.serviceVersion = PUBSUB_MESSAGE_SERIALIZATION_SERVICE_VERSION;
    act->serId = celix_bundleContext_registerServiceWithOptions(ctx, &opts);

    return CELIX_SUCCESS;
}

celix_status_t bnd_stop(struct activator *act, celix_bundle_context_t *ctx) {
    celix_bundleContext_unregisterService(ctx, act->serId);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(activator, bnd_start, bnd_stop)
