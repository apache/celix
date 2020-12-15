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

#include "celix/dm/Component.h"
#include "celix/dm/DependencyManager.h"
#include "celix/dm/ServiceDependency.h"
#include "celix/dm/ProvidedService.h"
#include "celix_dependency_manager.h"

#include <memory>
#include <iostream>
#include <iomanip>
#include <type_traits>
#include <algorithm>
#include <atomic>

using namespace celix::dm;

inline void BaseComponent::runBuild() {
    if (context == nullptr || cDepMan == nullptr) {
        return;
    }

    for (auto& provide : providedServices) {
        provide->runBuild();
    }

    for (auto &dep : dependencies) {
        dep->runBuild();
    }

    bool alreadyAdded = cmpAddedToDepMan.exchange(true);
    if (!alreadyAdded) {
        celix_dependencyManager_add(cDepMan, cCmp);
    }
}

inline BaseComponent::~BaseComponent() noexcept {
    if (!cmpAddedToDepMan) {
        celix_dmComponent_destroy(cCmp);
    }
}

template<class T>
Component<T>::Component(celix_bundle_context_t *context, celix_dependency_manager_t* cDepMan, const std::string &name) : BaseComponent(context, cDepMan, name) {}

template<class T>
Component<T>::~Component() = default;

template<class T>
template<class I>
Component<T>& Component<T>::addInterfaceWithName(const std::string &serviceName, const std::string &version, const Properties &properties) {
    if (!serviceName.empty()) {
        T* cmpPtr = &this->getInstance();
        I* intfPtr = static_cast<I*>(cmpPtr); //NOTE T should implement I

        auto provide = std::make_shared<ProvidedService<T,I>>(cComponent(), serviceName, intfPtr, true);
        provide->setVersion(version);
        provide->setProperties(properties);
        providedServices.push_back(provide);
    } else {
        std::cerr << "Cannot add interface with a empty name\n";
    }

    return *this;
}

template<class T>
template<class I>
Component<T>& Component<T>::addInterface(const std::string &version, const Properties &properties) {
    //get name if not provided
    static_assert(std::is_base_of<I,T>::value, "Component T must implement Interface I");
    std::string serviceName = typeName<I>();
    if (serviceName.empty()) {
        std::cerr << "Cannot add interface, because type name could not be inferred. function: '"  << __PRETTY_FUNCTION__ << "'\n";
    }

    return this->addInterfaceWithName<I>(serviceName, version, properties);
}

template<class T>
template<class I>
Component<T>& Component<T>::addCInterface(I* svc, const std::string &serviceName, const std::string &version, const Properties &properties) {
    auto provide = std::make_shared<ProvidedService<T,I>>(cComponent(), serviceName, svc, false);
    provide->setVersion(version);
    provide->setProperties(properties);
    providedServices.push_back(provide);
    return *this;
}

template<class T>
template<class I>
Component<T>& Component<T>::removeCInterface(const I* svc){
    celix_dmComponent_removeInterface(this->cComponent(), svc);
    for (auto it = providedServices.begin(); it != providedServices.end(); ++it) {
        std::shared_ptr<BaseProvidedService> provide = *it;
        if (provide->getService() == static_cast<const void*>(svc)) {
            providedServices.erase(it);
            break;
        }
    }
    return *this;
}

template<class T>
template<class I>
ServiceDependency<T,I>& Component<T>::createServiceDependency(const std::string &name) {
    static ServiceDependency<T,I> invalidDep{cComponent(), std::string{}, false};
    auto dep = std::shared_ptr<ServiceDependency<T,I>> {new ServiceDependency<T,I>(cComponent(), name)};
    if (dep == nullptr) {
        return invalidDep;
    }
    dependencies.push_back(dep);
    dep->setComponentInstance(&getInstance());
    return *dep;
}

template<class T>
template<class I>
Component<T>& Component<T>::remove(ServiceDependency<T,I>& dep) {
    celix_component_removeServiceDependency(cComponent(), dep.cServiceDependency());
    this->dependencies.erase(std::remove(this->dependencies.begin(), this->dependencies.end(), dep));
    return *this;
}

