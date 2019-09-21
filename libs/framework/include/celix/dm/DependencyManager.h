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

#include "celix/dm/types.h"
#include "celix/dm/Component.h"
#include "celix/dm/ServiceDependency.h"

#include "bundle_context.h"
#include "celix_bundle_context.h"
#include "celix_dependency_manager.h"

#include <vector>
#include <mutex>

#ifndef CELIX_DM_DEPENDENCYMANAGER_H
#define CELIX_DM_DEPENDENCYMANAGER_H

namespace celix { namespace dm {

    class DependencyManager {
    public:
        DependencyManager(celix_bundle_context_t *ctx) : context(ctx) {
                this->cDepMan = celix_bundleContext_getDependencyManager(ctx);
        }

        virtual ~DependencyManager() {
                this->cDepMan = nullptr;
        }

        DependencyManager(DependencyManager&& mgr) : componentsMutex{} {
                std::lock_guard<std::recursive_mutex> lock(this->componentsMutex);
                mgr.context = this->context;
                mgr.queuedComponents = std::move(this->queuedComponents);
                mgr.startedComponents = std::move(this->startedComponents);
                mgr.cDepMan = this->cDepMan;
                this->cDepMan = nullptr;
                this->context = nullptr;
        }
        DependencyManager& operator=(DependencyManager&&) = default;

        DependencyManager(const DependencyManager&) = delete;
        DependencyManager& operator=(const DependencyManager&) = delete;

        celix_bundle_context_t* bundleContext() const { return context; }
        celix_dependency_manager_t *cDependencyManager() const { return cDepMan; }


        /**
         * Creates and adds a new DM Component for a component of type T.
         *
         * @return Returns a reference to the DM Component
         */
        template<class T>
        Component<T>& createComponent(std::string name = std::string{});

        /**
         * Creates and adds a new DM Component for a component of type T and setting
         * the instance using a unique ptr.
         *
         * @return Returns a reference to the DM Component
         */
        template<class T>
        Component<T>& createComponent(std::unique_ptr<T>&& rhs, std::string name = std::string{});

        /**
         * Creates and adds a new DM Component for a component of type T and setting
         * the instance using a shared ptr.
         *
         * @return Returns a reference to the DM Component
         */
        template<class T>
        Component<T>& createComponent(std::shared_ptr<T> rhs, std::string name = std::string{});

        /**
         * Creates and adds a new DM Component for a component of type T and setting
         * the instance.
         *
         * @return Returns a reference to the DM Component
         */
        template<class T>
        Component<T>& createComponent(T rhs, std::string name = std::string{});

        /**
         * Starts the Dependency Manager
         */
        void start() {
                std::vector<std::unique_ptr<BaseComponent>> toBeStartedComponents {};
                {
                        std::lock_guard<std::recursive_mutex> lock(componentsMutex);
                        for (auto it = queuedComponents.begin(); it != queuedComponents.end(); ++it) {
                                toBeStartedComponents.push_back(std::move(*it));
                        }
                        queuedComponents.clear();
                }
                for (auto it = toBeStartedComponents.begin(); it != toBeStartedComponents.end(); ++it) {

                        celix_dependencyManager_add(cDepMan, (*it)->cComponent());
                        {
                                std::lock_guard<std::recursive_mutex> lock(componentsMutex);
                                startedComponents.push_back(std::move(*it));
                        }
                }
        }

        /**
         * Removes a component from the  Dependency Manager and destroys it
         */
        template<typename T>
        void destroyComponent(Component<T> &component) {
            celix_dependencyManager_remove(cDepMan, component.cComponent());
        }

        /**
         * Stops the Dependency Manager
         */
        void stop() {
            std::vector<std::unique_ptr<BaseComponent>> clearStarted{};
            std::vector<std::unique_ptr<BaseComponent>> clearQueued{};
            celix_dependencyManager_removeAllComponents(cDepMan);
            {
                std::lock_guard<std::recursive_mutex> lock(componentsMutex);
                std::swap(startedComponents, clearStarted);
                std::swap(queuedComponents, clearQueued);
            }
            clearStarted.clear();
            clearQueued.clear();
        }
    private:
        celix_bundle_context_t *context {nullptr};
        std::vector<std::unique_ptr<BaseComponent>> queuedComponents {};
        std::vector<std::unique_ptr<BaseComponent>> startedComponents {};
        celix_dependency_manager_t* cDepMan {nullptr};
        std::recursive_mutex componentsMutex{};
    };

}}

#include "celix/dm/DependencyManager_Impl.h"

#endif //CELIX_DM_DEPENDENCYMANAGER_H
