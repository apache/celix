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
#include <memory>

#include "celix/dm/Properties.h"

namespace celix { namespace dm {

    class BaseProvidedService {
    public:
        BaseProvidedService(celix_dm_component_t* _cmp, std::string svcName, std::shared_ptr<void> svc, bool _cppService);

        BaseProvidedService(BaseProvidedService&&) = delete;
        BaseProvidedService& operator=(BaseProvidedService&&) = delete;
        BaseProvidedService(const BaseProvidedService&) = delete;
        BaseProvidedService& operator=(const BaseProvidedService&) = delete;

        const std::string& getName() const;
        const std::string& getVersion() const;
        bool isCppService() const;
        void* getService() const;
        const celix::dm::Properties& getProperties() const;

        void runBuild();
        void wait() const;
    protected:
        celix_dm_component_t* cCmp;
        std::string svcName;
        std::shared_ptr<void> svc;
        bool cppService;
        std::string svcVersion{};
        celix::dm::Properties properties{};
        bool provideAddedToCmp{false};
    };

    template<typename T, typename I>
    class ProvidedService : public BaseProvidedService {
    public:
        ProvidedService(celix_dm_component_t* _cmp, std::string svcName, std::shared_ptr<I> svc, bool _cppService);

        /**
         * Set the version of the interface
         */
        ProvidedService<T,I>& setVersion(std::string v);

        /**
         * Set the properties of the interface. Note this will reset the already set / added properties
         */
        ProvidedService<T,I>& setProperties(celix::dm::Properties);

        /**
         * Add/override a single property to the service properties.
         * U should support std::to_string.
         */
        template<typename U>
        ProvidedService<T,I>& addProperty(const std::string& key, const U& value);

        /**
         * Add/override a single property to the service properties.
         */
        ProvidedService<T,I>& addProperty(const std::string& key, const char* value);

        /**
         * Add/override a single property to the service properties.
         */
        ProvidedService<T,I>& addProperty(const std::string& key, const std::string& value);

        /**
         * Build the provided service
         * A provided service added to an active component will only become active if the build is called.
         *
         * When build and the component is active, the service will be registered when this call is done.
         *
         * Should not be called on the Celix event thread.
         */
        ProvidedService<T,I>& build();

        /**
         * Same a build, but will not wait till the underlining service is registered
         * Can be called on the Celix event thread.
         */
        ProvidedService<T,I>& buildAsync();
    };

}}

#include "celix/dm/ProvidedService_Impl.h"
