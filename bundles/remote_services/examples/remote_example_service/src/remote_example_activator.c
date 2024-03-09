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

#include "celix_bundle_activator.h"
#include "remote_example.h"
#include "remote_constants.h"
#include "remote_example_impl.h"

struct activator {
    remote_example_impl_t *impl;
    remote_example_t service;
    long svcId;
};

celix_status_t remoteExampleBndStart(struct activator *act, celix_bundle_context_t *ctx) {
    act->svcId = -1L;
    act->impl = remoteExample_create(ctx);
    if (act->impl != NULL) {
        act->service.handle = act->impl;
        act->service.pow = (void*)remoteExample_pow;
        act->service.fib = (void*)remoteExample_fib;
        act->service.setName1 = (void*)remoteExample_setName1;
        act->service.setName2 = (void*)remoteExample_setName2;
        act->service.setEnum = (void*)remoteExample_setEnum;
        act->service.action = (void*)remoteExample_action;
        act->service.setComplex = (void*)remoteExample_setComplex;
        act->service.createAdditionalRemoteService = (void*)remoteExample_createAdditionalRemoteService;

        celix_properties_t *properties = celix_properties_create();
        celix_properties_set(properties, CELIX_RSA_SERVICE_EXPORTED_INTERFACES, REMOTE_EXAMPLE_NAME);
        act->svcId = celix_bundleContext_registerService(ctx, &act->service, REMOTE_EXAMPLE_NAME, properties);
    }
    return CELIX_SUCCESS;
}

celix_status_t remoteExampleBndStop(struct activator *act, celix_bundle_context_t *ctx) {
    celix_bundleContext_unregisterService(ctx, act->svcId);
    if (act->impl != NULL) {
        remoteExample_destroy(act->impl);
    }
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(struct activator, remoteExampleBndStart, remoteExampleBndStop);
