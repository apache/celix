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

using namespace celix::dm;

inline void BaseComponent::runBuild() {
    if (context == nullptr || cDepMan == nullptr) {
        return;
    }

    {
        std::lock_guard<std::mutex> lck{mutex};
        for (auto& provide : providedServices) {
            provide->runBuild();
        }
    }

    {
        std::lock_guard<std::mutex> lck{mutex};
        for (auto &dep : dependencies) {
            dep->runBuild();
        }
    }


    bool alreadyAdded = cmpAddedToDepMan.exchange(true);
    if (!alreadyAdded) {
        celix_dependencyManager_addAsync(cDepMan, cCmp);
    }
}

inline BaseComponent::~BaseComponent() noexcept = default;

inline void BaseComponent::wait() const {
    celix_dependencyManager_wait(cDepMan);
}

std::ostream& celix::dm::operator<<(std::ostream &out, const BaseComponent &cmp) {
    char* buf = nullptr;
    size_t bufSize = 0;
    FILE* stream = open_memstream(&buf, &bufSize);
    celix_dm_component_info_t* cmpInfo = nullptr;
    celix_dmComponent_getComponentInfo(cmp.cComponent(), &cmpInfo);
    celix_dmComponent_printComponentInfo(cmpInfo, true, true, stream);
    fclose(stream);
    celix_dmComponent_destroyComponentInfo(cmpInfo);
    out << buf;
    free(buf);
    return out;
}

template<class T>
Component<T>::Component(
        celix_bundle_context_t *context,
        celix_dependency_manager_t* cDepMan,
        std::string name,
        std::string uuid) : BaseComponent(
                context,
                cDepMan,
                celix::cmpTypeName<T>(name),
                std::move(uuid)) {}

template<class T>
Component<T>::~Component() = default;

template<class T>
template<class I>
Component<T>& Component<T>::addInterfaceWithName(const std::string &serviceName, const std::string &version, const Properties &properties) {
    static_assert(std::is_base_of<I,T>::value, "Component T must implement Interface I");
    if (!serviceName.empty()) {
        T* cmpPtr = &this->getInstance();
        I* svcPtr = static_cast<I*>(cmpPtr); //NOTE T should implement I
        std::shared_ptr<I> svc{svcPtr, [](I*){/*nop*/}};
        auto provide = std::make_shared<ProvidedService<T,I>>(cComponent(), serviceName, std::move(svc), true);
        provide->setVersion(version);
        provide->setProperties(properties);
        std::lock_guard<std::mutex> lck{mutex};
        providedServices.push_back(provide);
    } else {
        std::cerr << "Cannot add interface with a empty name\n";
    }

    return *this;
}

template<class T>
template<class I>
Component<T>& Component<T>::addInterface(const std::string &version, const Properties &properties) {
    static_assert(std::is_base_of<I,T>::value, "Component T must implement Interface I");
    std::string serviceName = typeName<I>();
    if (serviceName.empty()) {
        std::cerr << "Cannot add interface, because type name could not be inferred. function: '"  << __PRETTY_FUNCTION__ << "'\n";
    }
    auto svcVersion = celix::typeVersion<I>(version);
    if (!svcVersion.empty()) {
        properties[celix::SERVICE_VERSION] = std::move(svcVersion);
    }

    return this->addInterfaceWithName<I>(serviceName, version, properties);
}

template<class T>
template<class I>
Component<T>& Component<T>::addCInterface(I* svcPtr, const std::string &serviceName, const std::string &version, const Properties &properties) {
    std::shared_ptr<I> svc{svcPtr, [](I*){/*nop*/}};
    auto provide = std::make_shared<ProvidedService<T,I>>(cComponent(), serviceName, std::move(svc), false);
    provide->setVersion(version);
    provide->setProperties(properties);
    std::lock_guard<std::mutex> lck{mutex};
    providedServices.push_back(provide);
    return *this;
}

