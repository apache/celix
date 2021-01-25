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

#include <IEndpoint.h>
#include <celix_api.h>

namespace celix::async_rsa {

    struct IDiscovery {
        virtual ~IDiscovery() = default;

        /// Adds a newly exported endpoint
        /// \param endpoint
        virtual void addExportedEndpoint(celix::async_rsa::IEndpoint *endpoint) = 0;
    };

    struct StaticDiscovery final : public IDiscovery {
        explicit StaticDiscovery(std::shared_ptr<celix::dm::DependencyManager> &mng) noexcept;
        ~StaticDiscovery() final = default;

        /// StaticDiscovery only supports preconfigured endpoints, this function is a NOP.
        /// \param endpoint
        void addExportedEndpoint(celix::async_rsa::IEndpoint *endpoint) final;

    private:
        std::vector<IEndpoint*> _endpoints;
        std::shared_ptr<celix::dm::DependencyManager> _mng;

        void readImportedEndpointsFromFile(std::string_view);
    };
}