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
#include "pubsub_wire_v2_protocol_impl.h"

typedef struct ps_wp_activator {
    pubsub_protocol_wire_v2_t *wireprotocol;

    pubsub_protocol_service_t protocolSvc;
    long wireProtocolSvcId;
} ps_wp_activator_t;

static int ps_wp_start(ps_wp_activator_t *act, celix_bundle_context_t *ctx) {
    act->wireProtocolSvcId = -1L;

    celix_status_t status = pubsubProtocol_wire_v2_create(&(act->wireprotocol));
    if (status == CELIX_SUCCESS) {
        /* Set serializertype */
        celix_properties_t *props = celix_properties_create();
        celix_properties_set(props, PUBSUB_PROTOCOL_TYPE_KEY, PUBSUB_WIRE_V2_PROTOCOL_TYPE);
        celix_properties_setLong(props, OSGI_FRAMEWORK_SERVICE_RANKING, 10);

        act->protocolSvc.getHeaderSize = pubsubProtocol_wire_v2_getHeaderSize;
        act->protocolSvc.getHeaderBufferSize = pubsubProtocol_wire_v2_getHeaderBufferSize;
        act->protocolSvc.getSyncHeaderSize = pubsubProtocol_wire_v2_getSyncHeaderSize;
        act->protocolSvc.getSyncHeader = pubsubProtocol_wire_v2_getSyncHeader;
        act->protocolSvc.getFooterSize = pubsubProtocol_wire_v2_getFooterSize;
        act->protocolSvc.isMessageSegmentationSupported = pubsubProtocol_wire_v2_isMessageSegmentationSupported;

        act->protocolSvc.encodeHeader = pubsubProtocol_wire_v2_encodeHeader;
        act->protocolSvc.encodePayload = pubsubProtocol_wire_v2_encodePayload;
        act->protocolSvc.encodeMetadata = pubsubProtocol_wire_v2_encodeMetadata;
        act->protocolSvc.encodeFooter = pubsubProtocol_wire_v2_encodeFooter;

        act->protocolSvc.decodeHeader = pubsubProtocol_wire_v2_decodeHeader;
        act->protocolSvc.decodePayload = pubsubProtocol_wire_v2_decodePayload;
        act->protocolSvc.decodeMetadata = pubsubProtocol_wire_v2_decodeMetadata;
        act->protocolSvc.decodeFooter = pubsubProtocol_wire_v2_decodeFooter;

        act->wireProtocolSvcId = celix_bundleContext_registerService(ctx, &act->protocolSvc, PUBSUB_PROTOCOL_SERVICE_NAME, props);
    }
    return status;
}

static int ps_wp_stop(ps_wp_activator_t *act, celix_bundle_context_t *ctx) {
    celix_bundleContext_unregisterService(ctx, act->wireProtocolSvcId);
    act->wireProtocolSvcId = -1L;
    pubsubProtocol_wire_v2_destroy(act->wireprotocol);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(ps_wp_activator_t, ps_wp_start, ps_wp_stop)