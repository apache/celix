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

#include "RemoteServiceAdmin.h"
#include "celix/BundleContext.h"
#include "celix/rsa/RemoteConstants.h"

#define L_TRACE(...) \
        logHelper.trace(__VA_ARGS__);
#define L_DEBUG(...) \
        logHelper.debug(__VA_ARGS__);
#define L_INFO(...) \
        _logHelper.info(__VA_ARGS__);
#define L_WARN(...) \
        logHelper.warning(__VA_ARGS__);
#define L_ERROR(...) \
        logHelper.error(__VA_ARGS__);

celix::rsa::RemoteServiceAdmin::RemoteServiceAdmin(celix::LogHelper logHelper) : logHelper{std::move(logHelper)} {}

void celix::rsa::RemoteServiceAdmin::addEndpoint(const std::shared_ptr<celix::rsa::EndpointDescription>& endpoint) {
    assert(endpoint);

    auto interface = endpoint->getInterface();
    if (interface.empty()) {
        L_WARN("Adding endpoint but missing exported interfaces");
        return;
    }

    auto endpointId = endpoint->getId();
    if (endpointId.empty()) {
        L_WARN("Adding endpoint but missing service id");
        return;
    }

    std::lock_guard l(mutex);
    toBeImportedServices.emplace_back(endpoint);
    createImportServices();
}

void celix::rsa::RemoteServiceAdmin::removeEndpoint(const std::shared_ptr<celix::rsa::EndpointDescription>& endpoint) {
    assert(endpoint);

    auto id = endpoint->getId();
    if (id.empty()) {
        L_WARN("Cannot remove endpoint without a valid id");
        return;
    }

    std::shared_ptr<celix::rsa::IImportRegistration> tmpStore{}; //to ensure destruction outside of lock
    {
        std::lock_guard l(mutex);

        toBeImportedServices.erase(std::remove_if(toBeImportedServices.begin(), toBeImportedServices.end(), [&id](auto const &endpoint){
            return id == endpoint->getId();
        }), toBeImportedServices.end());

        auto it = importedServices.find(id);
        if (it != importedServices.end()) {
            tmpStore = std::move(it->second);
            importedServices.erase(it);
        }
    }
}

void celix::rsa::RemoteServiceAdmin::addImportedServiceFactory(
        const std::shared_ptr<celix::rsa::IImportServiceFactory> &factory,
        const std::shared_ptr<const celix::Properties> &properties) {
    auto targetServiceName = properties->get(celix::rsa::IImportServiceFactory::REMOTE_SERVICE_TYPE);
    if (targetServiceName.empty()) {
        L_WARN("Adding service factory but missing %s property", celix::rsa::IImportServiceFactory::REMOTE_SERVICE_TYPE);
        return;
    }


    std::lock_guard l(mutex);
    auto existingFactory = importServiceFactories.find(targetServiceName);
    if (existingFactory != end(importServiceFactories)) {
        L_WARN("Adding imported factory but factory already exists");
        return;
    }

    importServiceFactories.emplace(targetServiceName, factory);

    createImportServices();
}

void celix::rsa::RemoteServiceAdmin::removeImportedServiceFactory(
        const std::shared_ptr<celix::rsa::IImportServiceFactory> &/*factory*/,
        const std::shared_ptr<const celix::Properties> &properties) {
    auto targetServiceName = properties->get(celix::rsa::IImportServiceFactory::REMOTE_SERVICE_TYPE);
    if (targetServiceName.empty()) {
        L_WARN("Removing service factory but missing %s property", celix::rsa::IImportServiceFactory::REMOTE_SERVICE_TYPE);
        return;
    }

    std::lock_guard l(mutex);
    importServiceFactories.erase(targetServiceName);

    //TODO remove imported services from this factory ??needed
}

void celix::rsa::RemoteServiceAdmin::addExportedServiceFactory(
        const std::shared_ptr<celix::rsa::IExportServiceFactory> &factory,
        const std::shared_ptr<const celix::Properties> &properties) {
    auto targetServiceName = properties->get(celix::rsa::IExportServiceFactory::REMOTE_SERVICE_TYPE);
    if (targetServiceName.empty()) {
        L_WARN("Adding service factory but missing %s property", celix::rsa::IExportServiceFactory::REMOTE_SERVICE_TYPE);
        return;
    }

    std::lock_guard<std::mutex> lock{mutex};
    exportServiceFactories.emplace(targetServiceName, factory);
    createExportServices();
}

void celix::rsa::RemoteServiceAdmin::removeExportedServiceFactory(
        const std::shared_ptr<celix::rsa::IExportServiceFactory> &/*factory*/,
        const std::shared_ptr<const celix::Properties> &properties) {
    auto targetServiceName = properties->get(celix::rsa::IExportServiceFactory::REMOTE_SERVICE_TYPE);
    if (targetServiceName.empty()) {
        L_WARN("Removing service factory but missing %s property", celix::rsa::IExportServiceFactory::REMOTE_SERVICE_TYPE);
        return;
    }

    std::lock_guard l(mutex);
    exportServiceFactories.erase(targetServiceName);

    //TODO remove exported services from this factory ??needed
}

void celix::rsa::RemoteServiceAdmin::addService(const std::shared_ptr<void>& /*svc*/, const std::shared_ptr<const celix::Properties>& props) {
    auto serviceName = props->get(celix::SERVICE_NAME, "");
    if (serviceName.empty()) {
        L_WARN("Adding service to be exported but missing objectclass");
        return;
    }

    std::lock_guard<std::mutex> lock{mutex};
    toBeExportedServices.emplace_back(props);
    createExportServices();
}

