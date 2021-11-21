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

#include <topology_manager.h>
#include <celix_api.h>
#include <ConfiguredEndpoint.h>

#define L_DEBUG(...) \
    celix_logHelper_log(_logger, CELIX_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define L_INFO(...) \
    celix_logHelper_log(_logger, CELIX_LOG_LEVEL_INFO, __VA_ARGS__)
#define L_WARN(...) \
    celix_logHelper_log(_logger, CELIX_LOG_LEVEL_WARNING, __VA_ARGS__)
#define L_ERROR(...) \
    celix_logHelper_log(_logger, CELIX_LOG_LEVEL_ERROR, __VA_ARGS__)

celix::async_rsa::AsyncTopologyManager::AsyncTopologyManager(celix_log_helper_t *logger) noexcept : _logger(logger) {

}

celix::async_rsa::AsyncTopologyManager::~AsyncTopologyManager() {
    celix_logHelper_destroy(_logger);
}

void celix::async_rsa::AsyncTopologyManager::addExportedService(celix::async_rsa::IExportedService *exportedService, Properties &&properties) {
    auto interface = properties.get(celix::rsa::Endpoint::EXPORTS);

    if(interface.empty()) {
        L_DEBUG("Adding exported service but no exported interfaces");
        return;
    }

    std::unique_lock l(_m);

    auto existingFactory = _exportedServices.find(interface);

    if(existingFactory != end(_exportedServices)) {
        L_WARN("Adding service factory but factory already exists");
        return;
    }

    if(_discovery != nullptr) {
        _discovery->announceEndpoint(std::make_unique<celix::rsa::Endpoint>(std::move(properties)));
    } else {
        L_WARN("Discovery null");
        return;
    }

    _exportedServices.emplace(interface, exportedService);
}

void celix::async_rsa::AsyncTopologyManager::removeExportedService([[maybe_unused]] celix::async_rsa::IExportedService *exportedService, Properties &&properties) {
    auto interface = properties.get(celix::rsa::Endpoint::EXPORTS);

    if(interface.empty()) {
        L_WARN("Removing exported service but missing exported interfaces");
        return;
    }

    std::unique_lock l(_m);

    if(_exportedServices.erase(interface) > 0) {
        if(_discovery != nullptr) {
            _discovery->revokeEndpoint(std::make_unique<celix::rsa::Endpoint>(std::move(properties)));
        } else {
            L_WARN("Discovery null");
            return;
        }
    }
}

void celix::async_rsa::AsyncTopologyManager::setDiscovery(celix::rsa::IEndpointAnnouncer *discovery) {
    std::unique_lock l(_m);
    _discovery = discovery;
}

class TopologyManagerActivator {
public:
    explicit TopologyManagerActivator(const std::shared_ptr<celix::dm::DependencyManager> &mng) :
    _cmp(mng->createComponent(std::make_unique<celix::async_rsa::AsyncTopologyManager>(celix_logHelper_create(mng->bundleContext(), "celix_async_rsa_admin")))) {
        _cmp.createServiceDependency<celix::async_rsa::IExportedService>()
                .setRequired(false)
                .setCallbacks(&celix::async_rsa::AsyncTopologyManager::addExportedService, &celix::async_rsa::AsyncTopologyManager::removeExportedService)
                .build();

        _cmp.createServiceDependency<celix::rsa::IEndpointAnnouncer>()
                .setRequired(true)
                .setCallbacks(&celix::async_rsa::AsyncTopologyManager::setDiscovery)
                .build();

        _cmp.build();
    }

    TopologyManagerActivator(const TopologyManagerActivator &) = delete;
    TopologyManagerActivator &operator=(const TopologyManagerActivator &) = delete;
private:
    celix::dm::Component<celix::async_rsa::AsyncTopologyManager>& _cmp;
};

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(TopologyManagerActivator)
