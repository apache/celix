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

#include <stdlib.h>

#include "celix_bundle_context.h"

#ifndef CELIX_BUNDLE_ACTIVATOR_H_
#define CELIX_BUNDLE_ACTIVATOR_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Called when this bundle is started so the bundle can create an instance for its activator.
 * The framework does not assume any type for the activator instance, this is implementation specific.
 * The activator instance is handle as a void pointer by the framework, the implementation must cast it to the
 * implementation specific type.
 *
 * @param ctx The execution context of the bundle being started.
 * @param[out] userData A pointer to the specific activator instance used by this bundle.
 *
 * @return Status code indication failure or success:
 * 		- CELIX_SUCCESS when no errors are encountered.
 * 		- Any other status code will mark the bundle as stopped and the framework will remove this
 * 		  bundle's listeners, unregister all services, and release all services used by this bundle.
 */
celix_status_t celix_bundleActivator_create(celix_bundle_context_t *ctx, void **userData);

/**
 * Called when this bundle is started so the Framework can perform the bundle-specific activities necessary
 * to start this bundle. This method can be used to register services or to allocate any resources that this
 * bundle needs.
 *
 * <p>
 * This method must complete and return to its caller in a timely manner.
 *
 * @param userData The activator instance to be used.
 * @param ctx The execution context of the bundle being started.
 *
 * @return Status code indication failure or success:
 * 		- CELIX_SUCCESS when no errors are encountered.
 * 		- Any other status code will mark the bundle as stopped and the framework will remove this
 * 		  bundle's listeners, unregister all services, and release all services used by this bundle.
 */
celix_status_t celix_bundleActivator_start(void *userData, celix_bundle_context_t *ctx);

/**
 * Called when this bundle is stopped so the Framework can perform the bundle-specific activities necessary
 * to stop the bundle. In general, this method should undo the work that the <code>bundleActivator_start()</code>
 * function started. There should be no active threads that were started by this bundle when this bundle returns.
 * A stopped bundle must not call any Framework objects.
 *
 * <p>
 * This method must complete and return to its caller in a timely manner.
 *
 * @param userData The activator instance to be used.
 * @param ctx The execution context of the bundle being stopped.
 *
 * @return Status code indication failure or success:
 * 		- CELIX_SUCCESS when no errors are encountered.
 * 		- Any other status code will mark the bundle as stopped and the framework will remove this
 * 		  bundle's listeners, unregister all services, and release all services used by this bundle.
 */
celix_status_t celix_bundleActivator_stop(void *userData, celix_bundle_context_t *ctx);

/**
 * Called when this bundle is stopped so the bundle can destroy the instance of its activator. In general, this
 * method should undo the work that the <code>bundleActivator_create()</code> function initialized.
 *
 * <p>
 * This method must complete and return to its caller in a timely manner.
 *
 * @param userData The activator instance to be used.
 * @param ctx The execution context of the bundle being stopped.
 *
 * @return Status code indication failure or success:
 * 		- CELIX_SUCCESS when no errors are encountered.
 * 		- Any other status code will mark the bundle as stopped and the framework will remove this
 * 		  bundle's listeners, unregister all services, and release all services used by this bundle.
 */
celix_status_t celix_bundleActivator_destroy(void *userData, celix_bundle_context_t* ctx);


/**
 * This macro generates the required bundle activator functions for C.
 * This can be used to more type safe bundle activator entries.
 *
 * The macro will create the following bundle activator functions:
 * - bundleActivator_create which allocates a pointer to the provided type.
 * - bundleActivator_start/stop which will call the respectively provided typed start/stop functions.
 * - bundleActivator_destroy will free the allocated for the provided type.
 *
 * @param type The activator type (e.g. 'struct shell_activator').
 * @param start the activator actStart function with the following signature: celix_status_t (*)(<actType>*, celix_bundle_context_t*).
 * @param stop the activator actStop function with the following signature: celix_status_t (*)(<actType>*, celix_bundle_context_t*).
 */
#define CELIX_GEN_BUNDLE_ACTIVATOR(actType, actStart, actStop)                                                         \
celix_status_t celix_bundleActivator_create(celix_bundle_context_t *ctx __attribute__((unused)), void **userData) {    \
    celix_status_t status = CELIX_SUCCESS;                                                                             \
    actType *data = (actType*)calloc(1, sizeof(*data));                                                                \
    if (data != NULL) {                                                                                                \
        *userData = data;                                                                                              \
    } else {                                                                                                           \
        status = CELIX_ENOMEM;                                                                                         \
    }                                                                                                                  \
    return status;                                                                                                     \
}                                                                                                                      \
                                                                                                                       \
