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

#ifndef BUNDLE_H_
#define BUNDLE_H_

#include "celix_types.h"

#include "celix_errno.h"
#include "celix_bundle_state.h"
#include "bundle_archive.h"
#include "framework.h"
#include "module.h"
#include "service_reference.h"
#include "celix_log.h"
#include "celix_threads.h"
#include "bundle_context.h"
#include "celix_framework_export.h"

#ifdef __cplusplus
extern "C" {
#endif

struct celix_bundle_activator;
typedef struct celix_bundle_activator celix_bundle_activator_t;

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t bundle_isSystemBundle(const celix_bundle_t *bundle, bool *systemBundle);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t bundle_getArchive(const celix_bundle_t *bundle, bundle_archive_pt *archive);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t bundle_getCurrentModule(const celix_bundle_t *bundle, module_pt *module);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_array_list_t *bundle_getModules(const celix_bundle_t *bundle);

CELIX_FRAMEWORK_DEPRECATED_EXPORT void *bundle_getHandle(celix_bundle_t *bundle);

CELIX_FRAMEWORK_DEPRECATED_EXPORT void bundle_setHandle(celix_bundle_t *bundle, void *handle);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_bundle_activator_t *bundle_getActivator(const celix_bundle_t *bundle);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t bundle_setActivator(celix_bundle_t *bundle, celix_bundle_activator_t *activator);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t bundle_getContext(const celix_bundle_t *bundle, celix_bundle_context_t **context);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t bundle_setContext(celix_bundle_t *bundle, celix_bundle_context_t *context);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t bundle_getEntry(const celix_bundle_t *bundle, const char *name, char **entry);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t bundle_start(celix_bundle_t *bundle);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t bundle_update(celix_bundle_t *bundle, const char *inputFile);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t bundle_stop(celix_bundle_t *bundle);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t bundle_uninstall(celix_bundle_t *bundle);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t bundle_setState(celix_bundle_t *bundle, bundle_state_e state);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t bundle_setPersistentStateInactive(celix_bundle_t *bundle);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t bundle_setPersistentStateUninstalled(celix_bundle_t *bundle);

CELIX_FRAMEWORK_DEPRECATED_EXPORT void uninstallBundle(celix_bundle_t *bundle);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t bundle_revise(celix_bundle_t *bundle, const char *location, const char *inputFile);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t bundle_addModule(celix_bundle_t *bundle, module_pt module);

// Service Reference Functions
CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_array_list_t *getUsingBundles(service_reference_pt reference);

CELIX_FRAMEWORK_DEPRECATED_EXPORT int compareTo(service_reference_pt a, service_reference_pt b);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t bundle_getState(const celix_bundle_t *bundle, bundle_state_e *state);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t bundle_closeAndDelete(const celix_bundle_t *bundle);

CELIX_FRAMEWORK_EXPORT celix_status_t bundle_close(const celix_bundle_t *bundle)
    __attribute__((deprecated("bundle_close is deprecated and does nothing")));

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t bundle_refresh(celix_bundle_t *bundle);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t bundle_getBundleId(const celix_bundle_t *bundle, long *id);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t bundle_getRegisteredServices(celix_bundle_t *bundle, celix_array_list_t **list);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t bundle_getServicesInUse(celix_bundle_t *bundle, celix_array_list_t **list);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t bundle_getFramework(const celix_bundle_t *bundle, celix_framework_t **framework);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t bundle_getBundleLocation(const celix_bundle_t *bundle, const char **location);


#ifdef __cplusplus
}
#endif

#endif /* BUNDLE_H_ */
