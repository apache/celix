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
    clear();
}

template<class T>
Component<T>& DependencyManager::createComponentInternal(std::string name, std::string uuid) {
    auto cmp = Component<T>::create(this->context.get(), this->cDepMan.get(), std::move(name), std::move(uuid));
    if (cmp->isValid()) {
        auto baseCmp = std::static_pointer_cast<BaseComponent>(cmp);
        this->components.push_back(baseCmp);
    }

    return *cmp;
}

template<class T>
inline
typename std::enable_if<std::is_default_constructible<T>::value, Component<T>&>::type
DependencyManager::createComponent(std::string name, std::string uuid) {
    return createComponentInternal<T>(name, uuid);
}

template<class T>
Component<T>& DependencyManager::createComponent(std::unique_ptr<T>&& rhs, std::string name, std::string uuid) {
    return createComponentInternal<T>(name, uuid).setInstance(std::move(rhs));
}

template<class T>
Component<T>& DependencyManager::createComponent(std::shared_ptr<T> rhs, std::string name, std::string uuid) {
    return createComponentInternal<T>(name, uuid).setInstance(rhs);
}

template<class T>
Component<T>& DependencyManager::createComponent(T rhs, std::string name, std::string uuid) {
    return createComponentInternal<T>(name, uuid).setInstance(std::forward<T>(rhs));
}

inline void DependencyManager::start() {
    build();
}

inline void DependencyManager::build() {
    for (auto& cmp : components) {
        cmp->runBuild();
    }
    wait();
}

template<typename T>
void DependencyManager::destroyComponent(Component<T> &component) {
    for (auto it = components.begin(); it != components.end(); ++it) {
        if ( (*it).get() == &component) {
            //found
            components.erase(it);
            break;
        }
    }
    wait();
}

inline void DependencyManager::clear() {
    components.clear();
    wait();
}

inline void DependencyManager::wait() const {
    if (context) {
        auto *fw = celix_bundleContext_getFramework(context.get());
        if (!celix_framework_isCurrentThreadTheEventLoop(fw)) {
            celix_framework_waitForEmptyEventQueue(fw);
        }
    }
}

inline void DependencyManager::stop() {
    clear();
}

inline std::size_t DependencyManager::getNrOfComponents() const {
    return celix_dependencyManager_nrOfComponents(cDepMan.get());
}

template<typename T>
inline std::shared_ptr<Component<T>> DependencyManager::findComponent(const std::string& uuid) const  {
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

