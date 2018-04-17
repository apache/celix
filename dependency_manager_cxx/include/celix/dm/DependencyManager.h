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

#ifndef CELIX_DM_DEPENDENCYMANAGER_H
#define CELIX_DM_DEPENDENCYMANAGER_H


#include "celix/dm/types.h"
#include "celix/dm/Component.h"
#include "celix/dm/ServiceDependency.h"

#include "bundle_context.h"
#include "dm_dependency_manager.h"

#include <vector>
#include <mutex>

namespace celix { namespace dm {

    class DependencyManager {
    public:
        DependencyManager(bundle_context_pt ctx) : context(ctx) {
                this->cDepMan = nullptr;
                dependencyManager_create(context, &this->cDepMan);
        }

        virtual ~DependencyManager() {
                dependencyManager_destroy(this->cDepMan);
                this->cDepMan = nullptr;
        }

        DependencyManager(DependencyManager&& mgr) : componentsMutex{} {
                std::lock_guard<std::mutex> lock(componentsMutex);
                mgr.context = context;
                mgr.queuedComponents = std::move(queuedComponents);
                mgr.startedComponents = std::move(startedComponents);
                mgr.cDepMan = cDepMan;
                cDepMan = nullptr;
                context = nullptr;
        }
        DependencyManager& operator=(DependencyManager&&) = default;

        DependencyManager(const DependencyManager&) = delete;
        DependencyManager& operator=(const DependencyManager&) = delete;

        bundle_context_pt bundleContext() const { return context; }
        dm_dependency_manager_pt cDependencyManager() const { return cDepMan; }


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
                std::lock_guard<std::mutex> lock(componentsMutex);
                for (auto it = queuedComponents.begin(); it != queuedComponents.end(); ++it) {
                        dependencyManager_add(cDepMan, (*it)->cComponent());
                        startedComponents.push_back(std::move(*it));
                }
                queuedComponents.clear();
        }

        /**
         * Stops the Dependency Manager
         */
        void stop() {
                dependencyManager_removeAllComponents(cDepMan);
                queuedComponents.clear();
                startedComponents.clear();
        }
    private:
        bundle_context_pt context {nullptr};
        std::vector<std::unique_ptr<BaseComponent>> queuedComponents {};
        std::vector<std::unique_ptr<BaseComponent>> startedComponents {};
        dm_dependency_manager_pt cDepMan {nullptr};
        std::mutex componentsMutex{};
    };

}}

#include "celix/dm/DependencyManager_Impl.h"

#endif //CELIX_DM_DEPENDENCYMANAGER_H
