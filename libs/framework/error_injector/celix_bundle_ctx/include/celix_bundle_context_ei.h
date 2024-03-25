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

#ifndef CELIX_BUNDLE_CONTEXT_EI_H
#define CELIX_BUNDLE_CONTEXT_EI_H
#ifdef __cplusplus
extern "C" {
#endif
#include "celix_error_injector.h"
#include "celix_errno.h"
#include "celix_bundle_context.h"

CELIX_EI_DECLARE(celix_bundleContext_getProperty, const char*);
CELIX_EI_DECLARE(celix_bundleContext_registerServiceWithOptionsAsync, long);
CELIX_EI_DECLARE(celix_bundleContext_trackServicesWithOptionsAsync, long);
CELIX_EI_DECLARE(celix_bundleContext_getBundleId, long);
CELIX_EI_DECLARE(bundleContext_getServiceReferences, celix_status_t);
CELIX_EI_DECLARE(bundleContext_retainServiceReference, celix_status_t);
CELIX_EI_DECLARE(celix_bundleContext_registerServiceAsync, long);
CELIX_EI_DECLARE(celix_bundleContext_registerServiceFactoryAsync, long);
CELIX_EI_DECLARE(celix_bundleContext_scheduleEvent, long);
CELIX_EI_DECLARE(celix_bundleContext_getDependencyManager, celix_dependency_manager_t*);
CELIX_EI_DECLARE(celix_bundleContext_trackBundlesWithOptionsAsync, long);

#ifdef __cplusplus
}
#endif

#endif //CELIX_BUNDLE_CONTEXT_EI_H
