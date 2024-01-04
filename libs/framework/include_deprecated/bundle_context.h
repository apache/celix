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

#ifndef BUNDLE_CONTEXT_H_
#define BUNDLE_CONTEXT_H_

/**
 * A bundle's execution context within the Framework. The context is used to
 * grant access to other methods so that this bundle can interact with the
 * Framework.
 */

#include "celix_types.h"

#include "celix_cleanup.h"
#include "service_factory.h"
#include "celix_service_factory.h"
#include "celix_service_listener.h"
#include "bundle_listener.h"
#include "framework_listener.h"
#include "celix_service_listener.h"

#include "dm_dependency_manager.h"
#include <stdlib.h>

#include "bundle_context.h"
#include "celix_bundle_context.h"
#include "celix_framework_export.h"

#ifdef __cplusplus
extern "C" {
#endif

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t
bundleContext_create(celix_framework_t *framework, celix_framework_logger_t* logger, celix_bundle_t *bundle, celix_bundle_context_t **bundle_context);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t bundleContext_destroy(celix_bundle_context_t *context);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t bundleContext_getBundle(celix_bundle_context_t *context, celix_bundle_t **bundle);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t bundleContext_getFramework(celix_bundle_context_t *context, celix_framework_t **framework);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t
bundleContext_installBundle(celix_bundle_context_t *context, const char *location, celix_bundle_t **bundle);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t
bundleContext_installBundle2(celix_bundle_context_t *context, const char *location, const char *inputFile, celix_bundle_t **bundle);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t
bundleContext_registerService(celix_bundle_context_t *context, const char *serviceName, const void *svcObj,
                              celix_properties_t *properties, service_registration_pt *service_registration);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t
bundleContext_registerServiceFactory(celix_bundle_context_t *context, const char *serviceName, service_factory_pt factory,
                                     celix_properties_t *properties, service_registration_pt *service_registration);

/**
 * Get a service reference for the bundle context. When the service reference is no longer needed use bundleContext_ungetServiceReference.
 * ServiceReference are coupled to a bundle context. Do not share service reference between bundles. Exchange the service.id instead.
 * 
 * @param context The bundle context
 * @param serviceName The name of the service (objectClass) to get
 * @param service_reference _output_ The found service reference, or NULL when no service is found.
 * @return CELIX_SUCCESS on success
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t bundleContext_getServiceReference(celix_bundle_context_t *context, const char *serviceName,
                                                                  service_reference_pt *service_reference);

/** Same as bundleContext_getServiceReference, but than for a optional serviceName combined with a optional filter.
 * The resulting array_list should be destroyed by the caller. For all service references return a unget should be called.
 * 
 * @param context the bundle context
 * @param serviceName the serviceName, can be NULL
 * @param filter the filter, can be NULL. If present will be combined (and) with the serviceName 
 * @param service_references _output_ a array list, can be size 0. 
 * @return CELIX_SUCCESS on success
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t
bundleContext_getServiceReferences(celix_bundle_context_t *context, const char *serviceName, const char *filter,
                                   celix_array_list_t **service_references);

/**
 * Retains (increases the ref count) the provided service reference. Can be used to retain a service reference.
 * Note that this is a deviation from the OSGi spec, due to the fact that C has no garbage collect.
 * 
 * @param context the bundle context
 * @param reference the service reference to retain
 * @return CELIX_SUCCES on success
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t
bundleContext_retainServiceReference(celix_bundle_context_t *context, service_reference_pt reference);

/**
 * Ungets the service references. If the ref counter of the service refernce goes to 0, the reference will be destroyed.
 * This is coupled with the bundleContext_getServiceReference(s) and bundleContext_retainServiceReferenc.
 * Note: That this is a deviation from the OSGi standard, due to the fact that C has no garbage collect.
 * 
 * @param context The bundle context.
 * @param reference the service reference to unget
 * @return CELIX_SUCCESS on success.
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t
bundleContext_ungetServiceReference(celix_bundle_context_t *context, service_reference_pt reference);

/**
 * @brief Service reference guard.
 */
typedef struct celix_service_ref_guard {
    service_reference_pt reference;
    celix_bundle_context_t* context;
} celix_service_ref_guard_t;

/**
 * @brief Initialize a scope guard for an existing reference.
 * @param [in] context Bundle owner of the service reference.
 * @param [in] reference An existing service reference.
 * @return An initialized service reference guard.
 */
static CELIX_UNUSED inline celix_service_ref_guard_t celix_ServiceRefGuard_init(celix_bundle_context_t* context,
                                                                                service_reference_pt reference) {
    return (celix_service_ref_guard_t){.reference = reference, .context = context};
}

/**
 * @brief Deinitialize a service reference guard.
 * Ungets the contained service reference if it is not NULL.
 * @param [in] ref A service reference guard.
 */
static CELIX_UNUSED inline void celix_ServiceRefGuard_deinit(celix_service_ref_guard_t* ref) {
    if (ref->reference != NULL) {
        bundleContext_ungetServiceReference(ref->context, ref->reference);
    }
}

CELIX_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(celix_service_ref_guard_t, celix_ServiceRefGuard_deinit);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t
bundleContext_getService(celix_bundle_context_t *context, service_reference_pt reference, void **service_instance);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t
bundleContext_ungetService(celix_bundle_context_t *context, service_reference_pt reference, bool *result);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t bundleContext_getBundles(celix_bundle_context_t *context, celix_array_list_t **bundles);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t bundleContext_getBundleById(celix_bundle_context_t *context, long id, celix_bundle_t **bundle);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t
bundleContext_addServiceListener(celix_bundle_context_t *context, celix_service_listener_t *listener, const char *filter) __attribute__((deprecated("using service listeners directly is deprecated, use a service tracker instead!")));

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t
bundleContext_removeServiceListener(celix_bundle_context_t *context, celix_service_listener_t *listener);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t bundleContext_addBundleListener(celix_bundle_context_t *context, bundle_listener_pt listener);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t
bundleContext_removeBundleListener(celix_bundle_context_t *context, bundle_listener_pt listener);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t
bundleContext_addFrameworkListener(celix_bundle_context_t *context, framework_listener_pt listener);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t
bundleContext_removeFrameworkListener(celix_bundle_context_t *context, framework_listener_pt listener);

/**
 * Gets the config property - or environment variable if the config property does not exist - for the provided name.
 *
 * @param context The bundle context.
 * @param name The name of the config property / environment variable to get.
 * @param value A ptr to the output value. This will be set when a value is found or else will be set to NULL.
 * @return 0 if successful.
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t
bundleContext_getProperty(celix_bundle_context_t *context, const char *name, const char **value);

/**
 * Gets the config property - or environment variable if the config property does not exist - for the provided name.
 *
 * @param context The bundle context.
 * @param name The name of the config property / environment variable to get.
 * @param defaultValue The default value to return if no value is found.
 * @param value A ptr to the output value. This will be set when a value is found or else will be set to NULL.
 * @return 0 if successful.
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t
bundleContext_getPropertyWithDefault(celix_bundle_context_t *context, const char *name, const char *defaultValue, const char **value);


#ifdef __cplusplus
}
#endif

#endif /* BUNDLE_CONTEXT_H_ */