template<class T>
template<class I>
Component<T>& Component<T>::removeCInterface(const I* svc){
    celix_dmComponent_removeInterface(this->cComponent(), svc);
    std::lock_guard<std::mutex> lck{mutex};
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
    auto dep = std::shared_ptr<ServiceDependency<T,I>> {new ServiceDependency<T,I>(cComponent(), name)};
    std::lock_guard<std::mutex> lck{mutex};
    dependencies.push_back(dep);
    dep->setComponentInstance(&getInstance());
    return *dep;
}

template<class T>
template<class I>
Component<T>& Component<T>::remove(ServiceDependency<T,I>& dep) {
    celix_component_removeServiceDependency(cComponent(), dep.cServiceDependency());
    std::lock_guard<std::mutex> lck{mutex};
    this->dependencies.erase(std::remove(this->dependencies.begin(), this->dependencies.end(), dep));
    return *this;
}

template<class T>
template<typename I>
CServiceDependency<T,I>& Component<T>::createCServiceDependency(const std::string &name) {
    auto dep = std::shared_ptr<CServiceDependency<T,I>> {new CServiceDependency<T,I>(cComponent(), name)};
    std::lock_guard<std::mutex> lck{mutex};
    dependencies.push_back(dep);
    dep->setComponentInstance(&getInstance());
    return *dep;
}

template<class T>
template<typename I>
Component<T>& Component<T>::remove(CServiceDependency<T,I>& dep) {
    celix_component_removeServiceDependency(cComponent(), dep.cServiceDependency());
    std::lock_guard<std::mutex> lck{mutex};
    this->dependencies.erase(std::remove(this->dependencies.begin(), this->dependencies.end(), dep));
    return *this;
}

template<class T>
std::shared_ptr<Component<T>> Component<T>::create(celix_bundle_context_t *context, celix_dependency_manager_t* cDepMan, std::string name, std::string uuid) {
    std::string cmpName = celix::cmpTypeName<T>(name);
    return std::shared_ptr<Component<T>>{new Component<T>(context, cDepMan, std::move(cmpName), std::move(uuid)), [](Component<T>* cmp){
        //Using a callback of async destroy to ensure that the cmp instance is still exist while the
        //dm component is async disabled and destroyed.
        auto cb = [](void *data) {
            auto* c = static_cast<Component<T>*>(data);
            delete c;
        };

        if (cmp->cmpAddedToDepMan) {
            celix_dependencyManager_removeAsync(cmp->cDepMan, cmp->cCmp, cmp, cb);
        } else {
            celix_dmComponent_destroyAsync(cmp->cCmp, cmp, cb);
        }
    }};
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
    std::lock_guard<std::mutex> lck{instanceMutex};
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
    std::lock_guard<std::mutex> lck{instanceMutex};
    this->valInstance.clear();
    this->instance = std::unique_ptr<T> {nullptr};
    this->sharedInstance = std::move(inst);
    return *this;
}

template<class T>
Component<T>& Component<T>::setInstance(std::unique_ptr<T>&& inst) {
    std::lock_guard<std::mutex> lck{instanceMutex};
    this->valInstance.clear();
    this->sharedInstance = std::shared_ptr<T> {nullptr};
    this->instance = std::move(inst);
    return *this;
}

template<class T>
Component<T>& Component<T>::setInstance(T&& inst) {
    std::lock_guard<std::mutex> lck{instanceMutex};
    this->instance = std::unique_ptr<T> {nullptr};
    this->sharedInstance = std::shared_ptr<T> {nullptr};
    this->valInstance.clear();
    this->valInstance.push_back(std::forward<T>(inst));
    return *this;
}

