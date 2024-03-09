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
#ifndef CELIX_DISCOVERY_ZEROCONF_ANNOUNCER_H
#define CELIX_DISCOVERY_ZEROCONF_ANNOUNCER_H

#ifdef __cplusplus
extern "C" {
#endif
#include "endpoint_description.h"
#include "celix_cleanup.h"
#include "celix_log_helper.h"
#include "celix_types.h"
#include "celix_errno.h"

typedef struct discovery_zeroconf_announcer discovery_zeroconf_announcer_t;

celix_status_t discoveryZeroconfAnnouncer_create(celix_bundle_context_t *ctx, celix_log_helper_t *logHelper, discovery_zeroconf_announcer_t **announcerOut);

void discoveryZeroconfAnnouncer_destroy(discovery_zeroconf_announcer_t *announcer);

CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(discovery_zeroconf_announcer_t, discoveryZeroconfAnnouncer_destroy)

celix_status_t discoveryZeroconfAnnouncer_endpointAdded(void *handle, endpoint_description_t *endpoint, char *matchedFilter);

celix_status_t discoveryZeroconfAnnouncer_endpointRemoved(void *handle, endpoint_description_t *endpoint, char *matchedFilter);

#ifdef __cplusplus
}
#endif

#endif //CELIX_DISCOVERY_ZEROCONF_ANNOUNCER_H

