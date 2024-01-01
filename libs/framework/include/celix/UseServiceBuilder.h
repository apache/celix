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

#include <string>
#include <vector>
#include <memory>

#include "celix/Filter.h"
#include "celix/Bundle.h"
#include "celix/Properties.h"

namespace celix {
    /**
     * @brief Fluent builder API to use a service or services.
     *
     * The UseServiceBuilder can be used to use a service or a set of services registered in the celix
     * framework. The builder can be used to specify additional match filtering for services, configure use callbacks
     * and whether a single or multiple services will be used. For a single service the highest ranking service will
     * be used. The builder should be finished with a `build` call.
     *
     *      ctx->useService<ICalc>()
     *          .addUseCallback([](ICalc& calc) {
     *              std::cout << "result is " << calc.add(2, 3) << std::endl;
     *           })
     *           .build();
     *
     *
     * @note Not thread safe.
     * @tparam I The service type to use
     */
    template<typename I>
    class UseServiceBuilder {
    private:
        friend class BundleContext;

        //NOTE private to prevent move so that a build() call cannot be forgotten
        UseServiceBuilder(UseServiceBuilder&&) = default;
    public:
        explicit UseServiceBuilder(std::shared_ptr<celix_bundle_context_t> _cCtx, std::string _name, bool _useSingleService = true) :
                cCtx{std::move(_cCtx)},
                name{std::move(_name)},
                useSingleService{_useSingleService} {}

        UseServiceBuilder& operator=(UseServiceBuilder&&) = delete;
        UseServiceBuilder(const UseServiceBuilder&) = delete;
        UseServiceBuilder operator=(const UseServiceBuilder&) = delete;

        /**
         * @brief Set filter to be used to select a service.
         *
         * The filter must be LDAP filter.
         * Example:
         *      "(property_key=value)"
         */
        UseServiceBuilder& setFilter(const std::string& f) { filter = celix::Filter{f}; return *this; }

        /**
         * @brief Set filter to be used to matching services.
         */
        UseServiceBuilder& setFilter(celix::Filter f) { filter = std::move(f); return *this; }

        /**
         * @brief Sets a optional timeout.
         *
         * If the timeout is > 0 and there is no matching service, the "build" will block
         * until a matching service is found or the timeout is expired.
         *
         * Note: timeout is only valid if the a single service is used.
         *
         */
        template <typename Rep, typename Period>
        UseServiceBuilder& setTimeout(std::chrono::duration<Rep, Period> duration) {
            auto micro =  std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
            timeoutInSeconds = micro / 1000000.0;
            return *this;
        }

        /**
         * @brief Adds a use callback function which will be called when the UseServiceBuilder is
         * "build".
         *
         * The use callback function has 1 argument: a reference to the matching service.
         */
        UseServiceBuilder& addUseCallback(std::function<void(I&)> cb) {
            callbacks.emplace_back(std::move(cb));
            return *this;
        }

        /**
         * @brief Adds a use callback function which will be called when the UseServiceBuilder is
         * "build".
         *
         * The use callback function has 2 arguments:
         *  - A reference to the matching service.
         *  - A const reference to the matching service properties.
         */
        UseServiceBuilder& addUseCallback(std::function<void(I&, const celix::Properties&)> cb) {
            callbacksWithProperties.emplace_back(std::move(cb));
            return *this;
        }

        /**
         * @brief Adds a use callback function which will be called when the UseServiceBuilder is
         * "build".
         *
         * The use callback function has 3 arguments:
         *  - A reference to the matching service.
         *  - A const reference to the matching service properties.
         *  - A const reference to the bundle owning the matching service.
         */
        UseServiceBuilder& addUseCallback(std::function<void(I&, const celix::Properties&, const celix::Bundle&)> cb) {
            callbacksWithOwner.emplace_back(std::move(cb));
            return *this;
        }

        /**
         * @brief "Builds" the UseServiceBuild.
         *
         * Blocks until:
         *  - All the use callback functions are called with the highest ranking matching service
         *  - The timeout has expired
         *
         *  This means this if no services are found and the timeout is 0 (default) the function will
         *  return without blocking.
         *
         * \returns the number of services found and provided to the callback(s).
         */
        std::size_t build() {
            celix_service_use_options_t opts{};
            opts.filter.serviceName = name.empty() ? nullptr : name.c_str();
            opts.filter.filter = filter.empty() ? nullptr : filter.getFilterCString();
            opts.filter.versionRange = versionRange.empty() ? nullptr : versionRange.c_str();
            opts.waitTimeoutInSeconds = timeoutInSeconds;
            opts.callbackHandle = this;
            opts.useWithOwner = [](void* data, void *voidSvc, const celix_properties_t* cProps, const celix_bundle_t* cBnd) {
                auto* builder = static_cast<UseServiceBuilder<I>*>(data);
                auto* svc = static_cast<I*>(voidSvc);
                const Bundle bnd{const_cast<celix_bundle_t*>(cBnd)};
                auto props = celix::Properties::wrap(cProps);
                for (const auto& func : builder->callbacks) {
                    func(*svc);
                }
                for (const auto& func : builder->callbacksWithProperties) {
                    func(*svc, props);
                }
                for (const auto& func : builder->callbacksWithOwner) {
                    func(*svc, props, bnd);
                }
            };

            if (useSingleService) {
                bool called = celix_bundleContext_useServiceWithOptions(cCtx.get(), &opts);
                return called ? 1 : 0;
            } else {
                return celix_bundleContext_useServicesWithOptions(cCtx.get(), &opts);
            }
        }
    private:
        const std::shared_ptr<celix_bundle_context_t> cCtx;
        const std::string name;
        const bool useSingleService;
        double timeoutInSeconds{0};
        celix::Filter filter{};
        std::string versionRange{};
        std::vector<std::function<void(I&)>> callbacks{};
        std::vector<std::function<void(I&, const celix::Properties&)>> callbacksWithProperties{};
        std::vector<std::function<void(I&, const celix::Properties&, const celix::Bundle&)>> callbacksWithOwner{};
    };
}