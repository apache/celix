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
#include "celix_api.h"

#include "first_interceptor_private.h"
#include "second_interceptor_private.h"

#include <string.h>

struct interceptorActivator {
    first_interceptor_t *interceptor;
    uint64_t interceptorSvcId;

    second_interceptor_t *secondInterceptor;
    uint64_t secondInterceptorSvcId;
};

static int interceptor_start(struct interceptorActivator *act, celix_bundle_context_t *ctx) {
    pubsub_interceptor_t *interceptorSvc = calloc(1,sizeof(*interceptorSvc));
    first_interceptor_t *interceptor = NULL;
    firstInterceptor_create(&interceptor);

    interceptorSvc->handle = interceptor;
    interceptorSvc->preSend = firstInterceptor_preSend;
    interceptorSvc->postSend = firstInterceptor_postSend;
    interceptorSvc->preReceive = firstInterceptor_preReceive;
    interceptorSvc->postReceive = firstInterceptor_postReceive;

    act->interceptor = interceptor;

    celix_properties_t *props = celix_properties_create();
    celix_properties_setLong(props, OSGI_FRAMEWORK_SERVICE_RANKING, 10);

    celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
    opts.svc = interceptorSvc;
    opts.serviceName = PUBSUB_INTERCEPTOR_SERVICE_NAME;
    opts.serviceVersion = PUBSUB_INTERCEPTOR_SERVICE_VERSION;
    opts.properties = props;

    act->interceptorSvcId = celix_bundleContext_registerServiceWithOptions(ctx, &opts);

    pubsub_interceptor_t *secondInterceptorSvc = calloc(1, sizeof(*secondInterceptorSvc));
    second_interceptor_t *secondInterceptor = NULL;
    secondInterceptor_create(&secondInterceptor);

    secondInterceptorSvc->handle = secondInterceptor;
    secondInterceptorSvc->preSend = secondInterceptor_preSend;
    secondInterceptorSvc->postSend = secondInterceptor_postSend;
    secondInterceptorSvc->preReceive = secondInterceptor_preReceive;
    secondInterceptorSvc->postReceive = secondInterceptor_postReceive;

    act->secondInterceptor = secondInterceptor;

    celix_properties_t *secondProps = celix_properties_create();
    celix_properties_setLong(secondProps, OSGI_FRAMEWORK_SERVICE_RANKING, 20);

    celix_service_registration_options_t secondOpts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
    secondOpts.svc = secondInterceptorSvc;
    secondOpts.serviceName = PUBSUB_INTERCEPTOR_SERVICE_NAME;
    secondOpts.serviceVersion = PUBSUB_INTERCEPTOR_SERVICE_VERSION;
    secondOpts.properties = secondProps;

    act->secondInterceptorSvcId = celix_bundleContext_registerServiceWithOptions(ctx, &secondOpts);

    return 0;
}

static int interceptor_stop(struct interceptorActivator *act, celix_bundle_context_t *ctx) {
    celix_bundleContext_unregisterService(ctx, act->interceptorSvcId);
    firstInterceptor_destroy(act->interceptor);

    celix_bundleContext_unregisterService(ctx, act->secondInterceptorSvcId);
    secondInterceptor_destroy(act->secondInterceptor);

    return 0;
}

CELIX_GEN_BUNDLE_ACTIVATOR(struct interceptorActivator, interceptor_start, interceptor_stop)