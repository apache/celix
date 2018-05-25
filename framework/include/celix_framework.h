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
#ifndef CELIX_FRAMEWORK_H_
#define CELIX_FRAMEWORK_H_

typedef struct framework celix_framework_t;

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


#ifdef __cplusplus
}
#endif

#endif /* CELIX_FRAMEWORK_H_ */
