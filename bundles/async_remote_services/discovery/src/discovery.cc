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

#include <discovery.h>
#include <celix_api.h>

struct StaticEndpoint final : celix::async_rsa::IEndpoint {
    explicit StaticEndpoint() noexcept = default;
    ~StaticEndpoint() final = default;
};

celix::async_rsa::StaticDiscovery::StaticDiscovery(std::shared_ptr<celix::dm::DependencyManager> &mng) noexcept : _endpoints(), _mng(mng) {
    readImportedEndpointsFromFile("test");
}

void celix::async_rsa::StaticDiscovery::readImportedEndpointsFromFile(std::string_view) {
    _endpoints.emplace_back(&_mng->createComponent<StaticEndpoint>().addInterface<IEndpoint>("1.0.0", celix::dm::Properties{
            {"service.imported", "*"},
            {"service.exported.interfaces", "IHardcodedService"},
            {"endpoint.id", "1"},
    }).build().getInstance());
}

void celix::async_rsa::StaticDiscovery::addExportedEndpoint([[maybe_unused]] celix::async_rsa::IEndpoint *endpoint, [[maybe_unused]] Properties&& properties) {
    // NOP
}

void celix::async_rsa::StaticDiscovery::removeExportedEndpoint([[maybe_unused]] celix::async_rsa::IEndpoint *endpoint, [[maybe_unused]] Properties&& properties) {
    // NOP
}

struct DiscoveryActivator {
    explicit DiscoveryActivator([[maybe_unused]] std::shared_ptr<celix::dm::DependencyManager> mng) : _cmp(mng->createComponent(std::make_unique<celix::async_rsa::StaticDiscovery>(mng)).addInterface<celix::async_rsa::IDiscovery>().build()) {

    }

    DiscoveryActivator(const DiscoveryActivator &) = delete;
    DiscoveryActivator &operator=(const DiscoveryActivator &) = delete;

private:
    celix::dm::Component<celix::async_rsa::StaticDiscovery>& _cmp;
};

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(DiscoveryActivator)