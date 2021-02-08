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
#include <celix/dm/Properties.h>

namespace celix::async_rsa::discovery {

/**
 * Interface defining functions for all compatible discovery-manager classes.
 */
class IDiscoveryManager {
public:

    /**
     * Defaulted constructor.
     */
    IDiscoveryManager() = default;

    /**
     * Defaulted virtual destructor.
     */
    virtual ~IDiscoveryManager() = default;

    /**
     * Task the discovery-manager to find endpoints from remote frameworks or local files.
     */
    virtual void discoverEndpoints() = 0;

    /**
     * Task the discovery-manager to make the given endpoint visible for discovery by other managers/ frameworks.
     * @param endpoint The endpoint pointer in question.
     * @param properties The celix properties concerning the endpoint.
     */
    virtual void addExportedEndpoint(IEndpoint *endpoint, celix::dm::Properties&& properties) = 0;

    /**
     * Task the discovery-manager to remove the discoverability of a given endpoint.
     * @param endpoint The endpoint pointer in question.
     * @param properties The celix properties concerning the endpoint, used for lookup.
     */
    virtual void removeExportedEndpoint(IEndpoint *endpoint, celix::dm::Properties&& properties) = 0;
};

} // end namespace celix::async_rsa::discovery.
