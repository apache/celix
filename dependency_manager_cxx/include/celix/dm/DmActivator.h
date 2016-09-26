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

#ifndef CELIX_DM_ACTIVATOR_H
#define CELIX_DM_ACTIVATOR_H

#include "celix/dm/DependencyManager.h"

namespace celix { namespace dm {

    class DmActivator {
    protected:
        DependencyManager& manager;
        DmActivator(DependencyManager& mng) : manager(mng) {}
    public:
        ~DmActivator() = default;

        /**
         * The init of the DM Activator. Should be overridden by the bundle specific DM activator.
         *
         * @param manager A reference to the  Dependency Manager
         */
        virtual void init(DependencyManager& manager) {};

        /**
         * The init of the DM Activator. Can be overridden by the bundle specific DM activator.
         *
         * @param manager A reference to the  Dependency Manager
         */
        virtual void deinit(DependencyManager& manager) {};

        /**
         * Creates and adds a new DM Component for a component of type T.
         * If inst is provided the DM Component will manage provided instance.
         * If inst is not provided (nullptr) the DM Component will lazy contsruct a new instance if needed.
         *
         * @return Returns a reference to the DM Component
         */
        template< class T>
        Component<T>& createComponent(std::shared_ptr<T> inst = std::shared_ptr<T>{nullptr}) { return manager.createComponent<T>(inst); }

        /**
         * The static method to create a new DM activator.
         * NOTE that this method in intentionally not implemented in the C++ Dependency Manager library.
         * This should be done by the bundle specific DM activator.
         *
         * @param mng A reference to the Dependency Manager
         * @returns A pointer to a DmActivator. The Dependency Manager is responsible for deleting the pointer when the bundle is stopped.
         */
        static DmActivator* create(DependencyManager& mng);
    };
}}

#endif //CELIX_DM_ACTIVATOR_H
