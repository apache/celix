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

#include "celix/dm/DependencyManager.h"
#include "celix_bundle_activator.h"

#if __cplusplus >= 201402L //C++14 or higher
#include "celix/BundleContext.h"
namespace celix {
namespace impl {
    template<typename I>
    struct BundleActivatorData {
        long bndId{};
        std::shared_ptr<celix::BundleContext> ctx;
        std::unique_ptr<I> bundleActivator;
    };

    template<typename I>
    typename std::enable_if<std::is_constructible<I, std::shared_ptr<celix::BundleContext>>::value, celix_status_t>::type
    createActivator(celix_bundle_context_t * cCtx, void * *out) {
        auto ctx = std::make_shared<celix::BundleContext>(cCtx);
        auto act = std::make_unique<I>(ctx);
        auto *data = new BundleActivatorData<I>{ctx->getBundleId(), std::move(ctx), std::move(act)};
        *out = (void *) data;
        return CELIX_SUCCESS;
    }

    template<typename I>
    typename std::enable_if<std::is_constructible<I, std::shared_ptr<celix::dm::DependencyManager>>::value, celix_status_t>::type
    createActivator(celix_bundle_context_t * cCtx, void * *out) {
        auto ctx = std::make_shared<celix::BundleContext>(cCtx);
        auto dm = ctx->getDependencyManager();
        auto act = std::make_unique<I>(dm);
        dm->start();
        auto *data = new BundleActivatorData<I>{ctx->getBundleId(), std::move(ctx), std::move(act)};
        *out = (void *) data;
        return CELIX_SUCCESS;
    }

    template<typename T>
    void waitForExpired(long bndId, std::weak_ptr<celix::BundleContext> &weakCtx, const char *name,
                        std::weak_ptr<T> &observe) {
        auto start = std::chrono::steady_clock::now();
        while (!observe.expired()) {
            auto now = std::chrono::steady_clock::now();
            auto durationInSec = std::chrono::duration_cast<std::chrono::seconds>(now - start);
            if (durationInSec > std::chrono::seconds{5}) {
                auto msg = std::string{"Cannot destroy bundle "} + std::to_string(bndId) + ". " + name +
                           " is still in use. std::shared_ptr use count is " + std::to_string(observe.use_count()) +
                           "\n";
                auto ctx = weakCtx.lock();
                if (ctx) {
                    ctx->logWarn("%s", msg.c_str());
                } else {
                    std::cout << msg;
                }
                start = now;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds{50});
        }
    }

    template<typename I>
    celix_status_t destroyActivator(void *userData) {
        auto *data = static_cast<BundleActivatorData<I> *>(userData);
        data->bundleActivator.reset();
        data->ctx->getDependencyManager()->clear();

        auto bndId = data->bndId;
        std::weak_ptr<celix::BundleContext> ctx = data->ctx;
        std::weak_ptr<celix::dm::DependencyManager> dm = data->ctx->getDependencyManager();
        delete data;
        waitForExpired(bndId, ctx, "celix::BundleContext", ctx);
        waitForExpired(bndId, ctx, "celix::dm::DependencyManager", dm);
        return CELIX_SUCCESS;
    }
}}
#else //C++11
namespace celix {
namespace impl {
    template<typename I>
    struct BundleActivatorData {
        std::shared_ptr<celix::dm::DependencyManager> dm;
        std::unique_ptr<I> bundleActivator;
    };

    template<typename I>
    typename std::enable_if<std::is_constructible<I, std::shared_ptr<celix::dm::DependencyManager>>::value, celix_status_t>::type
    createActivator(celix_bundle_context_t *cCtx, void **out) {
        auto dm =  std::make_shared<celix::dm::DependencyManager>(cCtx);
        auto act = std::unique_ptr<I>(new I{dm});
        dm->start();
        auto *data = new BundleActivatorData<I>{std::move(dm), std::move(act)};
        *out = (void *) data;
        return CELIX_SUCCESS;
    }

    template<typename I>
    celix_status_t destroyActivator(void *userData) {
        auto *data = static_cast<BundleActivatorData<I> *>(userData);
        data->bundleActivator.reset();
        data->dm->clear();
        delete data;
        return CELIX_SUCCESS;
    }
} //end namespace impl
} //end namespace celix
#endif

/**
 * @brief Macro to generate the required bundle activator functions for C++.
 * This can be used to more type safe bundle activator entries.
 *
 * The macro will create the following bundle activator C functions:
 * - bundleActivator_create, which will create the required C++ object (bundle context and dependency manager) and create
 *   the C++ bundle activator class (RAII)
 * - bundleActivator_start function, which for C++ will do nothing.
 * - bundleActivator_stop function, which will trigger the destruction of the C++ BundleActivator and ensure that
 *   there is no dangling usage of the bundle context and/or dependency manager.
 * - bundleActivator_destroy function, which for C++ will do nothing.
 *
 * The destruction of the C++ BundleActivator is triggered in the bundleActivator_stop instead of
 * bundleActivator_destroy to ensure that the C++ dependency manager does is cleanup before the C dependency manager.
 * This is needed, because the C dependency manager is not aware of "above lying" C++ objects.
 *
 * @param type The activator type (e.g. 'ShellActivator'). A type which should have a constructor with a single
 * argument of std::shared_ptr<celix::BundleContext> or std::shared_ptr<DependencyManager>.
 */
#define CELIX_GEN_CXX_BUNDLE_ACTIVATOR(actType)                                                                        \
extern "C" celix_status_t celix_bundleActivator_create(celix_bundle_context_t *context, void** userData) {             \
    return celix::impl::createActivator<actType>(context, userData);                                                   \
}                                                                                                                      \
                                                                                                                       \
extern "C" celix_status_t celix_bundleActivator_start(void *, celix_bundle_context_t *) {                              \
    /*nop*/                                                                                                            \
    return CELIX_SUCCESS;                                                                                              \
}                                                                                                                      \
                                                                                                                       \
extern "C" celix_status_t celix_bundleActivator_stop(void *userData, celix_bundle_context_t*) {                        \
    return celix::impl::destroyActivator<actType>(userData);                                                           \
}                                                                                                                      \
                                                                                                                       \
extern "C" celix_status_t celix_bundleActivator_destroy(void *, celix_bundle_context_t*) {                             \
    /*nop*/                                                                                                            \
    return CELIX_SUCCESS;                                                                                              \
}
