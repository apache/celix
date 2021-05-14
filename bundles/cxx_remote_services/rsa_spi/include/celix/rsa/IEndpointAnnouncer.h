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

#include "celix/rsa/EndpointDescription.h"
#include "celix/dm/Properties.h"

namespace celix::rsa {

/**
 * @brief Interface used to announce or revoke endpoint to the remote service discovery network/system.
 */
class IEndpointAnnouncer {
public:
    /**
     * Defaulted virtual destructor.
     */
    virtual ~IEndpointAnnouncer() = default;

    /**
     * Task the endpoint announcer to make the given endpoint visible for discovery by other managers/ frameworks.
     * @param endpoint The endpoint pointer in question, with valid properties within.
     */
    virtual void announceEndpoint(std::unique_ptr<EndpointDescription> endpoint) = 0;

    /**
     * Task the endpoint announcer to remove the discoverability of a given endpoint.
     * @param endpoint The endpoint pointer in question, with valid properties within.
     */
    virtual void revokeEndpoint(std::unique_ptr<EndpointDescription> endpoint) = 0;
};

} // end namespace celix::rsa.
