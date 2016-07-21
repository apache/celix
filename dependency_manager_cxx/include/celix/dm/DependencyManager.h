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

#ifndef CELIX_DEPENDENCYMANAGER_H
#define CELIX_DEPENDENCYMANAGER_H


#include "celix/dm/types.h"
#include "celix/dm/Component.h"
#include "celix/dm/ServiceDependency.h"

extern "C" {
#include "bundle_context.h"
#include "dm_dependency_manager.h"
}

#include <list>

namespace celix { namespace dm {

    class DependencyManager {
        bundle_context_pt context = {nullptr};
        std::list<std::shared_ptr<BaseComponent>> components {};
        std::list<BaseComponent*> addedComponents {};
        dm_dependency_manager_pt cDepMan {nullptr};
        std::list<std::shared_ptr<BaseServiceDependency>> dependencies {};
    public:
        DependencyManager(bundle_context_pt context);
        virtual ~DependencyManager();

        const bundle_context_pt bundleContext() const;
        const dm_dependency_manager_pt cDependencyManager() const;

        /**
         * Create a new DM Component for a component of type T
         *
         * @return Returns a reference to the DM Component
         */
        template<class T>
        Component<T>& createComponent() {
            std::shared_ptr<Component<T>> cmp {new Component<T>(this->context, typeName<T>())};
            components.push_back(cmp);
            return *cmp;
        }

        /**
         * Adds a DM Component to the Dependency Manager
         */
        template<class T>
        void add(Component<T>& cmp) {
            addedComponents.push_back(&cmp);
        }

        /**
         * Removes a DM Component to the Dependency Manager
         */
        template<class T>
        void remove(Component<T>& cmp) {
            addedComponents.remove(&cmp);
        }

        /**
         * Create a new C++ service dependency for a component of type T with an interface of type I
         *
         * @return Returns a reference to the service dependency
         */
        template<class T, class I>
        ServiceDependency<T,I>& createServiceDependency() {
            auto dep = std::shared_ptr<ServiceDependency<T,I>> {new ServiceDependency<T,I>()};
            dependencies.push_back(dep);
            return *dep;
        };


        /**
         * Create a new C service dependency for a component of type T.
         *
         * @return Returns a reference to the service dependency
         */
        template<class T>
        CServiceDependency<T>& createCServiceDependency() {
            auto dep = std::shared_ptr<CServiceDependency<T>> {new CServiceDependency<T>()};
            dependencies.push_back(dep);
            return *dep;
        }

        /**
         * Starts the Dependency Manager
         */
        void start();

        /**
         * Stops the Dependency Manager
         */
        void stop();
    };

}}

#endif //CELIX_DEPENDENCYMANAGER_H
