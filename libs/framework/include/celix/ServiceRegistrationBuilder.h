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

#include "celix/Utils.h"
#include "celix/ServiceRegistration.h"

namespace celix {
    /**
     * @brief Fluent builder API to build a new service registration for a service.
     *
     * @see celix::BundleContext::registerService for more info.
     * @tparam I The service type (Note should be the abstract interface, not the interface implementer)
     * @note Not thread safe.
     */
    template<typename I>
    class ServiceRegistrationBuilder {
    private:
        friend class BundleContext;

        //NOTE private to prevent move so that a build() call cannot be forgotten
        ServiceRegistrationBuilder(ServiceRegistrationBuilder&&) noexcept = default;
    public:
        ServiceRegistrationBuilder(
                std::shared_ptr<celix_bundle_context_t> _cCtx,
                std::shared_ptr<I> _svc,
                std::string _name,
                bool _registerAsync = true,
                bool _unregisterAsync = true) :
                cCtx{std::move(_cCtx)},
                svc{std::move(_svc)},
                name{std::move(_name)},
                version{celix::typeVersion<I>()},
                registerAsync{_registerAsync},
                unregisterAsync{_unregisterAsync} {}

        ServiceRegistrationBuilder& operator=(ServiceRegistrationBuilder&&) = delete;
        ServiceRegistrationBuilder(const ServiceRegistrationBuilder&) = delete;
        ServiceRegistrationBuilder operator=(const ServiceRegistrationBuilder&) = delete;

        /**
         * @brief Set the service version.
         *
         * This will lead to a 'service.version' service property.
         */
#if __cplusplus >= 201703L //C++17 or higher
        ServiceRegistrationBuilder& setVersion(std::string_view v) { version = v; return *this; }
#else
        ServiceRegistrationBuilder& setVersion(std::string v) { version = std::move(v); return *this; }
#endif

        /**
         * @brief Add a property to the service properties.
         *
         * If a key is already present the value will be overridden.
         */
#if __cplusplus >= 201703L //C++17 or higher
        template<typename T>
        ServiceRegistrationBuilder& addProperty(std::string_view key, T&& value) { properties.template set(key, std::forward<T>(value)); return *this; }
#else
        template<typename T>
        ServiceRegistrationBuilder& addProperty(const std::string& key, T&& value) { properties.set(key, std::forward<T>(value)); return *this; }
#endif

        /**
         * @brief Set the service properties.
         *
         * Note this call will clear already set properties.
         */
        ServiceRegistrationBuilder& setProperties(celix::Properties p) { properties = std::move(p); return *this; }

        /**
         * @brief Add the properties to the service properties.
         *
         * Note this call will add these properties to the already set properties.
         * If a key is already present the value will be overridden.
         */
        ServiceRegistrationBuilder& addProperties(const celix::Properties& props) {
            for (const auto& pair : props) {
                properties.set(pair.first, pair.second);
            }
            return *this;
        }

        /**
         * @brief Adds an on registered callback for the service registration.
         *
         * This callback will be called on the Celix event thread when the service is registered (REGISTERED state)
         */
        ServiceRegistrationBuilder& addOnRegistered(std::function<void(ServiceRegistration&)> callback) {
            onRegisteredCallbacks.emplace_back(std::move(callback));
            return *this;
        }

        /**
         * @brief Adds an on unregistered callback for the service registration.
         *
         * This callback will be called on the Celix event thread when the service is unregistered (UNREGISTERED state)
         */
        ServiceRegistrationBuilder& addOnUnregistered(std::function<void(ServiceRegistration&)> callback) {
            onUnregisteredCallbacks.emplace_back(std::move(callback));
            return *this;
        }

        /**
         * @brief Configure if the service registration will be done synchronized or asynchronized.
         *
         * When a service registration is done synchronized the underlining service will be registered in the Celix
         * framework (and all service trackers will have been informed) when the build() returns.
         * For asynchronized the build() will not wait until this is done.
         *
         * The benefit of synchronized is the user is ensured a service registration is completely done, but extra
         * care has te be taking into account to prevent deadlocks:
         * lock object -> register service -> trigger service tracker -> trying to update a locked object.
         *
         * The benefit of asynchronized is that service registration can safely be done inside a lock.
         *
         * Default behavior is asynchronized.
         */
        ServiceRegistrationBuilder& setRegisterAsync(bool async) {
            registerAsync = async;
            return *this;
        }

        /**
         * @brief Configure if the service un-registration will be done synchronized or asynchronized.
         *
         * When a service un-registration is done synchronized the underlining service will be unregistered in the Celix
         * framework (and all service trackers will have been informed) when the ServiceRegistration::unregister is
         * called of when the ServiceRegistration goes out of scope (with a still REGISTERED service).
         * For asynchronized this will be done asynchronously and the actual ServiceRegistration object will only be
         * deleted if the un-registration is completed async.
         *
         * The benefit of synchronized is the user is ensured a service un-registration is completely done when
         * calling unregister() / letting ServiceRegistration out of scope, but extra
         * care has te be taking into account to prevent deadlocks:
         * lock object -> unregister service -> trigger service tracker -> trying to update a locked object.
         *
         * The benefit of asynchronized is that service un-registration can safely be called inside a lock.
         *
         * Default behavior is asynchronized.
         */
        ServiceRegistrationBuilder& setUnregisterAsync(bool async) {
            unregisterAsync = async;
            return *this;
        }

        /**
         * @brief "Builds" the service registration and return a ServiceRegistration.
         *
         * The ServiceRegistration will register the service to the Celix framework and unregister the service if
         * the shared ptr for the ServiceRegistration goes out of scope.
         *
         */
        std::shared_ptr<ServiceRegistration> build() {
            return ServiceRegistration::create(
                    cCtx,
                    std::move(svc),
                    std::move(name),
                    std::move(version),
                    std::move(properties),
                    registerAsync,
                    unregisterAsync,
                    std::move(onRegisteredCallbacks),
                    std::move(onUnregisteredCallbacks));
        }
    private:
        const std::shared_ptr<celix_bundle_context_t> cCtx;
        std::shared_ptr<I> svc;
        std::string name;
        std::string version;
        bool registerAsync;
        bool unregisterAsync;
        celix::Properties properties{};
        std::vector<std::function<void(ServiceRegistration&)>> onRegisteredCallbacks{};
        std::vector<std::function<void(ServiceRegistration&)>> onUnregisteredCallbacks{};
    };
}