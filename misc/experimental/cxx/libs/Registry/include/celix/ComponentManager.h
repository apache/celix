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

#include <memory>
#include <unordered_map>
#include <queue>
#include <mutex>

#include "celix/Utils.h"
#include "celix/ServiceRegistry.h"

namespace celix {

    enum class ComponentManagerState {
        Disabled,
        ComponentUninitialized,
        ComponentInitialized,
        ComponentStarted
    };

    template<typename T, typename I>
    class ServiceDependency; //forward declaration
    class GenericServiceDependency; //forward declaration

    template<typename T, typename I>
    class ProvidedService; //forward declaration
    class GenericProvidedService; //forward declaration

    class GenericComponentManager {
    public:
        virtual ~GenericComponentManager() = default;

        ComponentManagerState getState() const;
        bool isEnabled() const;
        bool isResolved() const;
        std::string getName() const;
        std::string getUUD() const;
        std::size_t getSuspendedCount() const;


        void removeServiceDependency(const std::string& serviceDependencyUUID);
        void removeProvideService(const std::string& provideServiceUUID);
        std::size_t nrOfServiceDependencies();
        std::size_t nrOfProvidedServices();
    protected:
        GenericComponentManager(std::shared_ptr<celix::IResourceBundle> owner, std::shared_ptr<celix::ServiceRegistry> reg, const std::string &name);

        void setEnabled(bool enable);
        std::shared_ptr<GenericServiceDependency> findGenericServiceDependency(const std::string& svcName, const std::string& svcDepUUID);
        std::shared_ptr<GenericProvidedService> findGenericProvidedService(const std::string& svcName, const std::string& providedServiceUUID);
        void updateState();
        void updateServiceRegistrations();
        void suspense();
        void resume();

        /**** Fields ****/
        const std::shared_ptr<celix::IResourceBundle> owner;
        const std::shared_ptr<celix::ServiceRegistry> reg;
        const std::string name;
        const std::string uuid;

        mutable std::mutex callbacksMutex{}; //protects below std::functions
        std::function<void()> initCmp{[]{/*nop*/}};
        std::function<void()> startCmp{[]{/*nop*/}};
        std::function<void()> stopCmp{[]{/*nop*/}};
        std::function<void()> deinitCmp{[]{/*nop*/}};

        std::mutex serviceDependenciesMutex{};
        std::unordered_map<std::string,std::shared_ptr<GenericServiceDependency>> serviceDependencies{}; //key = dep uuid

        std::mutex providedServicesMutex{};
        std::unordered_map<std::string,std::shared_ptr<GenericProvidedService>> providedServices{}; //key = provide uuid
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
        std::queue<std::pair<ComponentManagerState,ComponentManagerState>> transitionQueue{};
    };

    template<typename T>
    class ComponentManager : public GenericComponentManager {
    public:
        using ComponentType = T;

        ComponentManager(std::shared_ptr<celix::IResourceBundle> owner, std::shared_ptr<celix::ServiceRegistry> reg, std::shared_ptr<T> cmpInstance);
        ~ComponentManager() override = default;

        ComponentManager<T>& enable();
        ComponentManager<T>& disable();

        ComponentManager<T>& setCallbacks(
                void (T::*init)(),
                void (T::*start)(),
                void (T::*stop)(),
                void (T::*deinit)());

        template<typename I>
        ServiceDependency<T,I>& addServiceDependency();

        template<typename I>
        ServiceDependency<T,I>& findServiceDependency(const std::string& serviceDependencyUUID);

        ComponentManager<T>& extractUUID(std::string& out);

        template<typename I>
        ProvidedService<T,I>& addProvidedService();

        template<typename I>
        ProvidedService<T,I>& findProvidedService(const std::string& providedServiceUUID);
//
//        template<typename F>
//        ServiceDependency<T,F>& addFunctionServiceDependency(const std::string &functionName);

        std::shared_ptr<T> getCmpInstance() const;
    private:
        const std::shared_ptr<T> cmpInstance;
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
        std::vector<ServiceTracker> tracker{}; //max 1 (1 == enabled / 0 = disabled
    };

    template<typename T, typename I>
    class ServiceDependency : public GenericServiceDependency {
    public:
        using ComponentType = T;
        using ServiceType = I;

