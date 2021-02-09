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
            long bndId;
            std::shared_ptr<celix::BundleContext> ctx;
            std::shared_ptr<celix::dm::DependencyManager> dm;
            std::unique_ptr<I> bundleActivator;
        };


        template<typename I>
        typename std::enable_if<std::is_constructible<I, std::shared_ptr<celix::BundleContext>>::value, celix_status_t>::type
        inline createActivator(celix_bundle_context_t *cCtx, void **out) {
            auto ctx = std::make_shared<celix::BundleContext>(cCtx);
            auto dm = std::make_shared<celix::dm::DependencyManager>(cCtx);
            auto act = std::unique_ptr<I>(new I{ctx});
            auto *data = new BundleActivatorData<I>{ctx->getBundleId(), std::move(ctx), std::move(dm), std::move(act)};
            *out = (void *) data;
            return CELIX_SUCCESS;
        }

        template<typename I>
        typename std::enable_if<std::is_constructible<I, std::shared_ptr<celix::dm::DependencyManager>>::value, celix_status_t>::type
        inline createActivator(celix_bundle_context_t *cCtx, void **out) {
            auto ctx = std::make_shared<celix::BundleContext>(cCtx);
            auto dm = std::make_shared<celix::dm::DependencyManager>(cCtx);
            auto act = std::unique_ptr<I>(new I{dm});
            dm->start();
            auto *data = new BundleActivatorData<I>{ctx->getBundleId(), std::move(ctx), std::move(dm), std::move(act)};
            *out = (void *) data;
            return CELIX_SUCCESS;
        }

        template<typename T>
        inline void waitForExpired(long bndId, std::weak_ptr<celix::BundleContext>& weakCtx, const char* name, std::weak_ptr<T>& observe) {
            auto start = std::chrono::system_clock::now();
            while (!observe.expired()) {
                auto now = std::chrono::system_clock::now();
                auto durationInSec = std::chrono::duration_cast<std::chrono::seconds>(now - start);
                if (durationInSec > std::chrono::seconds{5}) {
                    auto msg =  std::string{"Cannot destroy bundle "} + std::to_string(bndId) + ". " + name + " is still in use. std::shared_ptr use count is " + std::to_string(observe.use_count()) + "\n";
                    auto ctx = weakCtx.lock();
                    if (ctx) {
                        ctx->logWarn(msg.c_str());
                    } else {
                        std::cout << msg;
                    }
                    start = now;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds{50});
            }
        }

        template<typename I>
        inline celix_status_t destroyActivator(void *userData) {
            auto *data = static_cast<BundleActivatorData<I> *>(userData);
            std::weak_ptr<celix::BundleContext> ctx = data->ctx;
            std::weak_ptr<celix::dm::DependencyManager> dm = data->dm;
            data->bundleActivator = nullptr;
            data->dm = nullptr;
            data->ctx = nullptr;
            waitForExpired(data->bndId, ctx, "celix::BundleContext", ctx);
            waitForExpired(data->bndId, ctx, "celix::dm::DependencyManager", dm);
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

