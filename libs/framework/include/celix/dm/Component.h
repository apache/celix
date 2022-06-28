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

#include <map>
#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <memory>
#include <iostream>
#include <type_traits>
#include <algorithm>
#include <cstdio>

#include "dm_component.h"
#include "celix/dm/types.h"
#include "celix/dm/ServiceDependency.h"
#include "celix/dm/ProvidedService.h"
#include "celix_dependency_manager.h"



namespace celix { namespace dm {

    enum class ComponentState {
        INACTIVE =                              1,
        WAITING_FOR_REQUIRED =                  2,
        INITIALIZING =                          3,
        DEINITIALIZING =                        4,
        INSTANTIATED_AND_WAITING_FOR_REQUIRED = 5,
        STARTING =                              6,
        STOPPING =                              7,
        TRACKING_OPTIONAL =                     8,
        SUSPENDING =                            9,
        SUSPENDED =                             10,
        RESUMING =                              11,
    };

    class BaseComponent {
    public:
        BaseComponent(celix_bundle_context_t *con, celix_dependency_manager_t* cdm, std::string name, std::string uuid) : context{con}, cDepMan{cdm}, cCmp{nullptr} {
            this->cCmp = celix_dmComponent_createWithUUID(this->context, name.c_str(), uuid.empty() ? nullptr : uuid.c_str());
            celix_dmComponent_setImplementation(this->cCmp, this);
            cmpUUID = std::string{celix_dmComponent_getUUID(this->cCmp)};
            cmpName = std::string{celix_dmComponent_getName(this->cCmp)};
        }
        virtual ~BaseComponent() noexcept;

        BaseComponent(const BaseComponent&) = delete;
        BaseComponent& operator=(const BaseComponent&) = delete;

        /**
         * Returns the C DM Component
         */
        celix_dm_component_t* cComponent() const { return this->cCmp; }

        /**
         * Returns the C bundle context
         */
        celix_bundle_context_t* bundleContext() const { return this->context; }

        /**
         * Returns the cmp uuid.
         */
        const std::string& getUUID() const {
            return cmpUUID;
        }

        /**
         * Returns the cmp name.
         */
        const std::string& getName() const {
            return cmpName;
        }

        /**
         * Return the cmp state
         */
        ComponentState getState() const {
             auto cState = celix_dmComponent_currentState(cCmp);
             switch (cState) {
                 case CELIX_DM_CMP_STATE_WAITING_FOR_REQUIRED:
                     return ComponentState::WAITING_FOR_REQUIRED;
                 case CELIX_DM_CMP_STATE_INITIALIZING:
                     return ComponentState::INITIALIZING;
                 case CELIX_DM_CMP_STATE_DEINITIALIZING:
                     return ComponentState::DEINITIALIZING;
                 case CELIX_DM_CMP_STATE_INITIALIZED_AND_WAITING_FOR_REQUIRED:
                     return ComponentState::INSTANTIATED_AND_WAITING_FOR_REQUIRED;
                 case CELIX_DM_CMP_STATE_STARTING:
                     return ComponentState::STARTING;
                 case CELIX_DM_CMP_STATE_STOPPING:
                     return ComponentState::STOPPING;
                 case CELIX_DM_CMP_STATE_TRACKING_OPTIONAL:
                     return ComponentState::TRACKING_OPTIONAL;
                 case CELIX_DM_CMP_STATE_SUSPENDING:
                     return ComponentState::SUSPENDING;
                 case CELIX_DM_CMP_STATE_SUSPENDED:
                     return ComponentState::SUSPENDED;
                 case CELIX_DM_CMP_STATE_RESUMING:
                     return ComponentState::RESUMING;
                 default:
                     return ComponentState::INACTIVE;
             }
         }

        /**
         * Wait for an empty Celix event queue.
         * Should not be called on the Celix event queue thread.
         *
         * Can be used to ensure that all created/updated components are completely processed (services registered
         * and/or service trackers are created).
         */
        void wait() const;

        /**
         * Run the dm component build. After this call the component is added to the dependency manager and
         * is enabled.
         * The underlining service registration and service tracker will be registered/created async.
         */
        void runBuild();

        friend std::ostream& operator<<(std::ostream& out, const BaseComponent& cmp);
    protected:
        celix_bundle_context_t* context;
        celix_dependency_manager_t* cDepMan;
        celix_dm_component_t *cCmp;
        std::string cmpUUID{};
        std::string cmpName{};