void celix::rsa::RemoteServiceAdmin::removeService(const std::shared_ptr<void>& /*svc*/, const std::shared_ptr<const celix::Properties>& props) {
    long svcId = props->getAsLong(celix::SERVICE_ID, -1);
    if (svcId < 0) {
        L_WARN("Removing service to be exported but missing service.id");
        return;
    }

    std::lock_guard l(mutex);

    auto instanceIt = exportedServices.find(svcId);
    if (instanceIt != end(exportedServices)) {
        exportedServices.erase(instanceIt);
    }

    //remove to be exported endpoint (if present)
    for (auto it = toBeExportedServices.begin(); it != toBeExportedServices.end(); ++it) {
        if ((*it)->getAsLong(celix::SERVICE_ID, -1) == svcId) {
            toBeExportedServices.erase(it);
            break;
        }
    }
}

bool celix::rsa::RemoteServiceAdmin::isEndpointMatch(const celix::rsa::EndpointDescription& endpoint, const celix::rsa::IImportServiceFactory& factory) const {
    if (factory.getSupportedConfigs().empty()) {
        L_WARN("Matching endpoint with a import service factory with no supported configs, this will always fail");
        return false;
    }
    if (endpoint.getConfigurationTypes().empty()) {
        L_WARN("Matching endpoint with empty configuration types (%s), this will always fail", celix::rsa::SERVICE_IMPORTED_CONFIGS);
        return false;
    }
    size_t matchCount = 0;
    for (const auto& config : endpoint.getConfigurationTypes()) {
        for (const auto& factoryConfig : factory.getSupportedConfigs()) {
            if (config == factoryConfig) {
                ++matchCount;
                break;
            }
        }
    }
    return matchCount == factory.getSupportedConfigs().size(); //note must match all requested configurations
}

bool celix::rsa::RemoteServiceAdmin::isExportServiceMatch(const celix::Properties& svcProperties, const celix::rsa::IExportServiceFactory& factory) const {
    auto providedConfigs = celix::split(svcProperties.get(celix::rsa::SERVICE_IMPORTED_CONFIGS));
    auto providedIntents = celix::split(svcProperties.get(celix::rsa::SERVICE_EXPORTED_INTENTS, "osgi.basic"));
    if (providedConfigs.empty() && providedIntents.empty()) {
        //note cannot match export service with no config or intent
        return false;
    }
    if (factory.getSupportedIntents().empty() && factory.getSupportedConfigs().empty()) {
        L_WARN("Matching service marked for export with a export service factory with no supported intents or configs, this will always fail");
        return false;
    }
    if (!providedConfigs.empty()) {
        size_t matchCount = 0;
        for (const auto& config : providedConfigs) {
            for (const auto& factoryConfig : factory.getSupportedConfigs()) {
                if (config == factoryConfig) {
                    ++matchCount;
                    break;
                }
            }
        }
        return matchCount == factory.getSupportedConfigs().size(); //note must match all requested configurations
    } else /*match on intent*/ {
        for (const auto& intent: providedIntents) {
            for (const auto& factoryIntent: factory.getSupportedIntents()) {
                if (intent == factoryIntent) {
                    return true;
                }
            }
        }
        return false;
    }
}

void celix::rsa::RemoteServiceAdmin::createImportServices() {
    //precondition mutex taken
    auto it = toBeImportedServices.begin();
    while (it != toBeImportedServices.end()) {
        auto interface = (*it)->getInterface();
        bool match = false;
        for (auto factoryIt = importServiceFactories.lower_bound(interface); factoryIt != importServiceFactories.end() && factoryIt->first == interface; ++factoryIt) {
            if (isEndpointMatch(**it, *factoryIt->second)) {
                match = true;
                auto endpointId = (*it)->getId();
                L_DEBUG("Adding endpoint %s, created export service for %s", endpointId.c_str(), (*it)->getInterface().c_str());
                importedServices.emplace(endpointId, factoryIt->second->importService(**it));
                break;
            } else {
                L_TRACE("Config mismatch.");
            }
        }
        if (match) {
            it = toBeImportedServices.erase(it);
        } else {
            L_DEBUG("Adding endpoint to be imported but no matching factory available yet, delaying import");
            ++it;
        }
    }
}

void celix::rsa::RemoteServiceAdmin::createExportServices() {
    //precondition mutex taken
    auto it = toBeExportedServices.begin();
    while (it != toBeExportedServices.end()) {
        const auto& svcProperties = **it;
        auto serviceName = svcProperties.get(celix::SERVICE_NAME, "");
        auto svcId = svcProperties.getAsLong(celix::SERVICE_ID, -1);
        bool match = false;
        for (auto factoryIt = exportServiceFactories.lower_bound(serviceName); factoryIt != exportServiceFactories.end() && factoryIt->first == serviceName; ++factoryIt) {
            if (isExportServiceMatch(svcProperties, *factoryIt->second)) {
                match = true;
                exportedServices.emplace(svcId, factoryIt->second->exportService(svcProperties));
                break;
            } else {
                L_TRACE("Intent mismatch");
            }
        }
        if (match) {
            it = toBeExportedServices.erase(it);
        } else {
            L_DEBUG("Adding endpoint to be imported but no matching factory available yet, delaying import");
            ++it;
        }
    }
}