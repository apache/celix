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

#include <ConfiguredDiscoveryManager.h>
#include <celix_api.h>

namespace {

/**
 * Activator class for the ConfiguredDiscoveryManager object.
 */
class ConfiguredDiscoveryManagerActivator {
public:

    /**
     * Deleted default constructor, dependencyManager parameter is required.
     */
    ConfiguredDiscoveryManagerActivator() = delete;

    /**
     *  Constructor for the ConfiguredDiscoveryManager.
     * @param dependencyManager shared_ptr to the context/container dependency manager.
     */
    explicit ConfiguredDiscoveryManagerActivator(const std::shared_ptr<celix::dm::DependencyManager>& dependencyManager);

    /**
     * Defaulted destructor.
     */
    ~ConfiguredDiscoveryManagerActivator() = default;

    /**
     * Defaulted copy-constructor.
     */
    ConfiguredDiscoveryManagerActivator(const ConfiguredDiscoveryManagerActivator&) = default;

    /**
     * Defaulted move-constructor.
     */
    ConfiguredDiscoveryManagerActivator(ConfiguredDiscoveryManagerActivator&&) = default;

private:

    celix::dm::Component<celix::async_rsa::ConfiguredDiscoveryManager>& _component;
};

} // end anonymous namespace.
