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

#include "DependencyManager.h"

using namespace celix::dm;

inline DependencyManager::DependencyManager(celix_bundle_context_t *ctx) :
    context{ctx, [](celix_bundle_context_t*){/*nop*/}},
    cDepMan{celix_bundleContext_getDependencyManager(ctx), [](celix_dependency_manager_t*){/*nop*/}} {}

inline DependencyManager::~DependencyManager() {/*nop*/}

template<class T>
Component<T>& DependencyManager::createComponentInternal(std::string name) {
    Component<T>* cmp = name.empty() ?
                        Component<T>::create(this->context.get(), this->cDepMan.get()) :
                        Component<T>::create(this->context.get(), this->cDepMan.get(), name);
    if (cmp->isValid()) {
        this->components.push_back(std::unique_ptr<BaseComponent> {cmp});
    }
    return *cmp;
}

template<class T>
inline
typename std::enable_if<std::is_default_constructible<T>::value, Component<T>&>::type
DependencyManager::createComponent(std::string name) {
    return createComponentInternal<T>(name);
}

template<class T>
Component<T>& DependencyManager::createComponent(std::unique_ptr<T>&& rhs, std::string name) {
    return createComponentInternal<T>(name).setInstance(std::move(rhs));
}

template<class T>
Component<T>& DependencyManager::createComponent(std::shared_ptr<T> rhs, std::string name) {
    return createComponentInternal<T>(name).setInstance(rhs);
}

template<class T>
Component<T>& DependencyManager::createComponent(T rhs, std::string name) {
    return createComponentInternal<T>(name).setInstance(std::forward<T>(rhs));
}

inline void DependencyManager::start() {
    build();
}

inline void DependencyManager::build() {
    for (auto& cmp : components) {
        cmp->runBuild();
    }
}

template<typename T>
void DependencyManager::destroyComponent(Component<T> &component) {
    celix_dependencyManager_remove(cDepMan.get(), component.cComponent());
}

inline void DependencyManager::clear() {
    celix_dependencyManager_removeAllComponents(cDepMan.get());
    components.clear();
}

inline void DependencyManager::stop() {
    clear();
}

inline std::size_t DependencyManager::getNrOfComponents() const {
    return celix_dependencyManager_nrOfComponents(cDepMan.get());
}

