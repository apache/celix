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

#pragma once

#include "celix/dm/types.h"
#include "celix/dm/Component.h"
#include "celix/dm/ServiceDependency.h"

#include "bundle_context.h"
#include "celix_bundle_context.h"
#include "celix_dependency_manager.h"

#include <vector>

namespace celix { namespace dm {

    /**
     * The Dependency Manager, Component, ServiceDependency and Properties are not thread safe!
     */
    class DependencyManager {
    public:
        DependencyManager(celix_bundle_context_t *ctx);

        virtual ~DependencyManager();

        DependencyManager(DependencyManager&& mgr) = default;
        DependencyManager& operator=(DependencyManager&& rhs) = default;

        DependencyManager(const DependencyManager&) = delete;
        DependencyManager& operator=(const DependencyManager&) = delete;

        celix_bundle_context_t* bundleContext() const { return context.get(); }
        celix_dependency_manager_t *cDependencyManager() const { return cDepMan.get(); }


        /**
         * Creates and adds a new DM Component for a component of type T.
         *
         * @return Returns a reference to the DM Component
         */
        template<class T>
        typename std::enable_if<std::is_default_constructible<T>::value, Component<T>&>::type
        createComponent(std::string name = std::string{});

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
         * Build the dependency manager.
         *
         * When building the dependency manager all created components are build.
         * A build is needed to to enable the components.
         * This is not done automatically so that user can firs construct component with their provided
         * service and service dependencies and when to components are complete call the build.
         *
         * If a component is updated after the dependency manager build is called, an new build call will result in
         * that the changes to the component are enabled.
         */
        void build();

        /**
        * Starts the Dependency Manager
        * Note this call is considered deprecated, please use build() instead.
        */
        void start();


        /**
         * Removes a component from the  Dependency Manager and destroys it
         */
        template<typename T>
        void destroyComponent(Component<T> &component);

        /**
         * Clears the dependency manager, which removes all configured components.
         */
        void clear();

        /**
         * Stops the Dependency Manager
         * Note this call is considered deprecated, please use clear() instead.
         */
        void stop();

        /**
         * Returns the nr of configured components for this dependency manager.
         */
        std::size_t getNrOfComponents() const;
    private:
        template<class T>
        Component<T>& createComponentInternal(std::string name);

        std::shared_ptr<celix_bundle_context_t> context;
        std::shared_ptr<celix_dependency_manager_t> cDepMan;
        std::vector<std::shared_ptr<BaseComponent>> components {};
    };

}}

#include "celix/dm/DependencyManager_Impl.h"
