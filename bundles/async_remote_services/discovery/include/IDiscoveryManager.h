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

#include <memory>
#include <IEndpointEventListener.h>

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
     * Add a endpoint event listener that will be updated by this discovery-manager whenever a new endpoint event is happening.
     * @param endpointEventListener The new endpoint event listener as a shared_ptr ref.
     */
    virtual void addEndpointEventListener(const std::shared_ptr<IEndpointEventListener>& endpointEventListener) = 0;

    /**
     * Task the discovery-manager to find endpoints from remote frameworks or local files.
     */
    virtual void discoverEndpoints() = 0;
};

} // end namespace celix::async_rsa::discovery.
