/**
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

#include <memory>
#include <iostream>
#include <iomanip>
#include <type_traits>
#include <algorithm>

using namespace celix::dm;

template<class T>
Component<T>::Component(celix_bundle_context_t *context, std::string name) : BaseComponent(context, name) {}

template<class T>
Component<T>::~Component() {
    this->dependencies.clear();
}

template<class T>
template<class I>
Component<T>& Component<T>::addInterfaceWithName(const std::string serviceName, const std::string version, const Properties properties) {
    if (!serviceName.empty()) {
        //setup c properties
        celix_properties_t *cProperties = properties_create();
        properties_set(cProperties, CELIX_FRAMEWORK_SERVICE_LANGUAGE, CELIX_FRAMEWORK_SERVICE_CXX_LANGUAGE);
        for (const auto& pair : properties) {
            properties_set(cProperties, (char *) pair.first.c_str(), (char *) pair.second.c_str());
        }

        T* cmpPtr = &this->getInstance();
        I* intfPtr = static_cast<I*>(cmpPtr); //NOTE T should implement I

        const char *cVersion = version.empty() ? nullptr : version.c_str();
        celix_dmComponent_addInterface(this->cComponent(), (char *) serviceName.c_str(), (char *) cVersion,
                               intfPtr, cProperties);
    } else {
        std::cerr << "Cannot add interface with a empty name\n";
    }

    return *this;
}

template<class T>
template<class I>
Component<T>& Component<T>::addInterface(const std::string version, const Properties properties) {
    //get name if not provided
    static_assert(std::is_base_of<I,T>::value, "Component T must implement Interface I");
    std::string serviceName = typeName<I>();
    if (serviceName.empty()) {
        std::cerr << "Cannot add interface, because type name could not be inferred. function: '"  << __PRETTY_FUNCTION__ << "'\n";
    }

    return this->addInterfaceWithName<I>(serviceName, version, properties);
};

template<class T>
template<class I>
Component<T>& Component<T>::addCInterface(const I* svc, const std::string serviceName, const std::string version, const Properties properties) {
    static_assert(std::is_pod<I>::value, "Service I must be a 'Plain Old Data' object");
    celix_properties_t *cProperties = properties_create();
    properties_set(cProperties, CELIX_FRAMEWORK_SERVICE_LANGUAGE, CELIX_FRAMEWORK_SERVICE_C_LANGUAGE);
    for (const auto& pair : properties) {
        properties_set(cProperties, (char*)pair.first.c_str(), (char*)pair.second.c_str());
    }

    const char *cVersion = version.empty() ? nullptr : version.c_str();
    celix_dmComponent_addInterface(this->cComponent(), (char*)serviceName.c_str(), (char*)cVersion, svc, cProperties);

    return *this;
};

template<class T>
template<class I>
Component<T>& Component<T>::removeCInterface(const I* svc){
    static_assert(std::is_pod<I>::value, "Service I must be a 'Plain Old Data' object");
    celix_dmComponent_removeInterface(this->cComponent(), svc);
    return *this;
};

template<class T>
template<class I>
ServiceDependency<T,I>& Component<T>::createServiceDependency(const std::string name) {
#ifdef __EXCEPTIONS
    auto dep = std::shared_ptr<ServiceDependency<T,I>> {new ServiceDependency<T,I>(name)};
#else
    static ServiceDependency<T,I> invalidDep{std::string{}, false};
    auto dep = std::shared_ptr<ServiceDependency<T,I>> {new(std::nothrow) ServiceDependency<T,I>(name)};
    if (dep == nullptr) {
        return invalidDep;
    }
#endif
    this->dependencies.push_back(dep);
    celix_dmComponent_addServiceDependency(cComponent(), dep->cServiceDependency());
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
CServiceDependency<T,I>& Component<T>::createCServiceDependency(const std::string name) {
#ifdef __EXCEPTIONS
    auto dep = std::shared_ptr<CServiceDependency<T,I>> {new CServiceDependency<T,I>(name)};
#else
    static CServiceDependency<T,I> invalidDep{std::string{}, false};
    auto dep = std::shared_ptr<CServiceDependency<T,I>> {new(std::nothrow) CServiceDependency<T,I>(name)};
    if (dep == nullptr) {
        return invalidDep;
    }
#endif
    this->dependencies.push_back(dep);
    celix_dmComponent_addServiceDependency(cComponent(), dep->cServiceDependency());
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
Component<T>* Component<T>::create(celix_bundle_context_t *context) {
    std::string name = typeName<T>();
    return Component<T>::create(context, name);
}

template<class T>
Component<T>* Component<T>::create(celix_bundle_context_t *context, std::string name) {
#ifdef __EXCEPTIONS
    Component<T>* cmp = new Component<T>{context, name};
#else
    static Component<T> invalid{nullptr, std::string{}};
    Component<T>* cmp = new(std::nothrow) Component<T>(context, name);
    if (cmp == nullptr) {
        cmp = &invalid;
    }
#endif
    return cmp;
}

template<class T>
bool Component<T>::isValid() const {
    return this->bundleContext() != nullptr;
}

template<class T>
T& Component<T>::getInstance() {
    if (this->valInstance.size() == 1) {
        return valInstance.front();
    } else if (this->sharedInstance.get() != nullptr) {
        return *this->sharedInstance;
    } else {
        if (this->instance.get() == nullptr) {
#ifdef __EXCEPTIONS
            this->instance = std::unique_ptr<T> {new T()};
#else
            this->instance = std::unique_ptr<T> {new(std::nothrow) T()};

#endif
        }
        return *this->instance;
    }
}

template<class T>
Component<T>& Component<T>::setInstance(std::shared_ptr<T> inst) {
    this->valInstance.clear();
    this->instance = std::unique_ptr<T> {nullptr};
    this->sharedInstance = inst;
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

