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


#include "first_interceptor_private.h"
#include "second_interceptor_private.h"
#include "celix_bundle_activator.h"
#include "celix_constants.h"

#include <string.h>
#include <stdlib.h>

struct interceptorActivator {
    first_interceptor_t *interceptor;
    uint64_t interceptorSvcId;
    remote_interceptor_t *interceptorSvc;

    second_interceptor_t *secondInterceptor;
    uint64_t secondInterceptorSvcId;
    remote_interceptor_t *secondInterceptorSvc;
};

static int interceptor_start(struct interceptorActivator *act, celix_bundle_context_t *ctx) {
    remote_interceptor_t *interceptorSvc = calloc(1,sizeof(*interceptorSvc));
    first_interceptor_t *interceptor = NULL;
    firstInterceptor_create(&interceptor);

    interceptorSvc->handle = interceptor;
    interceptorSvc->preProxyCall = firstInterceptor_preProxyCall;
    interceptorSvc->postProxyCall = firstInterceptor_postProxyCall;
    interceptorSvc->preExportCall = firstInterceptor_preExportCall;
    interceptorSvc->postExportCall = firstInterceptor_postExportCall;
    act->interceptorSvc = interceptorSvc;

    act->interceptor = interceptor;

    celix_properties_t *props = celix_properties_create();
    celix_properties_setLong(props, CELIX_FRAMEWORK_SERVICE_RANKING, 10);

    celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
    opts.svc = interceptorSvc;
    opts.serviceName = CELIX_RSA_REMOTE_INTERCEPTOR_SERVICE_NAME;
    opts.serviceVersion = CELIX_RSA_REMOTE_INTERCEPTOR_SERVICE_VERSION;
    opts.properties = props;

    act->interceptorSvcId = celix_bundleContext_registerServiceWithOptions(ctx, &opts);

    remote_interceptor_t *secondInterceptorSvc = calloc(1, sizeof(*secondInterceptorSvc));
    second_interceptor_t *secondInterceptor = NULL;
    secondInterceptor_create(&secondInterceptor);

    secondInterceptorSvc->handle = secondInterceptor;
    secondInterceptorSvc->preProxyCall = secondInterceptor_preProxyCall;
    secondInterceptorSvc->postProxyCall = secondInterceptor_postProxyCall;
    secondInterceptorSvc->preExportCall = secondInterceptor_preExportCall;
    secondInterceptorSvc->postExportCall = secondInterceptor_postExportCall;
    act->secondInterceptorSvc = secondInterceptorSvc;

    act->secondInterceptor = secondInterceptor;

    celix_properties_t *secondProps = celix_properties_create();
    celix_properties_setLong(secondProps, CELIX_FRAMEWORK_SERVICE_RANKING, 20);

    celix_service_registration_options_t secondOpts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
    secondOpts.svc = secondInterceptorSvc;
    secondOpts.serviceName = CELIX_RSA_REMOTE_INTERCEPTOR_SERVICE_NAME;
    secondOpts.serviceVersion = CELIX_RSA_REMOTE_INTERCEPTOR_SERVICE_VERSION;
    secondOpts.properties = secondProps;

    act->secondInterceptorSvcId = celix_bundleContext_registerServiceWithOptions(ctx, &secondOpts);

    return 0;
}

static int interceptor_stop(struct interceptorActivator *act, celix_bundle_context_t *ctx) {
    celix_bundleContext_unregisterService(ctx, act->interceptorSvcId);
    firstInterceptor_destroy(act->interceptor);
    free(act->interceptorSvc);

    celix_bundleContext_unregisterService(ctx, act->secondInterceptorSvcId);
    secondInterceptor_destroy(act->secondInterceptor);
    free(act->secondInterceptorSvc);

    return 0;
}

CELIX_GEN_BUNDLE_ACTIVATOR(struct interceptorActivator, interceptor_start, interceptor_stop)