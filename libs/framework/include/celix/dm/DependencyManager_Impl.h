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

#include <cassert>
#include "DependencyManager.h"

using namespace celix::dm;

inline DependencyManager::DependencyManager(celix_bundle_context_t *ctx) :
    context{ctx, [](celix_bundle_context_t*){/*nop*/}},
    cDepMan{celix_bundleContext_getDependencyManager(ctx), [](celix_dependency_manager_t*){/*nop*/}} {}

inline DependencyManager::~DependencyManager() {
    clearAsync();
}

template<class T>
Component<T>& DependencyManager::createComponentInternal(std::string name, std::string uuid) {
    auto cmp = Component<T>::create(this->context.get(), this->cDepMan.get(), std::move(name), std::move(uuid));
    if (cmp->isValid()) {
        auto baseCmp = std::static_pointer_cast<BaseComponent>(cmp);
        std::lock_guard<std::mutex> lck{mutex};
        this->components.push_back(baseCmp);
    }

    return *cmp;
}

template<class T>
inline
typename std::enable_if<std::is_default_constructible<T>::value, Component<T>&>::type
DependencyManager::createComponent(std::string name, std::string uuid) {
    return createComponentInternal<T>(std::move(name), std::move(uuid));
}

template<class T>
Component<T>& DependencyManager::createComponent(std::unique_ptr<T>&& rhs, std::string name, std::string uuid) {
    return createComponentInternal<T>(std::move(name), std::move(uuid)).setInstance(std::move(rhs));
}

template<class T>
Component<T>& DependencyManager::createComponent(std::shared_ptr<T> rhs, std::string name, std::string uuid) {
    return createComponentInternal<T>(std::move(name), std::move(uuid)).setInstance(rhs);
}

template<class T>
Component<T>& DependencyManager::createComponent(T rhs, std::string name, std::string uuid) {
    return createComponentInternal<T>(std::move(name), std::move(uuid)).setInstance(std::forward<T>(rhs));
}

inline void DependencyManager::start() {
    build();
}

inline void DependencyManager::build() {
    buildAsync();
    wait();
}

inline void DependencyManager::buildAsync() {
    std::lock_guard<std::mutex> lck{mutex};
    for (auto& cmp : components) {
        cmp->runBuild();
    }
}

inline void DependencyManager::destroyComponent(BaseComponent &component) {
    removeComponent(component.getUUID());
}

inline bool DependencyManager::removeComponent(const std::string& uuid) {
    bool removed = removeComponentAsync(uuid);
    wait();
    return removed;
}

inline bool DependencyManager::removeComponentAsync(const std::string& uuid) {
    std::shared_ptr<BaseComponent> tmpStore{};
    {
        std::lock_guard<std::mutex> lck{mutex};
        for (auto it = components.begin(); it != components.end(); ++it) {
            if ( (*it)->getUUID() == uuid) {
                //found
                tmpStore = *it; //prevents destruction in lock
                components.erase(it);
                break;
            }
        }
    }
    return tmpStore != nullptr;
}

inline void DependencyManager::clear() {
    clearAsync();
    wait();
}

inline void DependencyManager::clearAsync() {
    std::vector<std::shared_ptr<BaseComponent>> swappedComponents{};
    {
        std::lock_guard<std::mutex> lck{mutex};
        std::swap(swappedComponents, components);
    }
    swappedComponents.clear();
}

inline void DependencyManager::wait() const {
    celix_dependencyManager_wait(cDepMan.get());
}

inline void DependencyManager::waitIfAble() const {
    auto* fw = celix_bundleContext_getFramework(context.get());
    if (!celix_framework_isCurrentThreadTheEventLoop(fw)) {
        celix_dependencyManager_wait(cDepMan.get());
    }
}

inline void DependencyManager::stop() {
    clear();
}

inline std::size_t DependencyManager::getNrOfComponents() const {
    return celix_dependencyManager_nrOfComponents(cDepMan.get());
}

template<typename T>
inline std::shared_ptr<Component<T>> DependencyManager::findComponent(const std::string& uuid) const {
    std::lock_guard<std::mutex> lck{mutex};
    std::shared_ptr<BaseComponent> found{nullptr};
    for (const auto& cmp : components) {
        if (cmp->getUUID() == uuid) {
            found = cmp;
        }
    }
    if (found) {
        return std::static_pointer_cast<Component<T>>(found);
    } else {
        return nullptr;
    }
}

