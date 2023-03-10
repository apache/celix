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
#include <discovery_zeroconf_announcer.h>
#include <discovery_zeroconf_watcher.h>
#include <celix_log_helper.h>
#include <celix_bundle_activator.h>
#include <celix_types.h>
#include <celix_errno.h>

typedef struct discovery_zeroconf_activator {
    celix_log_helper_t *logHelper;
    discovery_zeroconf_announcer_t *announcer;
    discovery_zeroconf_watcher_t  *watcher;
}discovery_zeroconf_activator_t;

celix_status_t discoveryZeroconfActivator_start(discovery_zeroconf_activator_t *act, celix_bundle_context_t *ctx) {
    celix_status_t status = CELIX_SUCCESS;
    act->logHelper = celix_logHelper_create(ctx,"celix_rsa_zeroconf_discovery");
    if (act->logHelper == NULL) {
        status = CELIX_ENOMEM;
        goto log_helper_err;
    }
    status = discoveryZeroconfAnnouncer_create(ctx, act->logHelper, &act->announcer);
    if (status != CELIX_SUCCESS) {
        goto announcer_err;
    }
    status = discoveryZeroconfWatcher_create(ctx, act->logHelper, &act->watcher);
    if (status != CELIX_SUCCESS) {
        goto watcher_err;
    }
    return CELIX_SUCCESS;
watcher_err:
    discoveryZeroconfAnnouncer_destroy(act->announcer);
announcer_err:
    celix_logHelper_destroy(act->logHelper);
log_helper_err:
    return status;
}

celix_status_t discoveryZeroconfActivator_stop(discovery_zeroconf_activator_t *act, celix_bundle_context_t *ctx) {
    (void)ctx;//unused
    discoveryZeroconfWatcher_destroy(act->watcher);
    discoveryZeroconfAnnouncer_destroy(act->announcer);
    celix_logHelper_destroy(act->logHelper);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(discovery_zeroconf_activator_t, discoveryZeroconfActivator_start, discoveryZeroconfActivator_stop)