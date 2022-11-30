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

#include "Trackers.h"

namespace celix {

    /**
     * @brief Fluent builder API to track services.
     *
     * @see celix::BundleContext::trackServices for more info.
     * @tparam I The service type to track.
     * @note Not thread safe.
     */
    template<typename I>
    class ServiceTrackerBuilder {
    private:
        friend class BundleContext;

        //NOTE private to prevent move so that a build() call cannot be forgotten
        ServiceTrackerBuilder(ServiceTrackerBuilder&&) noexcept = default;
    public:
        explicit ServiceTrackerBuilder(std::shared_ptr<celix_bundle_context_t> _cCtx, std::string _name) :
                cCtx{std::move(_cCtx)},
                name{std::move(_name)} {}

        ServiceTrackerBuilder& operator=(ServiceTrackerBuilder&&) = delete;
        ServiceTrackerBuilder(const ServiceTrackerBuilder&) = delete;
        ServiceTrackerBuilder operator=(const ServiceTrackerBuilder&) = delete;

        /**
         * @brief Set filter to be used to matching services.
         *
         * The filter must be LDAP filter.
         * Example:
         *      "(property_key=value)"
         */
#if __cplusplus >= 201703L //C++17 or higher
        ServiceTrackerBuilder& setFilter(std::string_view f) { filter = celix::Filter{f}; return *this; }
#else
        ServiceTrackerBuilder& setFilter(const std::string& f) { filter = celix::Filter{f}; return *this; }
#endif

        /**
         * @brief Set filter to be used to matching services.
         */
        ServiceTrackerBuilder& setFilter(celix::Filter f) { filter = std::move(f); return *this; }

        /**
         * @brief Adds a add callback function, which will be called - on the Celix event thread -
         * when a new service match is found.
         *
         * @tparam F The callback function type. Signature should be compatible with std::function<void(const std::share_ptr<I>& service)>
         * @param add The callback function which will be called when a new service is found.
         * @return The ServiceTrackerBuilder reference for chaining (Fluent API).
         */
        template<typename F>
        ServiceTrackerBuilder& addAddCallback(F&& add) {
            addCallbacks.emplace_back([add = std::forward<F>(add)](const std::shared_ptr<I>& svc, const std::shared_ptr<const celix::Properties>&, const std::shared_ptr<const celix::Bundle>&) {
                add(svc);
            });
            return *this;
        }

         /**
          * @brief Adds a add callback function, which will be called - on the Celix event thread -
          * when a new service match is found.
          *
          *
          * @tparam F The callback function type. Signature should be compatible with std::function<void(const std::share_ptr<I>& service, const std::shared_ptr<const celix::Properties>& properties)>
          * @param add The callback function which will be called when a new service is found.
          * @return The ServiceTrackerBuilder reference for chaining (Fluent API).
          */
        template<typename F>
        ServiceTrackerBuilder& addAddWithPropertiesCallback(F&& add) {
            addCallbacks.emplace_back([add = std::forward<F>(add)](const std::shared_ptr<I>& svc, const std::shared_ptr<const celix::Properties>& props, const std::shared_ptr<const celix::Bundle>&) {
                add(svc, props);
            });
            return *this;
        }

        /**
         * @brief Adds a add callback function, which will be called - on the Celix event thread -
         * when a new service match is found.
         *
         * @tparam F The callback function type. Signature should be compatible with std::function<void(const std::share_ptr<I>& service, const std::shared_ptr<const celix::Properties>& properties, const std::shared_ptr<const celix::Bundle>&)>
         * @param add The callback function which will be called when a new service is found.
         * @return The ServiceTrackerBuilder reference for chaining (Fluent API).
         */
        template<typename F>
        ServiceTrackerBuilder& addAddWithOwnerCallback(F&& add) {
            addCallbacks.emplace_back(std::forward<F>(add));
            return *this;
        }

        /**
         * @brief Adds a remove callback function, which will be called - on the Celix event thread -
         * when a service match is being removed.
         *
         * @tparam F The callback function type. Signature should be compatible with std::function<void(const std::share_ptr<I>& service)>
         * @param remove The callback function which will be called when a added service is being removed.
         * @return The ServiceTrackerBuilder reference for chaining (Fluent API).
         */
        template<typename F>
        ServiceTrackerBuilder& addRemCallback(F&& remove) {
            remCallbacks.emplace_back([remove = std::forward<F>(remove)](const std::shared_ptr<I>& svc, const std::shared_ptr<const celix::Properties>&, const std::shared_ptr<const celix::Bundle>&) {
                remove(svc);
            });
            return *this;
        }

        /**
         * @brief Adds a remove callback function, which will be called - on the Celix event thread -
         * when a service match is being removed.
         *
         * @tparam F The callback function type. Signature should be compatible with std::function<void(const std::share_ptr<I>& service, const std::shared_ptr<const celix::Properties>& properties)>
         * @param remove The callback function which will be called when a added service is being removed.
         * @return The ServiceTrackerBuilder reference for chaining (Fluent API).
         */
        template<typename F>
        ServiceTrackerBuilder& addRemWithPropertiesCallback(F&& remove) {
            remCallbacks.emplace_back([remove = std::forward<F>(remove)](const std::shared_ptr<I>& svc, const std::shared_ptr<const celix::Properties>& props, const std::shared_ptr<const celix::Bundle>&) {
                remove(svc, props);
            });
            return *this;
        }