        ServiceDependency(
                std::shared_ptr<celix::ServiceRegistry> reg,
                std::function<void()> stateChangedCallback,
                std::function<std::shared_ptr<T>()> getCmpInstance,
                std::function<void()> suspenseCallback,
                std::function<void()> resumeCallback,
                bool isValid = true);
        ~ServiceDependency() override = default;

        ServiceDependency<T,I>& setFilter(const std::string &filter);
        ServiceDependency<T,I>& setStrategy(UpdateServiceStrategy strategy);
        ServiceDependency<T,I>& setRequired(bool required);
        ServiceDependency<T,I>& setCardinality(celix::Cardinality cardinality);
        ServiceDependency<T,I>& setCallbacks(void (T::*set)(const std::shared_ptr<I>&));
        ServiceDependency<T,I>& setCallbacks(void (T::*add)(const std::shared_ptr<I>&), void (T::*remove)(const std::shared_ptr<I>&));
        //TODO update callback
        //TODO callbacks with properties and owner

        ServiceDependency<T,I>& extractUUID(std::string& out);
        ServiceDependency<T,I>& enable();
        ServiceDependency<T,I>& disable();

        void setEnabled(bool e) override;

    private:
        void setService(std::shared_ptr<I> svc, const celix::Properties &props, const celix::IResourceBundle &owner);
        void addService(std::shared_ptr<I> svc, const celix::Properties &props, const celix::IResourceBundle &owner);
        void remService(std::shared_ptr<I> svc, const celix::Properties &props, const celix::IResourceBundle &owner);
        void updateServices(std::vector<std::tuple<std::shared_ptr<I>, const celix::Properties*, const celix::IResourceBundle *>> rankedServices);

        const std::function<std::shared_ptr<T>()> getCmpInstance;

        std::mutex callbacksMutex{}; //protects below
        std::function<void(const std::shared_ptr<I>& svc, const celix::Properties &props, const celix::IResourceBundle &owner)> set{};
        std::function<void(const std::shared_ptr<I>& svc, const celix::Properties &props, const celix::IResourceBundle &owner)> add{};
        std::function<void(const std::shared_ptr<I>& svc, const celix::Properties &props, const celix::IResourceBundle &owner)> rem{};
        std::function<void(std::vector<std::tuple<std::shared_ptr<I>, const celix::Properties*, const celix::IResourceBundle *>> rankedServices)> update{};
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
        std::vector<celix::ServiceRegistration> registration{}; //max size 1 (optional). 0 == disabled, 1 == enabled
        celix::Properties properties{};
    };

    template<typename T, typename I>
    class ProvidedService : public GenericProvidedService {
    public:
        ProvidedService(
                std::string cmpUUID,
                std::shared_ptr<celix::ServiceRegistry> reg,
                std::function<std::shared_ptr<T>()> getCmpInstanceCallback,
                std::function<void()> updateServiceRegistrationsCallback,
                bool valid = true);
        ~ProvidedService() override = default;

        ProvidedService& extractUUID(std::string& out);
        ProvidedService& enable();
        ProvidedService& disable();
        ProvidedService& setProperties(const celix::Properties& props);
        ProvidedService& setProperties(celix::Properties props);
        ProvidedService& addProperty(const std::string &name, const std::string &val); //TODO add int, long, bool, etc property

        void registerService() override;
    private:
        const std::function<std::shared_ptr<T>()> getcmpInstanceCallback;
    };
}

std::ostream& operator<< (std::ostream& out, celix::ComponentManagerState state);
std::ostream& operator<< (std::ostream& out, const celix::GenericComponentManager& mng);



/**********************************************************************************************************************
  ComponentManager Implementation
 **********************************************************************************************************************/

template<typename T>
inline celix::ComponentManager<T>::ComponentManager(
        std::shared_ptr<celix::IResourceBundle> owner,
        std::shared_ptr<celix::ServiceRegistry> reg,
        std::shared_ptr<T> cmpInstance) : GenericComponentManager{owner, reg, celix::typeName<T>()}, cmpInstance{std::move(cmpInstance)} {
}

template<typename T>
inline celix::ComponentManager<T>& celix::ComponentManager<T>::enable() {
    setEnabled(true);
    return *this;
}

