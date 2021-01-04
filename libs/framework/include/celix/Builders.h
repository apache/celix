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
#include <functional>

#include "celix/ServiceRegistration.h"
#include "celix/Trackers.h"

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
        ServiceRegistrationBuilder& addOnRegistered(std::function<void(const std::shared_ptr<ServiceRegistration>&)> callback) {
            onRegisteredCallbacks.emplace_back(std::move(callback));
            return *this;
        }

        /**
         * Adds an on unregistered callback for the service registration.
         * This callback will be called on the Celix event thread when the service is unregistered (UNREGISTERED state)
         */
        ServiceRegistrationBuilder& addOnUnregistered(std::function<void(const std::shared_ptr<ServiceRegistration>&)> callback) {
            onUnregisteredCallbacks.emplace_back(std::move(callback));
            return *this;
        }

        /**
         * "Builds" the service registration and return a ServiceRegistration.
         *
         * The ServiceRegistration will async register the service to the Celix framwork and unregister the service if
         * the ServiceRegistration is destroyed.
         *
         */
        std::shared_ptr<ServiceRegistration> build() {
            auto result = std::make_shared<ServiceRegistration>(cCtx, std::move(svc), std::move(name), std::move(version), std::move(properties), std::move(onRegisteredCallbacks), std::move(onUnregisteredCallbacks));
            result->setSelf(result);
            //TODO after moves , builder is not valid anymore. TBD howto handle this
            return result;
        }
    private:
        const std::shared_ptr<celix_bundle_context_t> cCtx;
        std::shared_ptr<I> svc;
        std::string name;
        std::string version{};
        celix::Properties properties{};
        std::vector<std::function<void(const std::shared_ptr<ServiceRegistration>&)>> onRegisteredCallbacks{};
        std::vector<std::function<void(const std::shared_ptr<ServiceRegistration>&)>> onUnregisteredCallbacks{};
    };

    /**
     * Fluent builder API to use a service(s) of type I.
     *
     * \warning Not thread safe.
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
                useSingleService{_useSingleService} {
        }

        UseServiceBuilder& operator=(UseServiceBuilder&&) = delete;
        UseServiceBuilder(const UseServiceBuilder&) = delete;
        UseServiceBuilder operator=(const UseServiceBuilder&) = delete;

        /**
         * Set filter to be used to select a service.
         * The filter must be LDAP filter.
         * Example: "(property_key=value)"
         */
        UseServiceBuilder& setFilter(std::string f) { filter = celix::Filter{std::move(f)}; return *this; }

        /**
         * Set filter to be used to matching services.
         */
        UseServiceBuilder& setFilter(celix::Filter f) { filter = std::move(f); return *this; }

        /**
         * Sets a optional timeout.
         * If the timeout is > 0 and there is no matching service, the "build" will block
         * until a matching service is found or the timeout is expired.
         *
         * Note: timeout is only valid if the a single service is used.
         *
         */
        template <typename Rep, typename Period>
        UseServiceBuilder& setTimeout(std::chrono::duration<Rep, Period> duration) {
            auto micro =  std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
            timeoutInSeconds = micro / 1000000;
            return *this;
        }

        /**
         * Adds a use callback function which will be called when the UseServiceBuilder is
         * "build".
         *
         * The use callback function has 1 argument: a reference to the matching service.
         */
        UseServiceBuilder& addUseCallback(std::function<void(I&)> cb) {
            callbacks.emplace_back(std::move(cb));
            return *this;
        }

        /**
         * Adds a use callback function which will be called when the UseServiceBuilder is
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
         * Adds a use callback function which will be called when the UseServiceBuilder is
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
         * "Builds" the UseServiceBuild and returns directly if no matching service is found
         * or blocks until:
         *  - All the use callback functions are called with the highest ranking matching service
         *  - The timeout has expired
         *
         * \returns the number of service called (1 or 0).
         */
        std::size_t build() {
            celix_service_use_options_t opts{};
            opts.filter.serviceName = name.empty() ? nullptr : name.c_str();
            opts.filter.ignoreServiceLanguage = true;
            opts.filter.filter = filter.empty() ? nullptr : filter.getFilterCString();
            opts.filter.versionRange = versionRange.empty() ? nullptr : versionRange.c_str();
            opts.waitTimeoutInSeconds = timeoutInSeconds;
            opts.callbackHandle = this;
            opts.useWithOwner = [](void* data, void *voidSvc, const celix_properties_t* cProps, const celix_bundle_t* cBnd) {
                auto* builder = static_cast<UseServiceBuilder<I>*>(data);
                auto* svc = static_cast<I*>(voidSvc);
                const Bundle bnd = Bundle{const_cast<celix_bundle_t*>(cBnd)};
                auto props = celix::Properties::wrap(cProps);
                for (const auto& func : builder->callbacks) {
                    func(*svc);
                }
                for (const auto& func : builder->callbacksWithProperties) {
                    func(*svc, *props);
                }
                for (const auto& func : builder->callbacksWithOwner) {
                    func(*svc, *props, bnd);
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

    /**
     * Fluent builder API to track services of type I.
     *
     * \warning Not thread safe.
     * @tparam I
     */
    template<typename I>
    class ServiceTrackerBuilder {
    private:
        friend class BundleContext;

        //NOTE private to prevent move so that a build() call cannot be forgotten
        ServiceTrackerBuilder(ServiceTrackerBuilder&&) = default;
    public:
        explicit ServiceTrackerBuilder(std::shared_ptr<celix_bundle_context_t> _cCtx, std::string _name) :
                cCtx{std::move(_cCtx)},
                name{std::move(_name)} {}

        ServiceTrackerBuilder& operator=(ServiceTrackerBuilder&&) = delete;
        ServiceTrackerBuilder(const ServiceTrackerBuilder&) = delete;
        ServiceTrackerBuilder operator=(const ServiceTrackerBuilder&) = delete;

        /**
         * Set filter to be used to matching services.
         * The filter must be LDAP filter.
         * Example: "(property_key=value)"
         */
        ServiceTrackerBuilder& setFilter(std::string f) { filter = celix::Filter{std::move(f)}; return *this; }

        /**
         * Set filter to be used to matching services.
         */
        ServiceTrackerBuilder& setFilter(celix::Filter f) { filter = std::move(f); return *this; }

        /**
         * Adds a add callback function, which will be called - on the Celix event thread -
         * when a new service match is found.
         *
         * The add callback function has 1 argument: A shared ptr to the added service.
         */
        ServiceTrackerBuilder& addAddCallback(std::function<void(std::shared_ptr<I>)> add) {
            addCallbacks.emplace_back([add](std::shared_ptr<I> svc, std::shared_ptr<const celix::Properties>, const celix::Bundle&) {
                add(svc);
            });
            return *this;
        }

        /**
         * Adds a add callback function, which will be called - on the Celix event thread -
         * when a new service match is found.
         *
         * The add callback function has 2 arguments:
         *  - A shared ptr to the added service.
         *  - A shared ptr to the added service properties.
         */
        ServiceTrackerBuilder& addAddWithPropertiesCallback(std::function<void(std::shared_ptr<I>, std::shared_ptr<const celix::Properties>)> add) {
            addCallbacks.emplace_back([add](std::shared_ptr<I> svc, std::shared_ptr<const celix::Properties> props, const celix::Bundle&) {
                add(svc, props);
            });
            return *this;
        }

        /**
         * Adds a add callback function, which will be called - on the Celix event thread -
         * when a new service match is found.
         *
         * The add callback function has 3 arguments:
         *  - A shared ptr to the added service.
         *  - A shared ptr to the added service properties.
         *  - A shared ptr to the bundle owning the added service.
         */
        ServiceTrackerBuilder& addAddWithOwnerCallback(std::function<void(std::shared_ptr<I>, std::shared_ptr<const celix::Properties>, const celix::Bundle&)> add) {
            addCallbacks.emplace_back([add](std::shared_ptr<I> svc, std::shared_ptr<const celix::Properties> props, const celix::Bundle& bnd) {
                add(svc, props, bnd);
            });
            return *this;
        }

        /**
         * Adds a remove callback function, which will be called - on the Celix event thread -
         * when a service match is being removed.
         *
         * The remove callback function has 1 arguments: A shared ptr to the removing service.
         */
        ServiceTrackerBuilder& addRemCallback(std::function<void(std::shared_ptr<I>)> remove) {
            remCallbacks.emplace_back([remove](std::shared_ptr<I> svc, std::shared_ptr<const celix::Properties>, const celix::Bundle&) {
                remove(svc);
            });
            return *this;
        }

        /**
         * Adds a remove callback function, which will be called - on the Celix event thread -
         * when a service match is being removed.
         *
         * The remove callback function has 2 arguments:
         *  - A shared ptr to the removing service.
         *  - A shared ptr to the removing service properties.
         */
        ServiceTrackerBuilder& addRemWithPropertiesCallback(std::function<void(std::shared_ptr<I>, std::shared_ptr<const celix::Properties>)> remove) {
            remCallbacks.emplace_back([remove](std::shared_ptr<I> svc, std::shared_ptr<const celix::Properties> props, const celix::Bundle&) {
                remove(svc, props);
            });
            return *this;
        }

        /**
         * Adds a remove callback function, which will be called - on the Celix event thread -
         * when a service match is being removed.
         *
         * The remove callback function has 3 arguments:
         *  - A shared ptr to the removing service.
         *  - A shared ptr to the removing service properties.
         *  - A shared ptr to the bundle owning the removing service.
         */
        ServiceTrackerBuilder& addRemWithOwnerCallback(std::function<void(std::shared_ptr<I>, std::shared_ptr<const celix::Properties>, const celix::Bundle&)> remove) {
            remCallbacks.emplace_back([remove](std::shared_ptr<I> svc, std::shared_ptr<const celix::Properties> props, const celix::Bundle& bnd) {
                remove(svc, props, bnd);
            });
            return *this;
        }

        /**
         * Adds a set callback function, which will be called - on the Celix event thread -
         * when there is a new highest ranking service match.
         * This can can also be an empty match (nullptr).
         *
         * The set callback function has 2 arguments: A shared ptr to the highest
         * ranking service match or nullptr.
         */
        ServiceTrackerBuilder& addSetCallback(std::function<void(std::shared_ptr<I>)> set) {
            setCallbacks.emplace_back([set](std::shared_ptr<I> svc, std::shared_ptr<const celix::Properties>, std::shared_ptr<const celix::Bundle>) {
                set(std::move(svc));
            });
            return *this;
        }

        /**
         * Adds a set callback function, which will be called - on the Celix event thread -
         * when there is a new highest ranking service match.
         * This can can also be an empty match (nullptr).
         *
         * The set callback function has 2 arguments:
         *  - A shared ptr to the highest ranking service match or nullptr.
         *  - A const shared ptr to the set service properties or nullptr (if the service is nullptr).
         */
        ServiceTrackerBuilder& addSetWithPropertiesCallback(std::function<void(std::shared_ptr<I>, std::shared_ptr<const celix::Properties>)> set) {
            setCallbacks.emplace_back([set](std::shared_ptr<I> svc, std::shared_ptr<const celix::Properties> props, std::shared_ptr<const celix::Bundle>) {
                set(std::move(svc), std::move(props));
            });
            return *this;
        }

        /**
         * Adds a set callback function, which will be called - on the Celix event thread -
         * when there is a new highest ranking service match.
         * This can can also be an empty match (nullptr).
         *
         * The set callback function has 3 arguments:
         *  - A shared ptr to the highest ranking service match or nullptr.
         *  - A const shared ptr to the set service properties or nullptr (if the service is nullptr).
         *  - A const shared ptr to the bundle owning the set service or nullptr (if the service is nullptr).
         */
        ServiceTrackerBuilder& addSetWithOwner(std::function<void(std::shared_ptr<I>, std::shared_ptr<const celix::Properties>, std::shared_ptr<const celix::Bundle>)> set) {
            setCallbacks.emplace_back(std::move(set));
            return *this;
        }

//        /**
//         * TODO
//         */
//        ServiceTrackerBuilder& addUpdateCallback(std::function<void(std::vector<std::shared_ptr<I>>)> update) {
//            //TODO update -> vector of ordered (svc rank) services
//            return *this;
//        }

        //TODO add function to register done call backs -> addOnStarted / addOnStopped (inheritance?)

        /**
         * "Builds" the service tracker and returns a ServiceTracker.
         *
         * The ServiceTracker will be started async.
         */
        std::shared_ptr<ServiceTracker<I>> build() {
            auto tracker = std::make_shared<ServiceTracker<I>>(cCtx, std::move(name), std::move(versionRange), std::move(filter), std::move(setCallbacks), std::move(addCallbacks), std::move(remCallbacks));
            tracker->open();
            //TODO after moves , builder is not valid anymore. TBD howto handle this
            return tracker;
        }
    private:
        const std::shared_ptr<celix_bundle_context_t> cCtx;
        std::string name;
        celix::Filter filter{};
        std::string versionRange{};
        std::vector<std::function<void(std::shared_ptr<I>, std::shared_ptr<const celix::Properties>, std::shared_ptr<const celix::Bundle>)>> setCallbacks{};
        std::vector<std::function<void(std::shared_ptr<I>, std::shared_ptr<const celix::Properties>, const celix::Bundle&)>> addCallbacks{}; //TODO make bundle arg std::shared_ptr
        std::vector<std::function<void(std::shared_ptr<I>, std::shared_ptr<const celix::Properties>, const celix::Bundle&)>> remCallbacks{}; //TODO make bundle arg std::shared_ptr
    };

    /**
     * Fluent builder API to track bundles.
     * \warning Not thread safe.
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
         * Adds a "on install" callback function, which will be called - on the Celix event thread -
         * when a new bundle has been installed.
         *
         * The "on install" callback function has 1 arguments: A const reference to the installed bundle.
         */
        BundleTrackerBuilder& addOnInstallCallback(std::function<void(const celix::Bundle&)> callback) {
            onInstallCallbacks.push_back(std::move(callback));
            return *this;
        }

        /**
         * Adds a "on start" callback function, which will be called - on the Celix event thread -
         * when a new bundle has been started.
         *
         * The "on start" callback function has 1 arguments: A const reference to the started bundle.
         */
        BundleTrackerBuilder& addOnStartCallback(std::function<void(const celix::Bundle&)> callback) {
            onStartCallbacks.push_back(std::move(callback));
            return *this;
        }

        /**
         * Adds a "on stop" callback function, which will be called - on the Celix event thread -
         * when a new bundle has been stopped.
         *
         * The "on stop" callback function has 1 arguments: A const reference to the stopped bundle.
         */
        BundleTrackerBuilder& addOnStopCallback(std::function<void(const celix::Bundle&)> callback) {
            onStopCallbacks.push_back(std::move(callback));
            return *this;
        }

        /**
         * "Builds" the bundle tracker and returns a BundleTracker.
         *
         * The BundleTracker will be started async.
         */
        std::shared_ptr<BundleTracker> build() {
            auto tracker = std::make_shared<BundleTracker>(cCtx, includeFrameworkBundle, std::move(onInstallCallbacks), std::move(onStartCallbacks), std::move(onStopCallbacks));
            tracker->open();
            return tracker;
        }
    private:
        const std::shared_ptr<celix_bundle_context_t> cCtx;
        bool includeFrameworkBundle{false};
        std::vector<std::function<void(const celix::Bundle&)>> onInstallCallbacks{};
        std::vector<std::function<void(const celix::Bundle&)>> onStartCallbacks{};
        std::vector<std::function<void(const celix::Bundle&)>> onStopCallbacks{};
    };

    /**
     * Fluent builder API to track service trackers.
     *
     * \warning Not thread safe.
     */
    class MetaTrackerBuilder {
    private:
        friend class BundleContext;

        //NOTE private to prevent move so that a build() call cannot be forgotten
        MetaTrackerBuilder(MetaTrackerBuilder &&) = default;
    public:
        explicit MetaTrackerBuilder(std::shared_ptr<celix_bundle_context_t> _cCtx, std::string _serviceName) :
                cCtx{std::move(_cCtx)},
                serviceName{std::move(_serviceName)}
                {}

        MetaTrackerBuilder &operator=(MetaTrackerBuilder &&) = delete;
        MetaTrackerBuilder(const MetaTrackerBuilder &) = delete;
        MetaTrackerBuilder operator=(const MetaTrackerBuilder &) = delete;

        /**
         * Adds a "on tracker created" callback function, which will be called - on the Celix event thread -
         * when a new service tracker has been created.
         *
         * The "on tracker created" callback function has 1 arguments: A const reference to a ServiceTrackerInfo object.
         */
        MetaTrackerBuilder& addOnTrackerCreatedCallback(std::function<void(const ServiceTrackerInfo&)> cb) {
            onTrackerCreated.emplace_back(std::move(cb));
            return *this;
        }

        /**
         * Adds a "on tracker destroyed" callback function, which will be called - on the Celix event thread -
         * when a new service tracker has been destroyed.
         *
         * The "on tracker destroyed" callback function has 1 arguments: A const reference to a ServiceTrackerInfo object.
         */
        MetaTrackerBuilder& addOnTrackerDestroyedCallback(std::function<void(const ServiceTrackerInfo&)> cb) {
            onTrackerDestroyed.emplace_back(std::move(cb));
            return *this;
        }

        /**
         * "Builds" the meta tracker and returns a MetaTracker.
         *
         * The MetaTracker will be started async.
         */
        std::shared_ptr<MetaTracker> build() {
            auto tracker = std::make_shared<MetaTracker>(cCtx, std::move(serviceName), std::move(onTrackerCreated), std::move(onTrackerDestroyed));
            tracker->open();
            return tracker;
        }
    private:
        const std::shared_ptr<celix_bundle_context_t> cCtx;
        std::string serviceName;
        std::vector<std::function<void(const ServiceTrackerInfo&)>> onTrackerCreated{};
        std::vector<std::function<void(const ServiceTrackerInfo&)>> onTrackerDestroyed{};
    };
}