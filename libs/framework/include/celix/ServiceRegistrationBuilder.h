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

#include "celix/ServiceRegistration.h"

namespace celix {
    /**
     * Fluent builder API to build a new service registration for a service of type I.
     *
     * \warning Not thread safe.
     * @tparam I The service type (Note should be the abstract interface, not the interface implementer)
     */
    template<typename I>
    class ServiceRegistrationBuilder {
    private:
        friend class BundleContext;

        //NOTE private to prevent move so that a build() call cannot be forgotten
        ServiceRegistrationBuilder(ServiceRegistrationBuilder&&) = default;
    public:
        ServiceRegistrationBuilder(std::shared_ptr<celix_bundle_context_t> _cCtx, std::shared_ptr<I> _svc, std::string _name) :
                cCtx{std::move(_cCtx)},
                svc{std::move(_svc)},
                name{std::move(_name)} {

        }

        ServiceRegistrationBuilder& operator=(ServiceRegistrationBuilder&&) = delete;
        ServiceRegistrationBuilder(const ServiceRegistrationBuilder&) = delete;
        ServiceRegistrationBuilder operator=(const ServiceRegistrationBuilder&) = delete;

        /**
         * Set the service version.
         * This will lead to a 'service.version' service property.
         */
        ServiceRegistrationBuilder& setVersion(std::string v) { version = std::move(v); return *this; }

        /**
         * Add a property to the service properties.
         * If a key is already present the value will be overridden.
         */
        ServiceRegistrationBuilder& addProperty(std::string key, std::string value) { properties.set(std::move(key), std::move(value)); return *this; }

        /**
         * Add a property to the service properties.
         * If a key is already present the value will be overridden.
         */
        ServiceRegistrationBuilder& addProperty(std::string key, const char* value) { properties.set(std::move(key), std::string{value}); return *this; }

        /**
         * Add a property to the service properties.
         * If a key is already present the value will be overridden.
         */
        template<typename T>
        ServiceRegistrationBuilder& addProperty(std::string key, T value) { properties.set(std::move(key), std::to_string(std::move(value))); return *this; }

        /**
         * Set the service properties.
         * Note this call will clear any already set properties.
         */
        ServiceRegistrationBuilder& setProperties(celix::Properties p) { properties = std::move(p); return *this; }

        /**
         * Add the properties to the service properties.
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
         * Adds an on registered callback for the service registration.
         * This callback will be called on the Celix event thread when the service is registered (REGISTERED state)
         */
        ServiceRegistrationBuilder& addOnRegistered(std::function<void(ServiceRegistration&)> callback) {
            onRegisteredCallbacks.emplace_back(std::move(callback));
            return *this;
        }

        /**
         * Adds an on unregistered callback for the service registration.
         * This callback will be called on the Celix event thread when the service is unregistered (UNREGISTERED state)
         */
        ServiceRegistrationBuilder& addOnUnregistered(std::function<void(ServiceRegistration&)> callback) {
            onUnregisteredCallbacks.emplace_back(std::move(callback));
            return *this;
        }

        /**
         * Configure if the service registration will be done synchronized or asynchronized.
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
         * Configure if the service un-registration will be done synchronized or asynchronized.
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
         * "Builds" the service registration and return a ServiceRegistration.
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
        std::string version{};
        celix::Properties properties{};
        bool registerAsync{true};
        bool unregisterAsync{true};
        std::vector<std::function<void(ServiceRegistration&)>> onRegisteredCallbacks{};
        std::vector<std::function<void(ServiceRegistration&)>> onUnregisteredCallbacks{};
    };
}