        /**
         * @brief Adds a remove callback function, which will be called - on the Celix event thread -
         * when a service match is being removed.
         *
         * @tparam F The callback function type. Signature should be compatible with std::function<void(const std::share_ptr<I>& service, const std::shared_ptr<const celix::Properties>& properties, const std::shared_ptr<const celix::Bundle>&)>
         * @param remove The callback function which will be called when a added service is being removed.
         * @return The ServiceTrackerBuilder reference for chaining (Fluent API).
         */
        template<typename F>
        ServiceTrackerBuilder& addRemWithOwnerCallback(F&& remove) {
            remCallbacks.emplace_back(std::forward<F>(remove));
            return *this;
        }

        /**
         * @brief Adds a set callback function, which will be called - on the Celix event thread -
         * when there is a new highest ranking service match.
         * This can can also be an empty match (nullptr).
         *
         * @tparam F The callback function type. Signature should be compatible with std::function<void(const std::share_ptr<I>& service)>
         * @param set The callback function which will be called when a new highest ranking service is found or when the last matching service is being removed (svc and properties will be a nullptr).
         * @return The ServiceTrackerBuilder reference for chaining (Fluent API).
         */
        template<typename F>
        ServiceTrackerBuilder& addSetCallback(F&& set) {
            setCallbacks.emplace_back([set = std::forward<F>(set)](const std::shared_ptr<I>& svc, const std::shared_ptr<const celix::Properties>&, const std::shared_ptr<const celix::Bundle>&) {
                set(svc);
            });
            return *this;
        }

        /**
         * @brief Adds a set callback function, which will be called - on the Celix event thread -
         * when there is a new highest ranking service match.
         * This can can also be an empty match (nullptr).
         *
         * @tparam F The callback function type. Signature should be compatible with std::function<void(const std::share_ptr<I>& service, const std::shared_ptr<const celix::Properties>& properties)>
         * @param set The callback function which will be called when a new highest ranking service is found or when the last matching service is being removed (svc and properties will be a nullptr).
         * @return The ServiceTrackerBuilder reference for chaining (Fluent API).
         */
        template<typename F>
        ServiceTrackerBuilder& addSetWithPropertiesCallback(F&& set) {
            setCallbacks.emplace_back([set = std::forward<F>(set)](const std::shared_ptr<I>& svc, const std::shared_ptr<const celix::Properties>& props, const std::shared_ptr<const celix::Bundle>&) {
                set(svc, props);
            });
            return *this;
        }

        /**
         * @brief Adds a set callback function, which will be called - on the Celix event thread -
         * when there is a new highest ranking service match.
         * This can can also be an empty match (nullptr).
         *
         * @tparam F The callback function type. Signature should be compatible with std::function<void(const std::share_ptr<I>& service, const std::shared_ptr<const celix::Properties>& properties, const std::shared_ptr<const celix::Bundle>&)>>
         * @param set The callback function which will be called when a new highest ranking service is found or when the last matching service is being removed (svc and properties will be a nullptr).
         * @return The ServiceTrackerBuilder reference for chaining (Fluent API).
         */
        template<typename F>
        ServiceTrackerBuilder& addSetWithOwner(F&& set) {
            setCallbacks.emplace_back(std::forward<F>(set));
            return *this;
        }

        /**
         * @brief "Builds" the service tracker and returns a ServiceTracker.
         *
         * The ServiceTracker will be started async.
         */
        std::shared_ptr<ServiceTracker<I>> build() {
            return ServiceTracker<I>::create(cCtx, std::move(name), std::move(versionRange), std::move(filter), std::move(setCallbacks), std::move(addCallbacks), std::move(remCallbacks));
        }
    private:
        const std::shared_ptr<celix_bundle_context_t> cCtx;
        std::string name;
        celix::Filter filter{};
        std::string versionRange{};
        std::vector<std::function<void(const std::shared_ptr<I>&, const std::shared_ptr<const celix::Properties>&, const std::shared_ptr<const celix::Bundle>&)>> setCallbacks{};
        std::vector<std::function<void(const std::shared_ptr<I>&, const std::shared_ptr<const celix::Properties>&, const std::shared_ptr<const celix::Bundle>&)>> addCallbacks{};
        std::vector<std::function<void(const std::shared_ptr<I>&, const std::shared_ptr<const celix::Properties>&, const std::shared_ptr<const celix::Bundle>&)>> remCallbacks{};
    };

    /**
     * @brief Fluent builder API to track bundles.
     *
     * @see celix::BundleContext::trackBundles for more info.
     * @note Not thread safe.
     */
    class BundleTrackerBuilder {
    private:
        friend class BundleContext;

