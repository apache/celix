/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */

#pragma once

#include <queue>
#include <optional>

namespace celix {

    //TODO the in between states, Initializing, Starting, Stopping, DeInitializing
    enum class ComponentManagerState {
        Disabled,
        ComponentUninitialized,
        ComponentInitialized,
        ComponentStarted
    };

    class GenericServiceDependency; //forward declaration
    class GenericProvidedService; //forward declaration

    //TODO refactor GenericComponentManager so that is useable without a type specific component, using callbacks.
    class GenericComponentManager {
    public:
        GenericComponentManager(std::shared_ptr <celix::IResourceBundle> owner, std::shared_ptr <celix::ServiceRegistry> reg, const std::string &name);

        virtual ~GenericComponentManager() = default;

        ComponentManagerState getState() const;

        bool isEnabled() const;

        bool isResolved() const;

        std::string getName() const;

        std::string getUUD() const;

        std::size_t getSuspendedCount() const;

//        GenericComponentManager &setLifecycleCallbacks(
//                std::function<void()> init,
//                std::function<void()> start,
//                std::function<void()> stop,
//                std::function<void()> deinit);
//
//        GenericComponentManager &setCreateCallbacks(
//                std::function<std::shared_ptr<void>()> create,
//                std::function<void()> destroy);

        void removeServiceDependency(const std::string &serviceDependencyUUID);

        void removeProvideService(const std::string &provideServiceUUID);

        std::size_t nrOfServiceDependencies();

        std::size_t nrOfProvidedServices();

        GenericComponentManager& enable();
        GenericComponentManager& disable();
    protected:
        void setEnabled(bool enable);
        std::shared_ptr <GenericServiceDependency> findGenericServiceDependency(const std::string &svcName, const std::string &svcDepUUID);
        std::shared_ptr <GenericProvidedService> findGenericProvidedService(const std::string &svcName, const std::string &providedServiceUUID);
        void updateState();
        void updateServiceRegistrations();
        void suspense();
        void resume();

        /**** Fields ****/
        const std::shared_ptr <celix::IResourceBundle> owner;
        const std::shared_ptr <celix::ServiceRegistry> reg;
        const std::string name;
        const std::string uuid;

        mutable std::mutex callbacksMutex{}; //protects below std::functions
        std::function<void()> initCmp{[] {/*nop*/}};
        std::function<void()> startCmp{[] {/*nop*/}};
        std::function<void()> stopCmp{[] {/*nop*/}};
        std::function<void()> deinitCmp{[] {/*nop*/}};

        std::mutex serviceDependenciesMutex{};
        std::unordered_map <std::string, std::shared_ptr<GenericServiceDependency>> serviceDependencies{}; //key = dep uuid

        std::mutex providedServicesMutex{};
        std::unordered_map <std::string, std::shared_ptr<GenericProvidedService>> providedServices{}; //key = provide uuid
    private:
        void setState(ComponentManagerState state);

        void setInitialized(bool initialized);

        void transition();

        mutable std::mutex stateMutex{}; //protects below
        ComponentManagerState state = ComponentManagerState::Disabled;
        bool enabled = false;
        bool initialized = false;
        bool suspended = false;
        std::size_t suspendedCount = 0;
        std::queue <std::pair<ComponentManagerState, ComponentManagerState>> transitionQueue{};
    };

    enum class Cardinality {
        One,
        Many
    };

    enum class UpdateServiceStrategy {
        Suspense,
        Locking
    };

    class GenericServiceDependency {
    public:
        virtual ~GenericServiceDependency() = default;

        bool isResolved() const;
        Cardinality getCardinality() const;
        bool isRequired() const;
        const std::string& getFilter() const;
        const std::string& getUUD() const;
        const std::string& getSvcName() const;
        bool isValid() const;
        UpdateServiceStrategy getStrategy() const;

        bool isEnabled() const;
        virtual void setEnabled(bool e) = 0;
    protected:
        GenericServiceDependency(
                std::shared_ptr<celix::ServiceRegistry> reg,
                std::string svcName,
                std::function<void()> stateChangedCallback,
                std::function<void()> suspenseCallback,
                std::function<void()> resumeCallback,
                bool isValid);

        void preServiceUpdate();
        void postServiceUpdate();

        //Fields
        const std::shared_ptr<celix::ServiceRegistry> reg;
        const std::string svcName;
        const std::function<void()> stateChangedCallback;
        const std::function<void()> suspenseCallback;
        const std::function<void()> resumeCallback;
        const std::string uuid;
        const bool valid;

        mutable std::mutex mutex{}; //protects below
        UpdateServiceStrategy strategy = UpdateServiceStrategy::Suspense;
        bool required = true;
        std::string filter{};
        Cardinality cardinality = Cardinality::One;
        std::vector<ServiceTracker> tracker{}; //max 1 (1 == enabled / 0 = disabled //TODO make optional
    };

    class GenericProvidedService {
    public:
        virtual ~GenericProvidedService() = default;

        bool isEnabled() const;
        const std::string& getUUID() const;
        const std::string& getServiceName() const;
        bool isValid() const;

        void setEnabled(bool enabled);
        void unregisterService();
        bool isServiceRegistered();
        virtual void registerService() = 0;
        void wait() const;
    protected:
        GenericProvidedService(std::string cmpUUID, std::shared_ptr<celix::ServiceRegistry> reg, std::string svcName, std::function<void()> updateServiceRegistrationsCallback, bool valid);

        const std::string cmpUUID;
        const std::shared_ptr<celix::ServiceRegistry> reg;
        const std::string uuid;
        const std::string svcName;
        const std::function<void()> updateServiceRegistrationsCallback;
        const bool valid;

        mutable std::mutex mutex{}; //protects below
        bool enabled = false;
        std::vector<celix::ServiceRegistration> registration{}; //TODO make optional
        celix::Properties properties{};
    };

    std::string toString(celix::ComponentManagerState state);
}

std::ostream& operator<<(std::ostream& out, celix::ComponentManagerState state);
std::ostream& operator<<(std::ostream& out, const celix::GenericComponentManager& mng);
