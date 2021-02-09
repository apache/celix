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

#include <string>
#include <vector>
#include <unordered_map>

namespace celix { namespace dm {

    /**
     * Trivial struct containing interface info.
     */
    struct InterfaceInfo {
        /**
         * The service name of the interface.
         */
        std::string serviceName{};

        /**
         * All the meta information for the component interface.
         */
        std::unordered_map<std::string, std::string> properties{};
    };

    /**
     * Trivial struct containing service dependency info.
     */
    struct ServiceDependencyInfo {
        /**
         * The service name targeted by this service dependency.
         */
        std::string serviceName{};

        /**
         * The additional filter used to track services.
         */
        std::string filter{};

        /**
         * The optional version range used to track services.
         */
        std::string versionRange{};

        /**
         * Whether the service is available (at least 1 dependency found).
         */
        bool isAvailable{};

        /**
         * Whether the service is required (need at least 1 available service to be resolved).
         */
        bool isRequired{};

        /**
         * Nummer tracker services.
         */
        std::size_t nrOfTrackedServices{0};
    };

    /**
     * Trivial struct component info.
     */
    struct ComponentInfo {
        /**
         * The UUID of the component.
         */
        std::string uuid{};

        /**
         * The name of the component.
         */
        std::string name{};

        /**
         * Whether the component is active.
         */
        bool isActive{};

        /**
         * The state of the component.
         */
        std::string state{};

        /**
         * Number of time started.
         */
         std::size_t nrOfTimesStarted{0};

        /**
        * Number of time resumed cycles.
        */
        std::size_t nrOfTimesResumed{0};

        /**
         * The interface provided by the component (when active).
         */
        std::vector<InterfaceInfo> interfacesInfo{};

        /**
         * The service dependencies of the component.
         */
        std::vector<ServiceDependencyInfo> dependenciesInfo{};
    };

    /**
     * Trivial struct containing Dependency Manager info.
     */
    struct DependencyManagerInfo {
        /**
         * The owning bundle id of the dependency manager where information is extracted from.
         */
        long bndId{};

        /**
         * The owning bundle symbolic name of the dependency manager where information is extracted from.
         */
        std::string bndSymbolicName{};

        /**
         * vector of component info for all the (build) components for this dependency manager
         */
        std::vector<ComponentInfo> components{};
    };
}}