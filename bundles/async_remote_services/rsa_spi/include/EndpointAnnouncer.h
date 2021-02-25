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

#include <Endpoint.h>

#include <memory>

#include <celix/dm/Properties.h>

namespace celix::rsa {

/**
 * Base class defining functions for all compatible endpoint announcer classes.
 */
class EndpointAnnouncer {
public:

    /**
     * Defaulted constructor.
     */
    EndpointAnnouncer() = default;

    /**
     * Defaulted virtual destructor.
     */
    virtual ~EndpointAnnouncer() = default;

    /**
     * Task the endpoint announcer to make the given endpoint visible for discovery by other managers/ frameworks.
     * @param endpoint The endpoint pointer in question, with valid properties within.
     */
    virtual void announceEndpoint([[maybe_unused]] std::unique_ptr<Endpoint> endpoint) {
        // default implementation ignores this function.
    }

    /**
     * Task the endpoint announcer to remove the discoverability of a given endpoint.
     * @param endpoint The endpoint pointer in question, with valid properties within.
     */
    virtual void revokeEndpoint([[maybe_unused]] std::unique_ptr<Endpoint> endpoint) {
        // default implementation ignores this function.
    }
};

} // end namespace celix::rsa.