celix_status_t celix_bundleActivator_start(void *userData, celix_bundle_context_t *ctx) {                              \
    return actStart((actType*)userData, ctx);                                                                          \
}                                                                                                                      \
                                                                                                                       \
celix_status_t celix_bundleActivator_stop(void *userData, celix_bundle_context_t *ctx) {                               \
    return actStop((actType*)userData, ctx);                                                                           \
}                                                                                                                      \
                                                                                                                       \
celix_status_t celix_bundleActivator_destroy(void *userData, celix_bundle_context_t *ctx __attribute__((unused))) {    \
    free(userData);                                                                                                    \
    return CELIX_SUCCESS;                                                                                              \
}

#ifdef __cplusplus
}


/**
 * This macro generates the required bundle activator functions for C++.
 * This can be used to more type safe bundle activator entries.
 *
 * The macro will create the following bundle activator functions:
 * - bundleActivator_create which allocates a pointer to the provided type.
 * - bundleActivator_start/stop which will call the respectively provided typed start/stop functions.
 * - bundleActivator_destroy will free the allocated for the provided type.
 *
 * @param type The activator type (e.g. 'ShellActivator'). A type which should have a constructor with a single arugment of std::shared_ptr<DependencyManager>.
 */
#define CELIX_GEN_CXX_BUNDLE_ACTIVATOR(actType)                                                                        \
                                                                                                                       \
namespace /*anon*/ {                                                                                                   \
struct BundleActivatorData {                                                                                           \
    std::shared_ptr<DependencyManager> mng{};                                                                          \
    std::unique_ptr<actType> activator{};                                                                              \
};                                                                                                                     \
}                                                                                                                      \
                                                                                                                       \
extern "C" celix_status_t bundleActivator_create(celix_bundle_context_t *context, void** userData) {                   \
    int status = CELIX_SUCCESS;                                                                                        \
                                                                                                                       \
    BundleActivatorData* data = nullptr;                                                                               \
    data = new BundleActivatorData{};                                                                                  \
    if (data != nullptr) {                                                                                             \
        data->mng = std::shared_ptr<celix::dm::DependencyManager>{new celix::dm::DependencyManager{context}};          \
    }                                                                                                                  \
                                                                                                                       \
    if (data == nullptr || data->mng == nullptr) {                                                                     \
        status = CELIX_ENOMEM;                                                                                         \
        if (data != nullptr) {                                                                                         \
            delete data;                                                                                               \
        }                                                                                                              \
        *userData = nullptr;                                                                                           \
    } else {                                                                                                           \
        *userData = data;                                                                                              \
    }                                                                                                                  \
    return status;                                                                                                     \
}                                                                                                                      \
                                                                                                                       \
extern "C" celix_status_t bundleActivator_start(void *userData, celix_bundle_context_t *) {                            \
    auto* data = static_cast<BundleActivatorData*>(userData);                                                          \
    if (data != nullptr) {                                                                                             \
        data->activator = std::unique_ptr<actType>{new actType{data->mng}};                                            \
        data->mng->start();                                                                                            \
    }                                                                                                                  \
    return CELIX_SUCCESS;                                                                                              \
}                                                                                                                      \
                                                                                                                       \
extern "C" celix_status_t bundleActivator_stop(void *userData, celix_bundle_context_t*) {                              \
    auto* data = static_cast<BundleActivatorData*>(userData);                                                          \
    if (data != nullptr) {                                                                                             \
        data->mng->stop();                                                                                             \
        data->activator = nullptr;                                                                                     \
        data->mng = nullptr;                                                                                           \
    }                                                                                                                  \
    return CELIX_SUCCESS;                                                                                              \
}                                                                                                                      \
                                                                                                                       \
extern "C" celix_status_t bundleActivator_destroy(void *userData, celix_bundle_context_t*) {                           \
    auto* data = static_cast<BundleActivatorData*>(userData);                                                          \
    if (data != nullptr) {                                                                                             \
        delete data;                                                                                                   \
    }                                                                                                                  \
    return CELIX_SUCCESS;                                                                                              \
}                                                                                                                      \


#endif

#endif /* CELIX_BUNDLE_ACTIVATOR_H_ */