template<typename T>
inline celix::ComponentManager<T>& celix::ComponentManager<T>::disable() {
    setEnabled(false);
    return *this;
}

template<typename T>
inline celix::ComponentManager<T>& celix::ComponentManager<T>::setCallbacks(
        void (T::*memberInit)(),
        void (T::*memberStart)(),
        void (T::*memberStop)(),
        void (T::*memberDeinit)()) {
    std::lock_guard<std::mutex> lck{callbacksMutex};
    initCmp = [this, memberInit]() {
        if (memberInit) {
            T* ptr = this->getCmpInstance().get();
            (ptr->*memberInit)();
        }
    };
    startCmp = [this, memberStart]() {
        if (memberStart) {
            T* ptr = this->getCmpInstance().get();
            (ptr->*memberStart)();
        }
    };
    stopCmp = [this, memberStop]() {
        if (memberStop) {
            T* ptr = this->getCmpInstance().get();
            (ptr->*memberStop)();
        }
    };
    deinitCmp = [this, memberDeinit]() {
        if (memberDeinit) {
            T* ptr = this->getCmpInstance().get();
            (ptr->*memberDeinit)();
        }
    };
    return *this;
}

template<typename T>
inline celix::ComponentManager<T>& celix::ComponentManager<T>::extractUUID(std::string &out) {
    out = uuid;
    return *this;
}

template<typename T>
template<typename I>
inline celix::ServiceDependency<T,I>& celix::ComponentManager<T>::addServiceDependency() {
    auto *dep = new celix::ServiceDependency<T,I>{reg, [this]{updateState();}, [this]{return getCmpInstance();}, [this]{suspense();}, [this]{resume();}};
    std::lock_guard<std::mutex> lck{serviceDependenciesMutex};
    serviceDependencies[dep->getUUD()] = std::unique_ptr<GenericServiceDependency>{dep};
    return *dep;
}

template<typename T>
template<typename I>
inline celix::ServiceDependency<T,I>& celix::ComponentManager<T>::findServiceDependency(const std::string& serviceDependencyUUID) {
    static celix::ServiceDependency<T,I> invalid{reg, []{}, []{return nullptr;}, []{} , []{}, false};
    auto svcName = celix::typeName<I>();
    auto found = findGenericServiceDependency(svcName, serviceDependencyUUID);
    if (found) {
        auto *ptr = static_cast<celix::ServiceDependency<T,I>*>(found.get());
        return *ptr;
    } else {
        //note warning logged in findGenericServiceDependency
        return invalid;
    }
}

template<typename T>
inline std::shared_ptr<T> celix::ComponentManager<T>::getCmpInstance() const {
    return cmpInstance;
}

template<typename T>
template<typename I>
inline celix::ProvidedService<T, I> &celix::ComponentManager<T>::addProvidedService() {
    auto *provided = new celix::ProvidedService<T,I>{getUUD(), reg, [this]{return getCmpInstance();}, [this]{updateServiceRegistrations();}};
    std::lock_guard<std::mutex> lck{providedServicesMutex};
    providedServices[provided->getUUID()] = std::shared_ptr<GenericProvidedService>{provided};
    return *provided;
}

template<typename T>
template<typename I>
inline celix::ProvidedService<T, I> &celix::ComponentManager<T>::findProvidedService(const std::string &providedServiceUUID) {
    static celix::ProvidedService<T,I> invalid{uuid, reg, []{return nullptr;}, []{}, false};
    auto svcName = celix::typeName<I>();
    auto found = findGenericProvidedService(svcName, providedServiceUUID);
    if (found) {
        auto *ptr = static_cast<celix::ProvidedService<T,I>*>(found.get());
        return *ptr;
    } else {
        //note warning logged in findGenericProvidedService
        return invalid;
    }
}


/**********************************************************************************************************************
  ServiceDependency Implementation
 **********************************************************************************************************************/

template<typename T, typename I>
inline celix::ServiceDependency<T,I>::ServiceDependency(
        std::shared_ptr<celix::ServiceRegistry> _reg,
        std::function<void()> _stateChangedCallback,
        std::function<std::shared_ptr<T>()> _getCmpInstance,
        std::function<void()> _suspenseCallback,
        std::function<void()> _resumeCallback,
        bool isValid) :
            GenericServiceDependency{std::move(_reg),
                                     celix::typeName<I>(),
             std::move(_stateChangedCallback),
             std::move(_suspenseCallback),
             std::move(_resumeCallback),
             isValid},
             getCmpInstance{std::move(_getCmpInstance)} {};