        std::atomic<bool> cmpAddedToDepMan{false};

        std::mutex mutex{}; //protects below
        std::vector<std::shared_ptr<BaseServiceDependency>> dependencies{};
        std::vector<std::shared_ptr<BaseProvidedService>> providedServices{};
        std::vector<std::shared_ptr<void>> componentContexts{};
    };

    /**
     * Stream outputs the full component info.
     */
    inline std::ostream& operator<<(std::ostream& out, const BaseComponent& cmp);

    template<class T>
    class Component : public BaseComponent {
        using type = T;
    private:
        std::mutex instanceMutex{};
        std::unique_ptr<T> instance {nullptr};
        std::shared_ptr<T> sharedInstance {nullptr};
        std::vector<T> valInstance {};

        void (T::*initFp)() = {};
        void (T::*startFp)() = {};
        void (T::*stopFp)() = {};
        void (T::*deinitFp)() = {};

        int (T::*initFpNoExc)() = {};
        int (T::*startFpNoExc)() = {};
        int (T::*stopFpNoExc)() = {};
        int (T::*deinitFpNoExc)() = {};

        /**
         * Ctor is private, use static create function member instead
         * @param context
         * @param cDepMan
         * @param name
         */
        Component(celix_bundle_context_t *context, celix_dependency_manager_t* cDepMan, std::string name, std::string uuid);
    public:
        ~Component() override;

        /**
         * Creates a Component using the provided bundle context
         * and component name.
         * @return newly created DM Component.
         */
        static std::shared_ptr<Component<T>> create(celix_bundle_context_t*, celix_dependency_manager_t* cDepMan, std::string name, std::string uuid);

        /**
         * Whether the component is valid. Invalid component can occurs when no new components can be created and
         * exceptions are not allowed.
         * @return
         */
        bool isValid() const;

        /**
         * Get the component instance. If no instance is explicitly set with setInstance than a instance will be create
         * using a default constructor.
         *
         * @return A reference to the component instance.
         */
        T& getInstance();

        /**
         * Set the component instance using a shared pointer.
         *
         * @return the DM Component reference for chaining (fluent API)
         */
        Component<T>& setInstance(std::shared_ptr<T> inst);

        /**
         * Set the component instance using a unique pointer.
         *
         * @return the DM Component reference for chaining (fluent API)
         */
        Component<T>& setInstance(std::unique_ptr<T>&& inst);

        /**
         * Set the component instance using a value or rval reference
         * The DM Component will contain the instance.
         *
         * @return the DM Component reference for chaining (fluent API)
         */
        Component<T>& setInstance(T&& inst);

        /**
         * Adds a C++ interface to provide as service to the Celix framework.
         *
         * @param serviceName The service name to use
         * @param version The version of the interface (e.g. "1.0.0"), can be an empty string
         * @param properties To (meta) properties to provide with the service
         * @return the DM Component reference for chaining (fluent API)
         */
        template<class I> Component<T>& addInterfaceWithName(const std::string &serviceName, const std::string &version = std::string{}, const Properties &properties = Properties{});

        /**
         * Adds a C++ interface to provide as service to the Celix framework.
         *
         * @param serviceName The service name to use
         * @param version The version of the interface (e.g. "1.0.0"), can be an empty string
         * @param properties To (meta) properties to provide with the service
         * @return the DM Component reference for chaining (fluent API)
         */
        template<class I> Component<T>& addInterface(const std::string &version = std::string{}, const Properties &properties = Properties{});

        /**
         * Adds a C interface to provide as service to the Celix framework.
         *
         * @param svc The service struct
         * @param serviceName The service name to use
         * @param version The version of the interface (e.g. "1.0.0"), can be an empty string
         * @param properties To (meta) properties to provide with the service
         */
        template<class I> Component<T>& addCInterface(I* svc, const std::string &serviceName, const std::string &version = std::string{}, const Properties &properties = Properties{});


        /**
         * @brief Creates a provided C services the component.
         *
         * The provided service can be fine tuned and build using a fluent API
         *
         * @param svc  The pointer to a C service (c struct)
         * @param serviceName The service name to use
         */
        template<class I> ProvidedService<T,I>& createProvidedCService(I* svc, std::string serviceName);