static celix::dm::DependencyManagerInfo createDepManInfoFromC(celix_dependency_manager_info_t* cInfo) {
    celix::dm::DependencyManagerInfo info{};
    info.bndId = cInfo->bndId;
    info.bndSymbolicName = std::string{cInfo->bndSymbolicName};

    for (int i = 0; i < celix_arrayList_size(cInfo->components); ++i) {
        auto* cCmpInfo = static_cast<dm_component_info_t*>(celix_arrayList_get(cInfo->components, i));
        celix::dm::ComponentInfo cmpInfo{};
        cmpInfo.uuid = std::string{cCmpInfo->id};
        cmpInfo.name = std::string{cCmpInfo->name};
        cmpInfo.isActive = cCmpInfo->active;
        cmpInfo.state = std::string{cCmpInfo->state};
        cmpInfo.nrOfTimesStarted = cCmpInfo->nrOfTimesStarted;
        cmpInfo.nrOfTimesResumed = cCmpInfo->nrOfTimesResumed;

        for (int k = 0; k < celix_arrayList_size(cCmpInfo->interfaces); ++k) {
            auto* cIntInfo = static_cast<dm_interface_info_t*>(celix_arrayList_get(cCmpInfo->interfaces, k));
            celix::dm::InterfaceInfo intInfo{};
            intInfo.serviceName = std::string{cIntInfo->name};
            CELIX_PROPERTIES_ITERATE(cIntInfo->properties, iter) {
                intInfo.properties[std::string{iter.key}] = std::string{iter.entry.value};
            }
            cmpInfo.interfacesInfo.emplace_back(std::move(intInfo));
        }

        for (int k = 0; k < celix_arrayList_size(cCmpInfo->dependency_list); ++k) {
            auto *cDepInfo = static_cast<dm_service_dependency_info_t *>(celix_arrayList_get(cCmpInfo->dependency_list, k));
            celix::dm::ServiceDependencyInfo depInfo{};
            depInfo.serviceName = std::string{cDepInfo->serviceName == nullptr ? "" : cDepInfo->serviceName};
            depInfo.filter = std::string{cDepInfo->filter == nullptr ? "" : cDepInfo->filter};
            depInfo.versionRange = std::string{cDepInfo->versionRange == nullptr ? "" : cDepInfo->versionRange};
            depInfo.isAvailable = cDepInfo->available;
            depInfo.isRequired = cDepInfo->required;
            depInfo.nrOfTrackedServices = cDepInfo->count;
            cmpInfo.dependenciesInfo.emplace_back(std::move(depInfo));
        }

        info.components.emplace_back(std::move(cmpInfo));
    }
    return info;
}

inline celix::dm::DependencyManagerInfo DependencyManager::getInfo() const {
    auto* cInfo = celix_dependencyManager_createInfo(cDependencyManager(), celix_bundleContext_getBundleId(context.get()));
    if (cInfo) {
        auto result = createDepManInfoFromC(cInfo);
        celix_dependencyManager_destroyInfo(cDependencyManager(), cInfo);
        return result;
    } else {
        return {};
    }
}

inline std::vector<celix::dm::DependencyManagerInfo> DependencyManager::getInfos() const {
    std::vector<celix::dm::DependencyManagerInfo> result{};
    auto* cInfos = celix_dependencyManager_createInfos(cDependencyManager());
    for (int i = 0; i < celix_arrayList_size(cInfos); ++i) {
        auto* cInfo = static_cast<celix_dependency_manager_info_t*>(celix_arrayList_get(cInfos, i));
        result.emplace_back(createDepManInfoFromC(cInfo));
    }
    celix_dependencyManager_destroyInfos(cDependencyManager(), cInfos);
    return result;
}

std::ostream& celix::dm::operator<<(std::ostream &out, const DependencyManager &mng) {
    char* buf = nullptr;
    size_t bufSize = 0;
    FILE* stream = open_memstream(&buf, &bufSize);
    celix_dependencyManager_printInfo(mng.cDepMan.get(), true, true, stream);
    fclose(stream);
    out << buf;
    free(buf);
    return out;
}