template<typename T, typename I>
inline void celix::ServiceDependency<T,I>::setEnabled(bool enable) {
    bool currentlyEnabled = isEnabled();
    {
        std::vector<ServiceTracker> newTracker{};
        if (enable && !currentlyEnabled) {
            //enable
            using namespace std::placeholders;
            ServiceTrackerOptions<I> opts{};
            opts.filter = this->filter;
            opts.preServiceUpdateHook = [this]{preServiceUpdate();};
            opts.postServiceUpdateHook = [this]{postServiceUpdate();};
            opts.setWithOwner = std::bind(&ServiceDependency::setService, this, _1, _2, _3);
            opts.addWithOwner = std::bind(&ServiceDependency::addService, this, _1, _2, _3);
            opts.removeWithOwner = std::bind(&ServiceDependency::remService, this, _1, _2, _3);
            opts.updateWithOwner = std::bind(&ServiceDependency::updateServices, this, _1);
            newTracker.emplace_back(reg->trackServices(opts));
            std::lock_guard<std::mutex> lck{mutex};
            std::swap(tracker, newTracker);

        } else if (!enable and currentlyEnabled) {
            //disable
            std::lock_guard<std::mutex> lck{mutex};
            std::swap(tracker, newTracker/*empty*/);
        }
        //newTracker out of scope -> RAII -> for disable clear current tracker, for enable empty newTracker
    }
    stateChangedCallback();
}

template<typename T, typename I>
inline celix::ServiceDependency<T,I>& celix::ServiceDependency<T,I>::enable() {
    setEnabled(true);
    return *this;
}

template<typename T, typename I>
inline celix::ServiceDependency<T,I>& celix::ServiceDependency<T,I>::disable() {
    setEnabled(false);
    return *this;
}


template<typename T, typename I>
inline celix::ServiceDependency<T,I>& celix::ServiceDependency<T,I>::setCallbacks(void (T::*setfp)(const std::shared_ptr<I>&)) {
    auto setFunc = [this, setfp](const std::shared_ptr<I>& svc, const celix::Properties &, const celix::IResourceBundle &) {
        (getCmpInstance().get()->*setfp)(svc);
    };
    std::lock_guard<std::mutex> lck{callbacksMutex};
    set = std::move(setFunc);
    return *this;
}


template<typename T, typename I>
inline celix::ServiceDependency<T,I>& celix::ServiceDependency<T,I>::setCallbacks(void (T::*addfp)(const std::shared_ptr<I>&), void (T::*remfp)(const std::shared_ptr<I>&)) {
    auto addFunc = [this, addfp](const std::shared_ptr<I>& svc, const celix::Properties &, const celix::IResourceBundle &) {
        (getCmpInstance().get()->*addfp)(svc);
    };
    auto remFunc = [this, remfp](const std::shared_ptr<I>& svc, const celix::Properties &, const celix::IResourceBundle &) {
        (getCmpInstance().get()->*remfp)(svc);
    };
    std::lock_guard<std::mutex> lck{callbacksMutex};
    add = std::move(addFunc);
    rem = std::move(remFunc);
    return *this;
}

template<typename T, typename I>
inline celix::ServiceDependency<T,I>& celix::ServiceDependency<T,I>::setFilter(const std::string &f) {
    std::lock_guard<std::mutex> lck{mutex};
    filter = f;
    return *this;
}

template<typename T, typename I>
inline celix::ServiceDependency<T,I>& celix::ServiceDependency<T,I>::setStrategy(UpdateServiceStrategy s) {
    std::lock_guard<std::mutex> lck{mutex};
    strategy = s;
    return *this;
}

template<typename T, typename I>
inline celix::ServiceDependency<T,I>& celix::ServiceDependency<T,I>::setRequired(bool r) {
    std::lock_guard<std::mutex> lck{mutex};
    required = r;
    return *this;
}

