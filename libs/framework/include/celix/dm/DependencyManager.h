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

#include <cstdio>
#include <iostream>
#include <vector>

#include "celix/dm/types.h"
#include "celix/dm/Component.h"
#include "celix/dm/ServiceDependency.h"
#include "celix/dm/DependencyManagerInfo.h"

#include "bundle_context.h"
#include "celix_bundle_context.h"
#include "celix_dependency_manager.h"

namespace celix { namespace dm {

    /**
     *
     * The build() methods For celix::dm::DependencyManager, celix::dm::Component and
     * celix::dm::(C)ServiceDependency should be called outside the Celix event thread.
     * Note that bundle activators are started and stopped outside the Celix event thread and thus the build()
     * methods can be used in bundle activators.
     *
     * Inside the Celix event thread - i.e. during a useService callback or add/rem/set call from a service tracker -
     * the buildAsync version should be used. After a buildASync method has returned, service registration and opening
     * service trackers are (possibly) still in progress.
     */
    class DependencyManager {
    public:
        DependencyManager(celix_bundle_context_t *ctx);

        virtual ~DependencyManager();

        DependencyManager(DependencyManager&& mgr) = delete;
        DependencyManager& operator=(DependencyManager&& rhs) = delete;

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
        createComponent(std::string name = std::string{}, std::string uuid = {});

        /**
         * Creates and adds a new DM Component for a component of type T and setting
         * the instance using a unique ptr.
         *
         * @return Returns a reference to the DM Component
         */
        template<class T>
        Component<T>& createComponent(std::unique_ptr<T>&& rhs, std::string name = std::string{}, std::string uuid = {});

        /**
         * Creates and adds a new DM Component for a component of type T and setting
         * the instance using a shared ptr.
         *
         * @return Returns a reference to the DM Component
         */
        template<class T>
        Component<T>& createComponent(std::shared_ptr<T> rhs, std::string name = std::string{}, std::string uuid = {});

        /**
         * Creates and adds a new DM Component for a component of type T and setting
         * the instance.
         *
         * @return Returns a reference to the DM Component
         */
        template<class T>
        Component<T>& createComponent(T rhs, std::string name = std::string{}, std::string uuid = {});

        /**
         * Build the dependency manager.
         *
         * When building the dependency manager all created components are build.
         * A build is needed to to enable the components.
         * This is not done automatically so that user can first construct component with their provided
         * service and service dependencies and when to components are complete call the build.
         *
         * If a component is updated after the dependency manager build is called, an new build call will result in
         * that the changes to the component are enabled.
         *
         * After this call the components will be created and if the components can be started, they
         * will be started and the services will be registered.
         *
         * Should not be called from the Celix event thread.
         */
        void build();

        /**
         * Same as build, but this call will not wait until all service registrations and tracker are registered/opened
         * on the Celix event thread.
         * Can be called on the Celix event thread.
         */
         void buildAsync();

        /**
        * Starts the Dependency Manager
        * Note this call is considered deprecated, please use build() instead.
        */
        void start();

        /**
         * @brief Wait for an empty Celix event queue.
         *
         * Should not be called on the Celix event queue thread.
         *
         * Can be used to ensure that all created/updated components are completely processed (services registered
         * and/or service trackers are created).
         */
        void wait() const;

        /**
         * @brief Wait (if not called on the Celix event thread) for an empty Celix event queue
         *
         * Can be used to ensure that all created/updated components are completely processed (services registered
         * and/or service trackers are created).
         */
        void waitIfAble() const;

        /**
         * @brief Removes a component from the Dependency Manager and wait until the component is destroyed.
         *
         * The use of removeComponent is preferred.
         */
        void destroyComponent(BaseComponent &component);


        /**
         * @brief Clears the dependency manager, which removes all configured components and waits until all components
         * are removed from the dependency manager.
         *
         * Should not be called from the Celix event thread.
         */
        void clear();

        /**
         * Clears the dependency manager, which removes all configured components.
         *
         * This is done async and this can be called from the Celix event thread.
         */
        void clearAsync();

        /**
         * Stops the Dependency Manager
         * Note this call is considered deprecated, please use clear() instead.
         */
        void stop();

        /**
         * Returns the nr of configured components for this dependency manager.
         */
        std::size_t getNrOfComponents() const;

        /**
         * Tries to find the component with UUID and statically cast it to
         * dm component of type T
         * @return pointer to found component or null if the component cannot be found.
         */
        template<typename T>
        std::shared_ptr<Component<T>> findComponent(const std::string& uuid) const;

        /**
         * @brief Removes component with provided UUID from the dependency manager and wail until the component is destroyed
         *
         * Should not be called from the Celix event thread.
         *
         * @return whether the component is found and removed.
         */
        bool removeComponent(const std::string& uuid);

        /**
         * @brief Removes a component from the Dependency Manager and destroys it async.
         *
         * Can be called from the Celix event thread.
         *
         * @return whether the component is found and removed.
         */
        bool removeComponentAsync(const std::string& uuid);

        /**
         * Get Dependency Management info for this component manager.
         * @return A DependencyManagerInfo struct.
         */
        celix::dm::DependencyManagerInfo getInfo() const;

        /**
         * Get Dependency Management info for all component manager (for all bundles).
         * @return A vector of DependencyManagerInfo structs.
         */
        std::vector<celix::dm::DependencyManagerInfo> getInfos() const;

        friend std::ostream& operator<<(std::ostream& out, const DependencyManager& mng);
    private:
        template<class T>
        Component<T>& createComponentInternal(std::string name, std::string uuid);

        std::shared_ptr<celix_bundle_context_t> context;
        std::shared_ptr<celix_dependency_manager_t> cDepMan;
        mutable std::mutex mutex{}; //protects below
        std::vector<std::shared_ptr<BaseComponent>> components{};
    };

    /**
     * Stream outputs the full dependency manager info.
     */
    inline std::ostream& operator<<(std::ostream& out, const DependencyManager& mng);
}}

#include "celix/dm/DependencyManager_Impl.h"