        //NOTE private to prevent move so that a build() call cannot be forgotten
        BundleTrackerBuilder(BundleTrackerBuilder &&) = default;
    public:
        explicit BundleTrackerBuilder(std::shared_ptr<celix_bundle_context_t> _cCtx) : cCtx{std::move(_cCtx)} {}

        BundleTrackerBuilder &operator=(BundleTrackerBuilder &&) = delete;
        BundleTrackerBuilder(const BundleTrackerBuilder &) = delete;
        BundleTrackerBuilder operator=(const BundleTrackerBuilder &) = delete;

        BundleTrackerBuilder& includeFrameworkBundleInCallback() {
            includeFrameworkBundle = true;
            return *this;
        }

        /**
         * @brief Adds a "on install" callback function, which will be called - on the Celix event thread -
         * when a new bundle has been installed.
         *
         * The "on install" callback function has 1 arguments: A const reference to the installed bundle.
         */
        BundleTrackerBuilder& addOnInstallCallback(std::function<void(const celix::Bundle&)> callback) {
            onInstallCallbacks.push_back(std::move(callback));
            return *this;
        }

        /**
         * @brief Adds a "on start" callback function, which will be called - on the Celix event thread -
         * when a new bundle has been started.
         *
         * The "on start" callback function has 1 arguments: A const reference to the started bundle.
         */
        BundleTrackerBuilder& addOnStartCallback(std::function<void(const celix::Bundle&)> callback) {
            onStartCallbacks.push_back(std::move(callback));
            return *this;
        }

        /**
         * @brief Adds a "on stop" callback function, which will be called - on the Celix event thread -
         * when a new bundle has been stopped.
         *
         * The "on stop" callback function has 1 arguments: A const reference to the stopped bundle.
         */
        BundleTrackerBuilder& addOnStopCallback(std::function<void(const celix::Bundle&)> callback) {
            onStopCallbacks.push_back(std::move(callback));
            return *this;
        }

        /**
         * @brief "Builds" the bundle tracker and returns a BundleTracker.
         *
         * The BundleTracker will be started async.
         */
        std::shared_ptr<BundleTracker> build() {
            return BundleTracker::create(cCtx, includeFrameworkBundle, std::move(onInstallCallbacks), std::move(onStartCallbacks), std::move(onStopCallbacks));
        }
    private:
        const std::shared_ptr<celix_bundle_context_t> cCtx;
        bool includeFrameworkBundle{false};
        std::vector<std::function<void(const celix::Bundle&)>> onInstallCallbacks{};
        std::vector<std::function<void(const celix::Bundle&)>> onStartCallbacks{};
        std::vector<std::function<void(const celix::Bundle&)>> onStopCallbacks{};
    };

    /**
     * @brief Fluent builder API to track service trackers.
     *
     * @see See celix::BundleContext::trackServiceTrackers for more info.
     * @note Not thread safe.
     */
    class MetaTrackerBuilder {
    private:
        friend class BundleContext;

        //NOTE private to prevent move so that a build() call cannot be forgotten
        MetaTrackerBuilder(MetaTrackerBuilder &&) = default;
    public:
        explicit MetaTrackerBuilder(std::shared_ptr<celix_bundle_context_t> _cCtx, std::string _serviceName) :
                cCtx{std::move(_cCtx)},
                serviceName{std::move(_serviceName)} {}

        MetaTrackerBuilder &operator=(MetaTrackerBuilder &&) = delete;
        MetaTrackerBuilder(const MetaTrackerBuilder &) = delete;
        MetaTrackerBuilder operator=(const MetaTrackerBuilder &) = delete;

        /**
         * @brief Adds a "on tracker created" callback function, which will be called - on the Celix event thread -
         * when a new service tracker has been created.
         *
         * The "on tracker created" callback function has 1 arguments: A const reference to a ServiceTrackerInfo object.
         */
        MetaTrackerBuilder& addOnTrackerCreatedCallback(std::function<void(const ServiceTrackerInfo&)> cb) {
            onTrackerCreated.emplace_back(std::move(cb));
            return *this;
        }

        /**
         * @brief Adds a "on tracker destroyed" callback function, which will be called - on the Celix event thread -
         * when a new service tracker has been destroyed.
         *
         * The "on tracker destroyed" callback function has 1 arguments: A const reference to a ServiceTrackerInfo object.
         */
        MetaTrackerBuilder& addOnTrackerDestroyedCallback(std::function<void(const ServiceTrackerInfo&)> cb) {
            onTrackerDestroyed.emplace_back(std::move(cb));
            return *this;
        }

        /**
         * @brief "Builds" the meta tracker and returns a MetaTracker.
         *
         * The MetaTracker will be started async.
         */
        std::shared_ptr<MetaTracker> build() {
            return MetaTracker::create(cCtx, std::move(serviceName), std::move(onTrackerCreated), std::move(onTrackerDestroyed));
        }
    private:
        const std::shared_ptr<celix_bundle_context_t> cCtx;
        std::string serviceName;
        std::vector<std::function<void(const ServiceTrackerInfo&)>> onTrackerCreated{};
        std::vector<std::function<void(const ServiceTrackerInfo&)>> onTrackerDestroyed{};
    };
}