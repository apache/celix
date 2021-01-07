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

#include <celix/dm/DependencyManager.h>

namespace celix::async_rsa {
    /// Service factory interface.
    struct IImportedServiceFactory {
        virtual ~IImportedServiceFactory() = default;

        virtual celix::dm::BaseComponent& create(std::shared_ptr<celix::dm::DependencyManager> &dm, celix::dm::Properties&& properties) = 0;
    };

    /// Default templated imported service factory. If more than a simple creation is needed, please create your own factory derived from IImportedServiceFactory
    /// \tparam Interface
    /// \tparam Implementation
    template <typename Interface, typename Implementation>
    struct DefaultImportedServiceFactory final : public IImportedServiceFactory {
        ~DefaultImportedServiceFactory() final = default;

        celix::dm::BaseComponent& create(std::shared_ptr<celix::dm::DependencyManager> &dm, celix::dm::Properties&& properties) final {
            return &dm->template createComponent<Implementation>()
                .template addInterface<Interface>(Interface::VERSION, std::forward<celix::dm::Properties>(properties))
                .build();
        }
    };
}