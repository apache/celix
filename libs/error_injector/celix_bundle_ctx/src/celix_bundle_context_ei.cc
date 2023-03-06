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

#include "celix_bundle_context_ei.h"
#include "celix_bundle_context.h"
#include "celix_error_injector.h"
#include "celix_properties.h"

extern "C" {
const char* __real_celix_bundleContext_getProperty(celix_bundle_context_t *__ctx, const char *__key, const char *__defaultVal);
CELIX_EI_DEFINE(celix_bundleContext_getProperty, const char*)
const char* __wrap_celix_bundleContext_getProperty(celix_bundle_context_t *__ctx, const char *__key, const char *__defaultVal) {
    CELIX_EI_IMPL0(celix_bundleContext_getProperty);
    return __real_celix_bundleContext_getProperty(__ctx, __key, __defaultVal);
}

long __real_celix_bundleContext_registerServiceWithOptionsAsync(celix_bundle_context_t *__ctx, const celix_service_registration_options_t *__opts);
CELIX_EI_DEFINE(celix_bundleContext_registerServiceWithOptionsAsync, long)
static void celix_properties_cleanup(celix_properties_t **__prop) {
    celix_properties_destroy(*__prop);
}
long __wrap_celix_bundleContext_registerServiceWithOptionsAsync(celix_bundle_context_t *__ctx, const celix_service_registration_options_t *__opts) {
    // It should free '__opts->properties', whether failure or success.
    celix_properties_t __attribute__((cleanup(celix_properties_cleanup))) *props = celix_properties_copy(__opts->properties);
    celix_properties_destroy(__opts->properties);
    CELIX_EI_IMPL_NEGATIVE(celix_bundleContext_registerServiceWithOptionsAsync);
    celix_service_registration_options_t opts;
    opts = *__opts;
    opts.properties = celix_properties_copy(props);
    return __real_celix_bundleContext_registerServiceWithOptionsAsync(__ctx, &opts);
}

long __real_celix_bundleContext_trackServicesWithOptionsAsync(celix_bundle_context_t *__ctx, const celix_service_tracking_options_t *__opts);
CELIX_EI_DEFINE(celix_bundleContext_trackServicesWithOptionsAsync, long)
long __wrap_celix_bundleContext_trackServicesWithOptionsAsync(celix_bundle_context_t *__ctx, const celix_service_tracking_options_t *__opts) {
    CELIX_EI_IMPL_NEGATIVE(celix_bundleContext_trackServicesWithOptionsAsync);
    return __real_celix_bundleContext_trackServicesWithOptionsAsync(__ctx, __opts);
}
}