        /**
         * @brief Creates a provided C++ services for the component.
         *
         * The provided service can be fine tuned and build using a fluent API
         *
         * @note The service type I must be a base of component type T.
         *
         * @param serviceName The optional service name. If not provided the service name is inferred from I.
         */
        template<class I> ProvidedService<T,I>& createProvidedService(std::string serviceName = {});

        /**
         * @brief Creates a unassociated provided services for the component.
         *
         * The provided service can be fine tuned and build using a fluent API
         *
         * @note The provided service can - and is expected to be - be unassociated with the component type.
         * I.e. it can be a C service.
         * The ProvidedService result will store the shared_ptr of the service during its lifecycle.
         *
         * @param serviceName The optional service name. If not provided the service name is inferred from I.
         */
        template<class I> ProvidedService<T,I>& createUnassociatedProvidedService(std::shared_ptr<I> svc, std::string serviceName = {});

        /**
         * Adds a C interface to provide as service to the Celix framework.
         *
         * @param svc The service struct
         * @param serviceName The service name to use
         * @param version The version of the interface (e.g. "1.0.0"), can be an empty string
         * @param properties To (meta) properties to provide with the service
         */
        template<class I> Component<T>& removeCInterface(const I* svc);


        /**
         * Creates and adds a C++ service dependency to the component
         *
         * @return the Service Dependency reference for chaining (fluent API)
         */
        template<class I>
        ServiceDependency<T,I>& createServiceDependency(const std::string &name = std::string{});

        /**
         Removes a C++ service dependency from the component
         *
         * @return the DM Component reference for chaining (fluent API)
         */
        template<class I>
        Component<T>& remove(ServiceDependency<T,I>& dep);

        /**
         * Adds a C service dependency to the component
         *
         * @return the DM Component reference for chaining (fluent API)
         */
        template<typename I>
        CServiceDependency<T,I>& createCServiceDependency(const std::string &name);

        /**
         * Removes a C service dependency to the component
         *
         * @return the DM Component reference for chaining (fluent API)
         */
        template<typename I>
        Component<T>& remove(CServiceDependency<T,I>& dep);

        /**
         * Set the callback for the component life cycle control
         *
         * @param init The init callback.
         * @param start The start callback.
         * @param stop The stop callback.
         * @param deinit The deinit callback.
         *
         * @return the DM Component reference for chaining (fluent API)
         */
        Component<T>& setCallbacks(
                void (T::*init)(),
                void (T::*start)(),
                void (T::*stop)(),
                void (T::*deinit)()
        );

        /**
         * Set the callback for the component life cycle control
         * with a int return to indicate an error.
         *
         * @param init The init callback.
         * @param start The start callback.
         * @param stop The stop callback.
         * @param deinit The deinit callback.
         *
         * @return the DM Component reference for chaining (fluent API)
         */
        Component<T>& setCallbacks(
                int (T::*init)(),
                int (T::*start)(),
                int (T::*stop)(),
                int (T::*deinit)()
        );

        /**
         * Remove the previously registered callbacks for the component life cycle control
         *
         * @return the DM Component reference for chaining (fluent API)
         */
        Component<T>& removeCallbacks();


        /**
         * @brief Add context to the component. This can be used to ensure a object lifespan at least
         * matches that of the component.
         */
        Component<T>& addContext(std::shared_ptr<void>);

        /**
         * Build the component.
         *
         * When building the component all provided services and services dependencies are enabled.
         * This is not done automatically so that user can first construct component with their provided
         * service and service dependencies.
         *
         * Note that the after this call the component will be created and if the component can be started, it
         * will be started and the services will be registered.
         *
         * Should not be called from the Celix event  thread.
         */
         Component<T>& build();

        /**
         * Same as build, but this call will not wait til all service registrations and tracker are registered/openend
         * on the Celix event thread.
         * Can be called on the Celix event thread.
         */
        Component<T>& buildAsync();
    private:
        /**
         * @brief try to invoke a lifecycle method (init, start, stop and deinit) and catch and log a possible exception.
         * @returns 0 if no exception occurred else -1.
         */
        int invokeLifecycleMethod(const std::string& methodName, void (T::*lifecycleMethod)());
    };
}}

#include "celix/dm/Component_Impl.h"