template<class T>
template<typename I>
CServiceDependency<T,I>& Component<T>::createCServiceDependency(const std::string &name) {
    static CServiceDependency<T,I> invalidDep{cComponent(), std::string{}, false};
    auto dep = std::shared_ptr<CServiceDependency<T,I>> {new CServiceDependency<T,I>(cComponent(), name)};
    if (dep == nullptr) {
        return invalidDep;
    }
    dependencies.push_back(dep);
    dep->setComponentInstance(&getInstance());
    return *dep;
}

template<class T>
template<typename I>
Component<T>& Component<T>::remove(CServiceDependency<T,I>& dep) {
    celix_component_removeServiceDependency(cComponent(), dep.cServiceDependency());
    this->dependencies.erase(std::remove(this->dependencies.begin(), this->dependencies.end(), dep));
    return *this;
}

template<class T>
Component<T>* Component<T>::create(celix_bundle_context_t *context, celix_dependency_manager_t* cDepMan) {
    std::string name = typeName<T>();
    return Component<T>::create(context, cDepMan, name);
}

template<class T>
Component<T>* Component<T>::create(celix_bundle_context_t *context, celix_dependency_manager_t* cDepMan, const std::string &name) {
    static Component<T> invalid{nullptr, nullptr, std::string{}};
    Component<T>* cmp = new (std::nothrow) Component<T>(context, cDepMan, name);
    if (cmp == nullptr) {
        cmp = &invalid;
    }
    return cmp;
}

template<class T>
bool Component<T>::isValid() const {
    return this->bundleContext() != nullptr;
}


template<typename T>
static
typename std::enable_if<std::is_default_constructible<T>::value, T*>::type
createInstance() {
    return new T{};
}

template<typename T>
static
typename std::enable_if<!std::is_default_constructible<T>::value, T*>::type
createInstance() {
    return nullptr;
}

template<class T>
T& Component<T>::getInstance() {
    if (!valInstance.empty()) {
        return valInstance.front();
    } else if (sharedInstance) {
        return *sharedInstance;
    } else if (instance) {
        return *instance;
    } else {
        T* newInstance = createInstance<T>(); //uses SFINAE to detect if default ctor exists
        instance = std::unique_ptr<T>{newInstance};
        return *instance;
    }
}

template<class T>
Component<T>& Component<T>::setInstance(std::shared_ptr<T> inst) {
    this->valInstance.clear();
    this->instance = std::unique_ptr<T> {nullptr};
    this->sharedInstance = std::move(inst);
    return *this;
}

template<class T>
Component<T>& Component<T>::setInstance(std::unique_ptr<T>&& inst) {
    this->valInstance.clear();
    this->sharedInstance = std::shared_ptr<T> {nullptr};
    this->instance = std::move(inst);
    return *this;
}

template<class T>
Component<T>& Component<T>::setInstance(T&& inst) {
    this->instance = std::unique_ptr<T> {nullptr};
    this->sharedInstance = std::shared_ptr<T> {nullptr};
    this->valInstance.clear();
    this->valInstance.push_back(std::forward<T>(inst));
    return *this;
}

