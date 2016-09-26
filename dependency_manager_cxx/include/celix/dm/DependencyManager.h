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
    private:
        bundle_context_pt context = {nullptr};
        std::list<std::unique_ptr<BaseComponent>> components {};
        dm_dependency_manager_pt cDepMan {nullptr};
    public:
        DependencyManager(bundle_context_pt context);
        virtual ~DependencyManager();

        const bundle_context_pt bundleContext() const;
        const dm_dependency_manager_pt cDependencyManager() const;

        /**
         * Creates and adds a new DM Component for a component of type T and instance inst
         * If inst if nullptr lazy initializion is used.
         *
         * @return Returns a reference to the DM Component
         */
        template<class T>
        Component<T>& createComponent(std::shared_ptr<T> inst = std::shared_ptr<T>{nullptr}) {
            Component<T>* cmp = Component<T>::create(this->context);
            if (cmp->isValid()) {
                this->components.push_back(std::unique_ptr<BaseComponent> {cmp});
            }
            return *cmp;
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