template<class T>
int Component<T>::invokeLifecycleMethod(const std::string& methodName, void (T::*lifecycleMethod)()) {
    try {
        if (lifecycleMethod != nullptr) {
            T& inst = getInstance();
            (inst.*lifecycleMethod)();
        }
    } catch (const std::exception& e) {
        celix_bundleContext_log(context, CELIX_LOG_LEVEL_ERROR, "Error invoking %s for component %s (uuid=%s). Exception: %s",
                                methodName.c_str(),
                                cmpName.c_str(),
                                cmpUUID.c_str(),
                                e.what());
        return -1;
    } catch (...) {
        celix_bundleContext_log(context, CELIX_LOG_LEVEL_ERROR, "Error invoking %s for component %s (uuid=%s).",
                                methodName.c_str(),
                                cmpName.c_str(),
                                cmpUUID.c_str());
        return -1;
    }
    return 0;
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
        return cmp->invokeLifecycleMethod("init", cmp->initFp);
    };
    int (*cStart)(void *) = [](void *handle) {
        Component<T>* cmp = (Component<T>*)(handle);
        return cmp->invokeLifecycleMethod("start", cmp->startFp);
    };
    int (*cStop)(void *) = [](void *handle) {
        Component<T>* cmp = (Component<T>*)(handle);
        return cmp->invokeLifecycleMethod("stop", cmp->stopFp);
    };
    int (*cDeinit)(void *) = [](void *handle) {
        Component<T>* cmp = (Component<T>*)(handle);
        return cmp->invokeLifecycleMethod("deinit", cmp->deinitFp);
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

template<class T>
Component<T>& Component<T>::addContext(std::shared_ptr<void> context) {
    std::lock_guard<std::mutex> lock{mutex};
    componentContexts.emplace_back(std::move(context));
    return *this;
}

template<typename T>
Component<T>& Component<T>::build() {
    runBuild();
    wait();
    return *this;
}

template<typename T>
Component<T>& Component<T>::buildAsync() {
    runBuild();
    return *this;
}

template<class T>
template<class I>
ProvidedService<T, I> &Component<T>::createProvidedCService(I *svcPtr, std::string serviceName) {
    std::shared_ptr<I> svc{svcPtr, [](I*){/*nop*/}};
    auto provide = std::make_shared<ProvidedService<T,I>>(cComponent(), serviceName, std::move(svc), false);
    std::lock_guard<std::mutex> lck{mutex};
    providedServices.push_back(provide);
    return *provide;
}

template<class T>
template<class I>
ProvidedService<T, I>& Component<T>::createProvidedService(std::string serviceName) {
    static_assert(std::is_base_of<I,T>::value, "Component T must implement Interface I");
    if (serviceName.empty()) {
        serviceName = typeName<I>();
    }
    if (serviceName.empty()) {
        std::cerr << "Cannot add interface, because type name could not be inferred. function: '"  << __PRETTY_FUNCTION__ << "'\n";
    }

    I* svcPtr = &this->getInstance();
    std::shared_ptr<I> svc{svcPtr, [](I*){/*nop*/}};
    auto provide = std::make_shared<ProvidedService<T,I>>(cComponent(), serviceName, std::move(svc), true);
    auto svcVersion = celix::typeVersion<I>();
    if (!svcVersion.empty()) {
        provide->addProperty(celix::SERVICE_VERSION, std::move(svcVersion));
    }
    std::lock_guard<std::mutex> lck{mutex};
    providedServices.push_back(provide);
    return *provide;
}

template<class T>
template<class I>
ProvidedService<T, I>& Component<T>::createUnassociatedProvidedService(std::shared_ptr<I> svc, std::string serviceName) {
    if (serviceName.empty()) {
        serviceName = typeName<I>();
    }
    if (serviceName.empty()) {
        std::cerr << "Cannot add interface, because type name could not be inferred. function: '"  << __PRETTY_FUNCTION__ << "'\n";
    }

    auto provide = std::make_shared<ProvidedService<T,I>>(cComponent(), serviceName, std::move(svc), true);
    auto svcVersion = celix::typeVersion<I>();
    if (!svcVersion.empty()) {
        provide->addProperty(celix::SERVICE_VERSION, std::move(svcVersion));
    }
    std::lock_guard<std::mutex> lck{mutex};
    providedServices.push_back(provide);
    return *provide;
}

