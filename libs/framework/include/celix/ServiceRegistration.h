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
#include <mutex>
#include <vector>
#include <functional>

#include "celix/Properties.h"
#include "celix_bundle_context.h"

namespace celix {

    enum class ServiceRegistrationState {
        REGISTERING,
        REGISTERED,
        UNREGISTERING,
        UNREGISTERED
    };

    class ServiceRegistration;

    /**
     * TODO
     * \note Thread safe.
     */
    class ServiceRegistration  {
    public:
        ServiceRegistration(
                std::shared_ptr<celix_bundle_context_t> _cCtx,
                std::shared_ptr<void> _svc,
                std::string _name,
                std::string _version,
                celix::Properties _properties,
                std::vector<std::function<void(const std::shared_ptr<ServiceRegistration>&)>> _onRegisteredCallbacks,
                std::vector<std::function<void(const std::shared_ptr<ServiceRegistration>&)>> _onUnregisteredCallbacks) :
                cCtx{std::move(_cCtx)},
                svc{std::move(_svc)},
                name{std::move(_name)},
                version{std::move(_version)},
                properties{std::move(_properties)},
                onRegisteredCallbacks{std::move(_onRegisteredCallbacks)},
                onUnregisteredCallbacks{std::move(_onUnregisteredCallbacks)} {

            //setup registration using C api.
            auto* cProps = celix_properties_copy(properties.getCProperties());
            celix_service_registration_options_t opts{};
            opts.svc = svc.get();
            opts.serviceName = name.c_str();
            opts.properties = cProps;
            if (!version.empty()) {
                opts.serviceVersion = version.c_str();
            }
            opts.asyncData = static_cast<void*>(this);
            opts.asyncCallback = [](void *data, long /*svcId*/) {
                auto *reg = static_cast<ServiceRegistration *>(data);
                {
                    std::lock_guard<std::mutex> lck{reg->mutex};
                    reg->state = ServiceRegistrationState::REGISTERED;
                }
                auto regAsSharedPtr = reg->getSelf();
                for (const auto &cb : reg->onRegisteredCallbacks) {
                    cb(regAsSharedPtr);
                }
            };
            std::lock_guard<std::mutex> lck{mutex};
            svcId = celix_bundleContext_registerServiceWithOptionsAsync(cCtx.get(), &opts);
        }

        ~ServiceRegistration() noexcept {
            wait(); //if state is REGISTERING, wait till registration is complete.
            unregister(); //if state is REGISTERED, queue unregistration.
            wait(); //if state is UNREGISTERING, wait till unregistration is complete.
        }

        const std::string& getServiceName() const { return name; }
        const std::string& getServiceVersion() const { return version; }
        const celix::Properties& getServiceProperties() const { return properties; }

        ServiceRegistrationState getCurrentState() const {
            std::lock_guard<std::mutex> lck{mutex};
            return state;
        }

        long getServiceId() const {
            std::lock_guard<std::mutex> lck{mutex};
            return svcId;
        }

        void wait() const {
            bool needWaitUnregistering = false;
            bool needWaitRegistering = false;
            long localId;
            {
                std::lock_guard<std::mutex> lck{mutex};
                localId = svcId;
                if (state == ServiceRegistrationState::REGISTERING) {
                    needWaitRegistering = true;
                } else if (state == ServiceRegistrationState::UNREGISTERING) {
                    needWaitUnregistering = true;
                }
            }
            if (needWaitRegistering) {
                celix_bundleContext_waitForAsyncRegistration(cCtx.get(), localId);
            }
            if (needWaitUnregistering) {
                celix_bundleContext_waitForAsyncUnregistration(cCtx.get(), localId);
            }
        }

        void unregister() {
            wait(); //if state is REGISTERING, wait till registration is complete.
            std::lock_guard<std::mutex> lck{mutex};
            if (state == ServiceRegistrationState::REGISTERED) {
                //not yet unregistering
                state = ServiceRegistrationState::UNREGISTERING;

                //NOTE: As long this unregister event is in the queue, the ServiceRegistration will wait in its dtor
                celix_bundleContext_unregisterServiceAsync(cCtx.get(), svcId, this, [](void *data) {
                    auto reg = static_cast<ServiceRegistration*>(data);
                    {
                        std::lock_guard<std::mutex> lck{reg->mutex};
                        reg->state = ServiceRegistrationState::UNREGISTERED;
                    }
                    auto regAsSharedPtr = reg->getSelf();
                    for (const auto &cb : reg->onUnregisteredCallbacks) {
                        cb(regAsSharedPtr);
                    }
                });
            }
        }

        void setSelf(const std::shared_ptr<ServiceRegistration>& s) {
            std::lock_guard<std::mutex> lck{mutex};
            self = s;
        }

        std::shared_ptr<ServiceRegistration> getSelf() const {
            std::lock_guard<std::mutex> lck{mutex};
            return self.lock();
        }

    private:
        const std::shared_ptr<celix_bundle_context_t> cCtx;
        const std::shared_ptr<void> svc;
        const std::string name;
        const std::string version;
        const celix::Properties properties;
        const std::vector<std::function<void(const std::shared_ptr<ServiceRegistration>&)>> onRegisteredCallbacks;
        const std::vector<std::function<void(const std::shared_ptr<ServiceRegistration>&)>> onUnregisteredCallbacks;

        mutable std::mutex mutex{}; //protects below
        long svcId{-1};
        ServiceRegistrationState state{ServiceRegistrationState::REGISTERING};
        std::weak_ptr<ServiceRegistration> self{}; //weak ptr to self, so that callbacks can receive a shared ptr
    };

}