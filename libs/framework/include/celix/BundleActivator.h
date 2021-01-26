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

#pragma once

#include <memory>
#include <celix/dm/DependencyManager.h>

#include "celix_bundle_activator.h"
#include "celix/BundleContext.h"

namespace celix {
    namespace impl {

        template<typename I>
        struct BundleActivatorData {
            std::weak_ptr<celix::BundleContext> ctx;
            std::weak_ptr<celix::dm::DependencyManager> dm;
            std::unique_ptr<I> bundleActivator;
        };


        template<typename I>
        typename std::enable_if<std::is_constructible<I, std::shared_ptr<celix::BundleContext>>::value, celix_status_t>::type
        createActivator(celix_bundle_context_t *cCtx, void **out) {
            auto ctx = std::make_shared<celix::BundleContext>(cCtx);
            auto dm = std::make_shared<celix::dm::DependencyManager>(cCtx);
            auto act = std::unique_ptr<I>(new I{ctx});
            auto *data = new BundleActivatorData<I>{std::move(ctx), std::move(dm), std::move(act)};
            *out = (void *) data;
            return CELIX_SUCCESS;
        }

        template<typename I>
        typename std::enable_if<std::is_constructible<I, std::shared_ptr<celix::dm::DependencyManager>>::value, celix_status_t>::type
        createActivator(celix_bundle_context_t *cCtx, void **out) {
            auto ctx = std::make_shared<celix::BundleContext>(cCtx);
            auto dm = std::make_shared<celix::dm::DependencyManager>(cCtx);
            auto act = std::unique_ptr<I>(new I{dm});
            dm->start();
            auto *data = new BundleActivatorData<I>{std::move(ctx), std::move(dm), std::move(act)};
            *out = (void *) data;
            return CELIX_SUCCESS;
        }

        template<typename I>
        celix_status_t destroyActivator(void *userData) {
            auto *data = static_cast<BundleActivatorData<I> *>(userData);
            data->bundleActivator = nullptr;
            while (!data->ctx.expired()) {
                std::cerr << "Cannot destroy bundle. Bundle context is still in use. Count is " << data->ctx.use_count()
                          << std::endl;
            }
            delete data;
            return CELIX_SUCCESS;
        }
    }
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
extern "C" celix_status_t bundleActivator_create(celix_bundle_context_t *context, void** userData) {                   \
    return celix::impl::createActivator<actType>(context, userData);                                                   \
}                                                                                                                      \
                                                                                                                       \
extern "C" celix_status_t bundleActivator_start(void *, celix_bundle_context_t *) {                                    \
    /*nop*/                                                                                                            \
    return CELIX_SUCCESS;                                                                                              \
}                                                                                                                      \
                                                                                                                       \
extern "C" celix_status_t bundleActivator_stop(void *, celix_bundle_context_t*) {                                      \
    /*nop*/                                                                                                            \
    return CELIX_SUCCESS;                                                                                              \
}                                                                                                                      \
                                                                                                                       \
extern "C" celix_status_t bundleActivator_destroy(void *userData, celix_bundle_context_t*) {                           \
    return celix::impl::destroyActivator<actType>(userData);                                                           \
}                                                                                                                      \

