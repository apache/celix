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

using namespace celix::dm;

template<class T>
Component<T>::Component(const bundle_context_pt context, std::string name) : BaseComponent(context, name) { }

template<class T>
Component<T>::~Component() { }

template<class T>
template<class I>
Component<T>& Component<T>::addInterface(const std::string version) {
    Properties props;
    return addInterface<I>(version, props);
}

template<class T>
template<class I>
Component<T>& Component<T>::addInterface(const std::string version, const Properties props) {
    this->addInterface(typeName<I>(), version.c_str(), props);
    return *this;
}

template<class T>
Component<T>& Component<T>::addInterface(const std::string serviceName, const std::string version, const Properties properties) {
    properties_pt cProperties = properties_create();
    properties_set(cProperties, (char*)"lang", (char*)"c++");
    for (auto iter = properties.begin(); iter != properties.end(); iter++) {
        properties_set(cProperties, (char*)iter->first.c_str(), (char*)iter->second.c_str());
    }

    const char *cVersion = version.empty() ? nullptr : version.c_str();
    component_addInterface(this->cComponent(), (char*)serviceName.c_str(), (char*)cVersion, &this->getInstance(), cProperties);

    return *this;
};

template<class T>
Component<T>& Component<T>::addCInterface(const void* svc, const std::string serviceName, const std::string version) {
    return this->addCInterface(svc, serviceName, version, Properties());
};

template<class T>
Component<T>& Component<T>::addCInterface(const void* svc, const std::string serviceName, const std::string version, const Properties properties) {
    properties_pt cProperties = properties_create();
    properties_set(cProperties, (char*)"lang", (char*)"c");
    for (auto iter = properties.begin(); iter != properties.end(); iter++) {
        properties_set(cProperties, (char*)iter->first.c_str(), (char*)iter->second.c_str());
    }

    const char *cVersion = version.empty() ? nullptr : version.c_str();
    component_addInterface(this->cComponent(), (char*)serviceName.c_str(), (char*)cVersion, svc, cProperties);

    return *this;
};

template<class T>
template<class I>
Component<T>& Component<T>::add(ServiceDependency<T,I>& dep) {
    component_addServiceDependency(cComponent(), dep.cServiceDependency());
    dep.setComponentInstance(&getInstance());
    return *this;
}

template<class T>
template<class I>
Component<T>& Component<T>::remove(ServiceDependency<T,I>& dep) {
    component_removeServiceDependency(cComponent(), dep.cServiceDependency());
    return *this;
}

template<class T>
template<typename I>
Component<T>& Component<T>::add(CServiceDependency<T,I>& dep) {
    component_addServiceDependency(cComponent(), dep.cServiceDependency());

    dep.setComponentInstance(&getInstance());
    return *this;
}

template<class T>
template<typename I>
Component<T>& Component<T>::remove(CServiceDependency<T,I>& dep) {
    component_removeServiceDependency(cComponent(), dep.cServiceDependency());
    return *this;
}

template<class T>
T& Component<T>::getInstance() {
    if (this->refInstance.size() == 1) {
        return refInstance.front();
    } else {
        if (this->instance.get() == nullptr) {
            this->instance = std::shared_ptr<T> {new T()};
        }
        return *this->instance.get();
    }
}

template<class T>
Component<T>& Component<T>::setInstance(std::shared_ptr<T> inst) {
    this->instance = inst;
    return *this;
}

template<class T>
Component<T>& Component<T>::setInstance(T&& inst) {
    this->refInstance.clear();
    this->refInstance.push_back(std::move(inst));
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

    component_setCallbacks(this->cComponent(), cInit, cStart, cStop, cDeinit);

    return *this;
}
