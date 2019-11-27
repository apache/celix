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

#ifndef CELIX_FRAMEWORK_H_
#define CELIX_FRAMEWORK_H_


#include "celix_types.h"
#include "celix_properties.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returns the framework UUID. This is unique for every created framework and will not be the same if the process is
 * restarted.
 */
const char* celix_framework_getUUID(const celix_framework_t *fw);

/**
 * Returns the framework bundle context. This is the same as a 'normal' bundle context and can be used to register, use
 * and track services. The only difference is that the framework is the bundle.
 * @param fw The framework
 * @return A pointer to the bundle context of the framework or NULL if something went wrong.
 */
celix_bundle_context_t* celix_framework_getFrameworkContext(const celix_framework_t *fw);

/**
 * Returns the framework bundle. This is the same as a 'normal' bundle, expect that this bundle cannot be uninstalled
 * and the `celix_bundle_getEntry` return a entries relative from the working directory.
  * @param fw The framework
 * @return A pointer to the bundle of the framework or NULL if something went wrong.
 */
celix_bundle_t* celix_framework_getFrameworkBundle(const celix_framework_t *fw);

/**
 * Use the currently active (started) bundles.
 * The provided callback will be called for all the currently started bundles.
 *
 * @param ctx                       The bundle context.
 * @param includeFrameworkBundle    If true the callback will also be triggered for the framework bundle.
 * @param callbackHandle            The data pointer, which will be used in the callbacks
 * @param use                       The callback which will be called for the currently started bundles.
 *                                  The bundle pointers are only guaranteed to be valid during the callback.
 */
void celix_framework_useBundles(celix_framework_t *fw, bool includeFrameworkBundle, void *callbackHandle, void(*use)(void *handle, const celix_bundle_t *bnd));

/**
 * Use the bundle with the provided bundle id
 * The provided callback will be called if the bundle is found.
 *
 * @param fw                The framework.
 * @param onlyActive        If true only starting and active bundles will trigger the callback.
 * @param bundleId          The bundle id.
 * @param callbackHandle    The data pointer, which will be used in the callbacks
 * @param use               The callback which will be called for the currently started bundles.
 *                          The bundle pointers are only guaranteed to be valid during the callback.
 * @return                  Returns true if the bundle is found and the callback is called.
 */
bool celix_framework_useBundle(celix_framework_t *fw, bool onlyActive, long bndId, void *callbackHandle, void(*use)(void *handle, const celix_bundle_t *bnd));



#ifdef __cplusplus
}
#endif

#endif /* CELIX_FRAMEWORK_H_ */