template<class T>
Component<T>& Component<T>::setCallbacks(
        void (T::*init)(),
        void (T::*start)(),
        void (T::*stop)(),
        void (T::*deinit)() ) {

    this->initFp = init;
    this->startFp = start;
    this->stopFp = stop;
    this->deinitFp = deinit;

    int (*cInit)(void *) = [](void *handle) {
        Component<T>* cmp = (Component<T>*)(handle);
        T* inst = &cmp->getInstance();
        void (T::*fp)() = cmp->initFp;
        if (fp != nullptr) {
            (inst->*fp)();
        }
        return 0;
    };
    int (*cStart)(void *) = [](void *handle) {
        Component<T>* cmp = (Component<T>*)(handle);
        T* inst = &cmp->getInstance();
        void (T::*fp)() = cmp->startFp;
        if (fp != nullptr) {
            (inst->*fp)();
        }
        return 0;
    };
    int (*cStop)(void *) = [](void *handle) {
        Component<T>* cmp = (Component<T>*)(handle);
        T* inst = &cmp->getInstance();
        void (T::*fp)() = cmp->stopFp;
        if (fp != nullptr) {
            (inst->*fp)();
        }
        return 0;
    };
    int (*cDeinit)(void *) = [](void *handle) {
        Component<T>* cmp = (Component<T>*)(handle);
        T* inst = &cmp->getInstance();
        void (T::*fp)() = cmp->deinitFp;
        if (fp != nullptr) {
            (inst->*fp)();
        }
        return 0;
    };

    celix_dmComponent_setCallbacks(this->cComponent(), cInit, cStart, cStop, cDeinit);

    return *this;
}

template<class T>
Component<T>& Component<T>::setCallbacks(
        int (T::*init)(),
        int (T::*start)(),
        int (T::*stop)(),
        int (T::*deinit)() ) {

    this->initFpNoExc = init;
    this->startFpNoExc = start;
    this->stopFpNoExc = stop;
    this->deinitFpNoExc = deinit;

    int (*cInit)(void *) = [](void *handle) {
        Component<T>* cmp = (Component<T>*)(handle);
        T* inst = &cmp->getInstance();
        int (T::*fp)() = cmp->initFpNoExc;
        if (fp != nullptr) {
            return (inst->*fp)();
        }
        return 0;
    };
    int (*cStart)(void *) = [](void *handle) {
        Component<T>* cmp = (Component<T>*)(handle);
        T* inst = &cmp->getInstance();
        int (T::*fp)() = cmp->startFpNoExc;
        if (fp != nullptr) {
            return (inst->*fp)();
        }
        return 0;
    };
    int (*cStop)(void *) = [](void *handle) {
        Component<T>* cmp = (Component<T>*)(handle);
        T* inst = &cmp->getInstance();
        int (T::*fp)() = cmp->stopFpNoExc;
        if (fp != nullptr) {
            return (inst->*fp)();
        }
        return 0;
    };
    int (*cDeinit)(void *) = [](void *handle) {
        Component<T>* cmp = (Component<T>*)(handle);
        T* inst = &cmp->getInstance();
        int (T::*fp)() = cmp->deinitFpNoExc;
        if (fp != nullptr) {
            return (inst->*fp)();
        }
        return 0;
    };

    celix_dmComponent_setCallbacks(this->cComponent(), cInit, cStart, cStop, cDeinit);

    return *this;
}

template<class T>
Component<T>& Component<T>::removeCallbacks() {

    celix_dmComponent_setCallbacks(this->cComponent(), nullptr, nullptr, nullptr, nullptr);

    return *this;
}

template<typename T>
Component<T>& Component<T>::build() {
    runBuild();
    return *this;
}

template<class T>
template<class I>
ProvidedService<T, I> &Component<T>::createProvidedCService(I *svc, std::string serviceName) {
    auto provide = std::make_shared<ProvidedService<T,I>>(cComponent(), serviceName, svc, false);
    providedServices.push_back(provide);
    return *provide;
}

template<class T>
template<class I>
ProvidedService<T, I> &Component<T>::createProvidedService(std::string serviceName) {
    static_assert(std::is_base_of<I,T>::value, "Component T must implement Interface I");
    if (serviceName.empty()) {
        serviceName = typeName<I>();
    }
    if (serviceName.empty()) {
        std::cerr << "Cannot add interface, because type name could not be inferred. function: '"  << __PRETTY_FUNCTION__ << "'\n";
    }

    I* svcPtr = &this->getInstance();
    auto provide = std::make_shared<ProvidedService<T,I>>(cComponent(), serviceName, svcPtr, true);
    providedServices.push_back(provide);
    return *provide;
}

