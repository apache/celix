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

#include "celix/Constants.h"
#include "celix/Properties.h"
#include "celix_bundle_context.h"
#include "celix_bundle.h"
#include "celix_framework.h"

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
        static std::shared_ptr<ServiceRegistration> create(std::shared_ptr<celix_bundle_context_t> cCtx,
                                                           std::shared_ptr<void> svc,
                                                           std::string name,
                                                           std::string version,
                                                           celix::Properties properties,
                                                           bool registerAsync,
                                                           bool unregisterAsync,
                                                           std::vector<std::function<void(ServiceRegistration&)>> onRegisteredCallbacks,
                                                           std::vector<std::function<void(ServiceRegistration&)>> onUnregisteredCallbacks) {
            auto delCallback = [](ServiceRegistration* reg) {
                if (reg->getState() == ServiceRegistrationState::UNREGISTERED) {
                    delete reg;
                } else {
                    /*
                     * if not registered/unregistering -> unregister() -> new event on the Celix event thread
                     * if unregistering -> nop unregister() -> there is already a event on the Celix event thread to unregister
                     */
                    reg->unregister();

                    /*
                     * Creating event on the Event loop, this will be after the unregistration is done
                     */
                    auto* fw = celix_bundleContext_getFramework(reg->cCtx.get());
                    auto* bnd = celix_bundleContext_getBundle(reg->cCtx.get());
                    long bndId = celix_bundle_getId(bnd);
                    celix_framework_fireGenericEvent(
                            fw,
                            -1,
                            bndId,
                            "celix::ServiceRegistration delete callback",
                            reg,
                            [](void *data) {
                                auto* r = static_cast<ServiceRegistration*>(data);
                                delete r;
                            },
                            nullptr,
                            nullptr);
                }
            };

            auto reg = std::shared_ptr<ServiceRegistration>{
                new ServiceRegistration{
                    std::move(cCtx),
                    std::move(svc),
                    std::move(name),
                    std::move(version),
                    std::move(properties),
                    registerAsync,
                    unregisterAsync,
                    std::move(onRegisteredCallbacks),
                    std::move(onUnregisteredCallbacks)},
                delCallback
            };
            reg->setSelf(reg);
            reg->registerService();
            return reg;
        }

        const std::string& getServiceName() const { return name; }
        const std::string& getServiceVersion() const { return version; }
        const celix::Properties& getServiceProperties() const { return properties; }

        ServiceRegistrationState getState() const {
            std::lock_guard<std::mutex> lck{mutex};
            return state;
        }

        long getServiceId() const {
            std::lock_guard<std::mutex> lck{mutex};
            return svcId;
        }

        long getServiceRanking() const {
            return properties.getAsLong(celix::SERVICE_RANKING, 0);
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

        /**
         * Unregister the managed service from the Celix framework if the state is REGISTERED.
         * If ServiceRegistration::unregisterAsync is true still will be done async, if not this will be done
         * synchronized.
         */
        void unregister() {
            if (unregisterAsync) {
                std::lock_guard<std::mutex> lck{mutex};
                if (state == ServiceRegistrationState::REGISTERED || state == ServiceRegistrationState::REGISTERING) {
                    //not yet unregistering
                    state = ServiceRegistrationState::UNREGISTERING;

                    //NOTE: As long this unregister event is in the queue, the ServiceRegistration will wait in its dtor
                    celix_bundleContext_unregisterServiceAsync(cCtx.get(), svcId, this, [](void *data) {
                        auto reg = static_cast<ServiceRegistration*>(data);
                        {
                            std::lock_guard<std::mutex> lck{reg->mutex};
                            reg->state = ServiceRegistrationState::UNREGISTERED;
                        }
                        for (const auto &cb : reg->onUnregisteredCallbacks) {
                            cb(*reg);
                        }
                    });
                }
            } else /*sync*/ {
                long localSvcId = -1;
                {
                    std::lock_guard<std::mutex> lck{mutex};
                    if (state == ServiceRegistrationState::REGISTERED || state == ServiceRegistrationState::REGISTERING) {
                        state = ServiceRegistrationState::UNREGISTERING;
                        localSvcId = svcId;
                        svcId = -1;
                    }
                }
                if (localSvcId >= 0) {
                    celix_bundleContext_unregisterService(cCtx.get(), localSvcId);
                    {
                        std::lock_guard<std::mutex> lck{mutex};
                        state = ServiceRegistrationState::UNREGISTERED;
                    }
                    for (const auto& cb: onUnregisteredCallbacks) {
                        cb(*this);
                    }
                }
            }
        }

        /**
         * Returns the shared_ptr of for this object.
         * This method can return null when the ServiceRegistration in UNREGISTERED.
         */
        std::shared_ptr<ServiceRegistration> getSelf() const {
            std::lock_guard<std::mutex> lck{mutex};
            return self.lock();
        }

    private:
        /**
         * Private ctor, use static create method to create a shared_ptr for this object.
         */
        ServiceRegistration(
                std::shared_ptr<celix_bundle_context_t> _cCtx,
                std::shared_ptr<void> _svc,
                std::string _name,
                std::string _version,
                celix::Properties _properties,
                bool _registerAsync,
                bool _unregisterAsync,
                std::vector<std::function<void(ServiceRegistration&)>> _onRegisteredCallbacks,
                std::vector<std::function<void(ServiceRegistration&)>> _onUnregisteredCallbacks) :
                cCtx{std::move(_cCtx)},
                svc{std::move(_svc)},
                name{std::move(_name)},
                version{std::move(_version)},
                properties{std::move(_properties)},
                registerAsync{_registerAsync},
                unregisterAsync{_unregisterAsync},
                onRegisteredCallbacks{std::move(_onRegisteredCallbacks)},
                onUnregisteredCallbacks{std::move(_onUnregisteredCallbacks)} {}


        /**
         * Register service in the Celix framework.
         * This is done async if ServiceRegistration::registerAsync is true and sync otherwise.
         *
         * Note that 'register' is a keyword in C and that is why this method is called
         * registerService instead of register with would match the unregister method.
         */
        void registerService() {
            //setup registration using C api.
            auto* cProps = celix_properties_copy(properties.getCProperties());
            celix_service_registration_options_t opts{};
            opts.svc = svc.get();
            opts.serviceName = name.c_str();
            opts.properties = cProps;
            if (!version.empty()) {
                opts.serviceVersion = version.c_str();
            }

            if (registerAsync) {
                opts.asyncData = static_cast<void*>(this);
                opts.asyncCallback = [](void *data, long /*svcId*/) {
                    auto *reg = static_cast<ServiceRegistration *>(data);
                    {
                        std::lock_guard<std::mutex> lck{reg->mutex};
                        reg->state = ServiceRegistrationState::REGISTERED;
                    }
                    for (const auto &cb : reg->onRegisteredCallbacks) {
                        cb(*reg);
                    }
                };
                std::lock_guard<std::mutex> lck{mutex};
                svcId = celix_bundleContext_registerServiceWithOptionsAsync(cCtx.get(), &opts);
            } else /*sync*/ {
                long localSvcId = celix_bundleContext_registerServiceWithOptions(cCtx.get(), &opts);
                {
                    std::lock_guard<std::mutex> lck{mutex};
                    svcId = localSvcId;
                    state = ServiceRegistrationState::REGISTERED;
                }
                for (const auto& cb: onRegisteredCallbacks) {
                    cb(*this);
                }
            }
        }

        /**
         * Ensure this this object can create a std::shared_ptr to it self.
         */
        void setSelf(const std::shared_ptr<ServiceRegistration>& s) {
            std::lock_guard<std::mutex> lck{mutex};
            self = s;
        }

        const std::shared_ptr<celix_bundle_context_t> cCtx;
        const std::shared_ptr<void> svc;
        const std::string name;
        const std::string version;
        const celix::Properties properties;
        const bool registerAsync;
        const bool unregisterAsync;
        const std::vector<std::function<void(ServiceRegistration&)>> onRegisteredCallbacks;
        const std::vector<std::function<void(ServiceRegistration&)>> onUnregisteredCallbacks;

        mutable std::mutex mutex{}; //protects below
        long svcId{-1};
        ServiceRegistrationState state{ServiceRegistrationState::REGISTERING};
        std::weak_ptr<ServiceRegistration> self{}; //weak ptr to self, so that callbacks can receive a shared ptr
    };

}