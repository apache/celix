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

#include "celix_api.h"
#include "pubsub_wire_protocol_impl.h"

typedef struct ps_wp_activator {
    pubsub_protocol_wire_v1_t *wireprotocol;

    pubsub_protocol_service_t protocolSvc;
    long wireProtocolSvcId;
} ps_wp_activator_t;

static int ps_wp_start(ps_wp_activator_t *act, celix_bundle_context_t *ctx) {
    act->wireProtocolSvcId = -1L;

    celix_status_t status = pubsubProtocol_create(&(act->wireprotocol));
    if (status == CELIX_SUCCESS) {
        /* Set serializertype */
        celix_properties_t *props = celix_properties_create();
        celix_properties_set(props, PUBSUB_PROTOCOL_TYPE_KEY, PUBSUB_WIRE_PROTOCOL_TYPE);

        act->protocolSvc.getSyncHeader = pubsubProtocol_getSyncHeader;
        
        act->protocolSvc.encodeHeader = pubsubProtocol_encodeHeader;
        act->protocolSvc.encodePayload = pubsubProtocol_encodePayload;
        act->protocolSvc.encodeMetadata = pubsubProtocol_encodeMetadata;

        act->protocolSvc.decodeHeader = pubsubProtocol_decodeHeader;
        act->protocolSvc.decodePayload = pubsubProtocol_decodePayload;
        act->protocolSvc.decodeMetadata = pubsubProtocol_decodeMetadata;

        act->wireProtocolSvcId = celix_bundleContext_registerService(ctx, &act->protocolSvc, PUBSUB_PROTOCOL_SERVICE_NAME, props);
    }
    return status;
}

static int ps_wp_stop(ps_wp_activator_t *act, celix_bundle_context_t *ctx) {
    celix_bundleContext_unregisterService(ctx, act->wireProtocolSvcId);
    act->wireProtocolSvcId = -1L;
    pubsubProtocol_destroy(act->wireprotocol);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(ps_wp_activator_t, ps_wp_start, ps_wp_stop)