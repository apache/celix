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
#include "celix_types.h"
#include "celix_error_injector.h"
#include "celix_properties.h"

extern "C" {
const char* __real_celix_bundleContext_getProperty(celix_bundle_context_t *__ctx, const char *__key, const char *__defaultVal);
CELIX_EI_DEFINE(celix_bundleContext_getProperty, const char*)
const char* __wrap_celix_bundleContext_getProperty(celix_bundle_context_t *__ctx, const char *__key, const char *__defaultVal) {
    CELIX_EI_IMPL(celix_bundleContext_getProperty);
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
    CELIX_EI_IMPL(celix_bundleContext_registerServiceWithOptionsAsync);
    celix_service_registration_options_t opts;
    opts = *__opts;
    opts.properties = celix_properties_copy(props);
    return __real_celix_bundleContext_registerServiceWithOptionsAsync(__ctx, &opts);
}

long __real_celix_bundleContext_trackServicesWithOptionsAsync(celix_bundle_context_t *__ctx, const celix_service_tracking_options_t *__opts);
CELIX_EI_DEFINE(celix_bundleContext_trackServicesWithOptionsAsync, long)
long __wrap_celix_bundleContext_trackServicesWithOptionsAsync(celix_bundle_context_t *__ctx, const celix_service_tracking_options_t *__opts) {
    CELIX_EI_IMPL(celix_bundleContext_trackServicesWithOptionsAsync);
    return __real_celix_bundleContext_trackServicesWithOptionsAsync(__ctx, __opts);
}

long __real_celix_bundleContext_getBundleId(const celix_bundle_context_t *__ctx);
CELIX_EI_DEFINE(celix_bundleContext_getBundleId, long)
long __wrap_celix_bundleContext_getBundleId(const celix_bundle_context_t *__ctx) {
    CELIX_EI_IMPL(celix_bundleContext_getBundleId);
    return __real_celix_bundleContext_getBundleId(__ctx);
}

celix_status_t __real_bundleContext_getServiceReferences(celix_bundle_context_t *__ctx, const char *__serviceName, const char *__filter, celix_array_list_t **service_references);
CELIX_EI_DEFINE(bundleContext_getServiceReferences, celix_status_t)
celix_status_t __wrap_bundleContext_getServiceReferences(celix_bundle_context_t *__ctx, const char *__serviceName, const char *__filter, celix_array_list_t **service_references) {
    CELIX_EI_IMPL(bundleContext_getServiceReferences);
    return __real_bundleContext_getServiceReferences(__ctx, __serviceName, __filter, service_references);
}

celix_status_t __real_bundleContext_retainServiceReference(celix_bundle_context_t *__ctx, service_reference_pt __ref);
CELIX_EI_DEFINE(bundleContext_retainServiceReference, celix_status_t)
celix_status_t __wrap_bundleContext_retainServiceReference(celix_bundle_context_t *__ctx, service_reference_pt __ref) {
    CELIX_EI_IMPL(bundleContext_retainServiceReference);
    return __real_bundleContext_retainServiceReference(__ctx, __ref);
}

long __real_celix_bundleContext_registerServiceAsync(celix_bundle_context_t *__ctx, const char *__serviceName, void *__service, celix_properties_t *__properties);
CELIX_EI_DEFINE(celix_bundleContext_registerServiceAsync, long)
long __wrap_celix_bundleContext_registerServiceAsync(celix_bundle_context_t *__ctx, const char *__serviceName, void *__service, celix_properties_t *__properties) {
    celix_properties_t __attribute__((cleanup(celix_properties_cleanup))) *props = celix_properties_copy(__properties);
    celix_properties_destroy(__properties);
    CELIX_EI_IMPL(celix_bundleContext_registerServiceAsync);
    __properties = celix_properties_copy(props);
    return __real_celix_bundleContext_registerServiceAsync(__ctx, __serviceName, __service, __properties);
}

long __real_celix_bundleContext_registerServiceFactoryAsync(celix_bundle_context_t *__ctx, const char *__serviceName, service_factory_pt __factory, celix_properties_t *__properties);
CELIX_EI_DEFINE(celix_bundleContext_registerServiceFactoryAsync, long)
long __wrap_celix_bundleContext_registerServiceFactoryAsync(celix_bundle_context_t *__ctx, const char *__serviceName, service_factory_pt __factory, celix_properties_t *__properties) {
    celix_properties_t __attribute__((cleanup(celix_properties_cleanup))) *props = celix_properties_copy(__properties);
    celix_properties_destroy(__properties);
    CELIX_EI_IMPL(celix_bundleContext_registerServiceFactoryAsync);
    __properties = celix_properties_copy(props);
    return __real_celix_bundleContext_registerServiceFactoryAsync(__ctx, __serviceName, __factory, __properties);
}

long __real_celix_bundleContext_scheduleEvent(celix_bundle_context_t *__ctx, const celix_scheduled_event_options_t* __options);
CELIX_EI_DEFINE(celix_bundleContext_scheduleEvent, long)
long __wrap_celix_bundleContext_scheduleEvent(celix_bundle_context_t *__ctx, const celix_scheduled_event_options_t* __options) {
    CELIX_EI_IMPL(celix_bundleContext_scheduleEvent);
    return __real_celix_bundleContext_scheduleEvent(__ctx, __options);
}

long __real_celix_bundleContext_trackBundlesWithOptionsAsync(celix_bundle_context_t *__ctx, const celix_bundle_tracking_options_t *__opts);
CELIX_EI_DEFINE(celix_bundleContext_trackBundlesWithOptionsAsync, long)
long __wrap_celix_bundleContext_trackBundlesWithOptionsAsync(celix_bundle_context_t *__ctx, const celix_bundle_tracking_options_t *__opts) {
    CELIX_EI_IMPL(celix_bundleContext_trackBundlesWithOptionsAsync);
    return __real_celix_bundleContext_trackBundlesWithOptionsAsync(__ctx, __opts);
}

}