template<typename T, typename I>
inline celix::ServiceDependency<T,I>& celix::ServiceDependency<T,I>::setCardinality(celix::Cardinality c) {
    std::lock_guard<std::mutex> lck{mutex};
    cardinality = c;
    return *this;
}

template<typename T, typename I>
inline celix::ServiceDependency<T,I>& celix::ServiceDependency<T,I>::extractUUID(std::string& out) {
    out = uuid;
    return *this;
}

template<typename T, typename I>
inline void celix::ServiceDependency<T,I>::setService(std::shared_ptr<I> svc, const celix::Properties &props, const celix::IResourceBundle &owner) {
    std::lock_guard<std::mutex> lck{callbacksMutex};
    if (set) {
        set(std::move(svc), props, owner);
    }
}

template<typename T, typename I>
inline void celix::ServiceDependency<T,I>::addService(std::shared_ptr<I> svc, const celix::Properties &props, const celix::IResourceBundle &owner) {
    std::lock_guard<std::mutex> lck{callbacksMutex};
    if (add) {
        add(std::move(svc), props, owner);
    }
}

template<typename T, typename I>
inline void celix::ServiceDependency<T,I>::remService(std::shared_ptr<I> svc, const celix::Properties &props, const celix::IResourceBundle &owner) {
    std::lock_guard<std::mutex> lck{callbacksMutex};
    if (rem) {
        rem(std::move(svc), props, owner);
    }
}

template<typename T, typename I>
inline void celix::ServiceDependency<T,I>::updateServices(std::vector<std::tuple<std::shared_ptr<I>, const celix::Properties*, const celix::IResourceBundle *>> rankedServices) {
    std::lock_guard<std::mutex> lck{callbacksMutex};
    if (update) {
        update(std::move(rankedServices));
    }
}

/**********************************************************************************************************************
  ProvidedService Implementation
 **********************************************************************************************************************/

template<typename T, typename I>
inline celix::ProvidedService<T,I>::ProvidedService(
        std::string cmpUUID,
        std::shared_ptr<celix::ServiceRegistry> reg,
        std::function<std::shared_ptr<T>()> _getCmpInstanceCallback,
        std::function<void()> updateServiceRegistrationsCallback,
        bool valid) :
    GenericProvidedService{std::move(cmpUUID), std::move(reg), celix::typeName<I>(), std::move(updateServiceRegistrationsCallback), valid}, getcmpInstanceCallback{_getCmpInstanceCallback} {}

template<typename T, typename I>
inline void celix::ProvidedService<T, I>::registerService() {
    if (isServiceRegistered()) {
        return; //already registered. note that also means a disable -> enable is needed to update a provided service
    }
    auto inst = getcmpInstanceCallback();
    properties[celix::SERVICE_COMPONENT_ID] = cmpUUID; //always set cmp uuid

    //TODO try to improve compile error when this happens
    std::shared_ptr<I> svcPtr = inst; //NOTE T should implement (inherit) I

    std::vector<ServiceRegistration> newReg{};
    newReg.emplace_back(reg->registerService<I>(inst, properties));
    std::lock_guard<std::mutex> lck{mutex};
    std::swap(newReg, registration);
}

template<typename T, typename I>
inline celix::ProvidedService<T,I>& celix::ProvidedService<T, I>::extractUUID(std::string &out) {
    out = uuid;
    return *this;
}

template<typename T, typename I>
inline celix::ProvidedService<T,I>& celix::ProvidedService<T, I>::enable() {
    setEnabled(true);
    return *this;
}

template<typename T, typename I>
inline celix::ProvidedService<T,I>& celix::ProvidedService<T, I>::disable() {
    setEnabled(false);
    return *this;
}

template<typename T, typename I>
inline celix::ProvidedService<T,I>& celix::ProvidedService<T, I>::setProperties(const celix::Properties& p) {
    properties = p;
    return *this;
}

template<typename T, typename I>
inline celix::ProvidedService<T,I>& celix::ProvidedService<T, I>::setProperties(celix::Properties p) {
    properties = std::move(p);
    return *this;
}

template<typename T, typename I>
inline celix::ProvidedService<T,I>& celix::ProvidedService<T, I>::addProperty(const std::string &name, const std::string &val) {
    properties[name] = val;
    return *this;
}
