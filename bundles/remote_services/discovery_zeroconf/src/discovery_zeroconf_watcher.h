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
#ifndef CELIX_DISCOVERY_ZEROCONF_WATCHER_H
#define CELIX_DISCOVERY_ZEROCONF_WATCHER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <celix_log_helper.h>
#include <celix_types.h>
#include <celix_errno.h>

typedef struct discovery_zeroconf_watcher discovery_zeroconf_watcher_t;

celix_status_t discoveryZeroconfWatcher_create(celix_bundle_context_t *ctx, celix_log_helper_t *logHelper, discovery_zeroconf_watcher_t **watcherOut);

void discoveryZeroconfWatcher_destroy(discovery_zeroconf_watcher_t *watcher);

#ifdef __cplusplus
}
#endif

#endif //CELIX_DISCOVERY_ZEROCONF_WATCHER_H

