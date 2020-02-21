/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */


//#include <fmt/ostream.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <iostream>
#include <uuid/uuid.h>

#include "celix/ComponentManager.h"


static auto logger = spdlog::stdout_color_mt("celix::ComponentManager");


static std::string genUUID() {
    uuid_t tmpUUID;
    char uuidStr[37];

    uuid_generate(tmpUUID);
    uuid_unparse(tmpUUID, uuidStr);

    return std::string{uuidStr};
}

celix::GenericServiceDependency::GenericServiceDependency(
        std::shared_ptr<celix::ServiceRegistry> _reg,
        std::string _svcName,
        std::function<void()> _stateChangedCallback,
        std::function<void()> _suspendCallback,
        std::function<void()> _resumeCallback,
        bool _isValid) :
            reg{std::move(_reg)},
            svcName{std::move(_svcName)},
            stateChangedCallback{std::move(_stateChangedCallback)},
            suspenseCallback{std::move(_suspendCallback)},
            resumeCallback{std::move(_resumeCallback)},
            uuid{genUUID()},
            valid{_isValid} {}


bool celix::GenericServiceDependency::isResolved() const {
    std::lock_guard<std::mutex> lck{mutex};
    if (!tracker.empty()) {
        return tracker[0].trackCount() >= 1;
    } else {
        return false;
    }
}

celix::Cardinality celix::GenericServiceDependency::getCardinality() const {
    std::lock_guard<std::mutex> lck{mutex};
    return cardinality;
}

bool celix::GenericServiceDependency::isRequired() const {
    std::lock_guard<std::mutex> lck{mutex};
    return required;
}

const std::string &celix::GenericServiceDependency::getFilter() const {
    std::lock_guard<std::mutex> lck{mutex};
    return filter;
}

const std::string& celix::GenericServiceDependency::getUUD() const {
    return uuid;
}

bool celix::GenericServiceDependency::isEnabled() const {
    std::lock_guard<std::mutex> lck{mutex};
    return !tracker.empty();
}

celix::UpdateServiceStrategy celix::GenericServiceDependency::getStrategy() const {
    std::lock_guard<std::mutex> lck{mutex};
    return strategy;
}

const std::string &celix::GenericServiceDependency::getSvcName() const {
    return svcName;
}

bool celix::GenericServiceDependency::isValid() const {
    return valid;
}

void celix::GenericServiceDependency::preServiceUpdate() {
    if (getStrategy() == UpdateServiceStrategy::Suspense) {
        suspenseCallback(); //note resume will be done in the updateServices (last called)
    }
}

void celix::GenericServiceDependency::postServiceUpdate() {
    if (getStrategy() == UpdateServiceStrategy::Suspense) {
        resumeCallback(); //note suspend call is done in the setService (first called)
    }
    this->stateChangedCallback();
}

celix::ComponentManagerState celix::GenericComponentManager:: getState() const {
    std::lock_guard<std::mutex> lck{stateMutex};
    return state;
}

bool celix::GenericComponentManager::isEnabled() const  {
    std::lock_guard<std::mutex> lck{stateMutex};
    return enabled;
}

bool celix::GenericComponentManager::isResolved() const {
    if (!isEnabled()) {
        return false; //not enabled is not resolved!
    }
    std::lock_guard<std::mutex> lck{stateMutex};
    for (auto &pair : serviceDependencies) {
        if (pair.second->isEnabled() && !pair.second->isResolved()) {
            return false;
        }
    }
    return true;
}

void celix::GenericComponentManager::setEnabled(bool e) {
    std::vector<std::shared_ptr<GenericServiceDependency>> deps{};
    std::vector<std::shared_ptr<GenericProvidedService>> provides{};
    {
        std::lock_guard<std::mutex> lck{serviceDependenciesMutex};
        for (auto &pair : serviceDependencies) {
            deps.push_back(pair.second);
        }
    }
    {
        std::lock_guard<std::mutex> lck{providedServicesMutex};
        for (auto &pair : providedServices) {
            provides.push_back(pair.second);
        }
    }
    //NOTE updating outside of the lock
    for (auto& dep : deps) {
        dep->setEnabled(e);
    }
    for (auto& provide : provides) {
        provide->setEnabled(e);
    }


    {
        std::lock_guard<std::mutex> lck{stateMutex};
        enabled = e;
    }

    updateState();
}

void celix::GenericComponentManager::updateState() {
    bool allRequiredDependenciesResolved = true; /*all required and enabled dependencies resolved? */
    {
        std::lock_guard<std::mutex> lck{serviceDependenciesMutex};
        for (auto &pair : serviceDependencies) {
            auto depEnabled = pair.second->isEnabled();
            auto required = pair.second->isRequired();
            auto resolved = pair.second->isResolved();
            if (depEnabled && required && !resolved) {
                allRequiredDependenciesResolved = false;
                break;
            }
        }
    }

    {
        std::lock_guard<std::mutex> lck{stateMutex};
        ComponentManagerState currentState = state;
        ComponentManagerState targetState = ComponentManagerState::Disabled;

        if (enabled && allRequiredDependenciesResolved) {
            targetState = ComponentManagerState::ComponentStarted;
        } else if (enabled) /*not all required dep resolved*/ {
            targetState = initialized ? ComponentManagerState::ComponentInitialized : ComponentManagerState::ComponentUninitialized;
        }

        if (currentState != targetState) {
            transitionQueue.emplace(currentState, targetState);
        }
    }
    transition();
}

void celix::GenericComponentManager::transition() {
    ComponentManagerState currentState;
    ComponentManagerState targetState;

    {
        std::lock_guard<std::mutex> lck{stateMutex};
        if (transitionQueue.empty()) {
            return;
        }
        auto pair = transitionQueue.front();
        transitionQueue.pop();
        currentState = pair.first;
        targetState = pair.second;
    }

//    logger->info("Transition {} ({})from {} to {}", name, uuid, currentState, targetState);
    logger->info("Transition {} ({}) TODO", name, uuid);

    if (currentState == ComponentManagerState::Disabled) {
        switch (targetState) {
            case celix::ComponentManagerState::Disabled:
                //nop
                break;
            default:
                setState(ComponentManagerState::ComponentUninitialized);
                break;
        }
    } else if (currentState == ComponentManagerState::ComponentUninitialized) {
        switch (targetState) {
            case celix::ComponentManagerState::Disabled:
                setState(ComponentManagerState::Disabled);
                break;
            case celix::ComponentManagerState::ComponentUninitialized:
                //nop
                break;
            default: //targetState initialized of started
                initCmp();
                setInitialized(true);
                setState(ComponentManagerState::ComponentInitialized);
                break;
        }
    } else if (currentState == ComponentManagerState::ComponentInitialized) {
        switch (targetState) {
            case celix::ComponentManagerState::Disabled:
                deinitCmp();
                setInitialized(false);
                setState(ComponentManagerState::ComponentUninitialized);
                break;
            case celix::ComponentManagerState::ComponentUninitialized:
                deinitCmp();
                setInitialized(false);
                setState(ComponentManagerState::ComponentUninitialized);
                break;
            case celix::ComponentManagerState::ComponentInitialized:
                //nop
                break;
            case celix::ComponentManagerState::ComponentStarted:
                startCmp();
                setState(ComponentManagerState ::ComponentStarted);
                updateServiceRegistrations();
                break;
        }
    } else if (currentState == ComponentManagerState::ComponentStarted) {
        switch (targetState) {
            case celix::ComponentManagerState::ComponentStarted:
                //nop
                break;
            default:
                stopCmp();
                setState(ComponentManagerState::ComponentInitialized);
                updateServiceRegistrations(); //TODO before stop?
                break;
        }
    }

    updateState();
}

void celix::GenericComponentManager::setState(ComponentManagerState s) {
    std::lock_guard<std::mutex> lck{stateMutex};
    state = s;
}


std::string celix::GenericComponentManager::getUUD() const {
    return uuid;
}

celix::GenericComponentManager::GenericComponentManager(
        std::shared_ptr<celix::IResourceBundle> _owner,
        std::shared_ptr<celix::ServiceRegistry> _reg,
        const std::string &_name) : owner{std::move(_owner)}, reg{std::move(_reg)}, name{_name}, uuid{genUUID()} {
}

void celix::GenericComponentManager::removeServiceDependency(const std::string& serviceDependencyUUID) {
    std::shared_ptr<GenericServiceDependency> removed{};
    {
        std::lock_guard<std::mutex> lck{serviceDependenciesMutex};
        auto it = serviceDependencies.find(serviceDependencyUUID);
        if (it != serviceDependencies.end()) {
            removed = it->second;
            serviceDependencies.erase(it);
        } else {
            logger->info("Cannot find service dependency with uuid {} in component manager {} with uuid {}", serviceDependencyUUID, name, uuid);
        }
    }
    if (removed) {
        removed->setEnabled(false);
    }
    //NOTE removed out of scope -> RAII will cleanup
}

void celix::GenericComponentManager::removeProvideService(const std::string& provideServiceUUID) {
    std::shared_ptr<GenericProvidedService> removed{};
    {
        std::lock_guard<std::mutex> lck{providedServicesMutex};
        auto it = providedServices.find(provideServiceUUID);
        if (it != providedServices.end()) {
            removed = it->second;
            providedServices.erase(it);
        } else {
            logger->info("Cannot find provided service with uuid {} in component manager {} with uuid {}", provideServiceUUID, name, uuid);
        }
    }
    if (removed) {
        removed->setEnabled(false);
    }
    //NOTE removed out of scope -> RAII will cleanup
}

std::string celix::GenericComponentManager::getName() const {
    return name;
}

void celix::GenericComponentManager::setInitialized(bool i) {
    std::lock_guard<std::mutex> lck{stateMutex};
    initialized = i;
}

std::shared_ptr<celix::GenericServiceDependency>
celix::GenericComponentManager::findGenericServiceDependency(const std::string &svcName,
                                                             const std::string &svcDepUUID) {
    std::lock_guard<std::mutex> lck{serviceDependenciesMutex};
    auto it = serviceDependencies.find(svcDepUUID);
    if (it != serviceDependencies.end()) {
        if (it->second->getSvcName() == svcName) {
            return it->second;
        } else {
            logger->warn("Requested svc name has svc name {} instead of the requested {}", it->second->getSvcName(), svcName);
            return nullptr;
        }
    } else {
        logger->warn("Cmp Manager ({}) does not have a service dependency with uuid {}; returning an invalid GenericServiceDependency", uuid, svcDepUUID);
        return nullptr;
    }
}

std::size_t celix::GenericComponentManager::getSuspendedCount() const {
    std::lock_guard<std::mutex> lck{stateMutex};
    return suspendedCount;
}

void celix::GenericComponentManager::suspense() {
    bool changedSuspended = false;
    {
        std::lock_guard<std::mutex> lck{stateMutex};
        if (!suspended) {
            suspended = true;
            changedSuspended = true;
        }
    }
    if (changedSuspended) {
        transition();
        std::lock_guard<std::mutex> lck{stateMutex};
        suspendedCount += 1;
        logger->info("Suspended {} ({})", name, uuid);
    }
}

void celix::GenericComponentManager::resume() {
    bool changedSuspended = false;
    {
        std::lock_guard<std::mutex> lck{stateMutex};
        if (suspended) {
            suspended = false;
            changedSuspended = true;
        }
    }
    if (changedSuspended) {
        transition();
        logger->info("Resumed {} ({})", name, uuid);
    }
}

void celix::GenericComponentManager::updateServiceRegistrations() {
    std::vector<std::shared_ptr<GenericProvidedService>> toRegisterServices{};
    std::vector<std::shared_ptr<GenericProvidedService>> toUnregisterServices{};
    if (getState() == ComponentManagerState::ComponentStarted) {
        std::lock_guard<std::mutex> lck{providedServicesMutex};
        for (auto &pair : providedServices) {
            auto provideEnabled = pair.second->isEnabled();
            auto registered = pair.second->isServiceRegistered();
            if (enabled && !registered) {
                toRegisterServices.push_back(pair.second);
            } else if (registered && !provideEnabled) {
                toUnregisterServices.push_back(pair.second);
            }
        }
    } else {
        std::lock_guard<std::mutex> lck{providedServicesMutex};
        for (auto &pair : providedServices) {
            auto registered = pair.second->isServiceRegistered();
            if (registered) {
                toUnregisterServices.push_back(pair.second);
            }
        }
    }

    //note (un)registering service outside of mutex
    for (auto& provide : toRegisterServices) {
        provide->registerService();
    }
    for (auto& provide : toUnregisterServices) {
        provide->unregisterService();
    }
}

std::shared_ptr<celix::GenericProvidedService>
celix::GenericComponentManager::findGenericProvidedService(const std::string &svcName,
                                                           const std::string &providedServiceUUID) {
    std::lock_guard<std::mutex> lck{providedServicesMutex};
    auto it = providedServices.find(providedServiceUUID);
    if (it != providedServices.end()) {
        if (it->second->getServiceName() == svcName) {
            return it->second;
        } else {
            logger->warn("Requested svc name has svc name {} instead of the requested {}", it->second->getServiceName(), svcName);
            return nullptr;
        }
    } else {
        logger->warn("Cmp Manager ({}) does not have a provided service with uuid {}; returning an invalid GenericProvidedService", uuid, providedServiceUUID);
        return nullptr;
    }
}

std::size_t celix::GenericComponentManager::nrOfServiceDependencies() {
    std::lock_guard<std::mutex> lck{serviceDependenciesMutex};
    return serviceDependencies.size();
}

std::size_t celix::GenericComponentManager::nrOfProvidedServices() {
    std::lock_guard<std::mutex> lck{providedServicesMutex};
    return providedServices.size();
}

std::ostream& operator<< (std::ostream& out, const celix::GenericComponentManager& mng) {
    out << "ComponentManager[name=" << mng.getName() << ", uuid=" << mng.getUUD() << "]";
    return out;
}


std::ostream& operator<< (std::ostream& out, celix::ComponentManagerState state)
{
    switch (state) {
        case celix::ComponentManagerState::Disabled:
            out << "Disabled";
            break;
        case celix::ComponentManagerState::ComponentUninitialized:
            out << "ComponentUninitialized";
            break;
        case celix::ComponentManagerState::ComponentInitialized:
            out << "ComponentInitialized";
            break;
        case celix::ComponentManagerState::ComponentStarted:
            out << "ComponentStarted";
            break;
    }
    return out;
}

bool celix::GenericProvidedService::isEnabled() const {
    std::lock_guard<std::mutex> lck{mutex};
    return enabled;
}

const std::string &celix::GenericProvidedService::getUUID() const {
    return uuid;
}

const std::string &celix::GenericProvidedService::getServiceName() const {
    return svcName;
}

bool celix::GenericProvidedService::isValid() const {
    return valid;
}

celix::GenericProvidedService::GenericProvidedService(
        std::string _cmpUUID,
        std::shared_ptr<celix::ServiceRegistry> _reg,
        std::string _svcName,
        std::function<void()> _updateServiceRegistrationsCallback,
        bool _valid) :
    cmpUUID{std::move(_cmpUUID)},
    reg{std::move(_reg)},
    uuid{genUUID()},
    svcName{std::move(_svcName)},
    updateServiceRegistrationsCallback{std::move(_updateServiceRegistrationsCallback)},
    valid{_valid} {

}

void celix::GenericProvidedService::setEnabled(bool e) {
    {
        std::lock_guard<std::mutex> lck{mutex};
        enabled = e;
    }
    updateServiceRegistrationsCallback();
}

void celix::GenericProvidedService::unregisterService() {
    std::vector<ServiceRegistration> unregister{};
    {
        std::lock_guard<std::mutex> lck{mutex};
        std::swap(registration, unregister);
    }
    //NOTE RAII will ensure unregister
}

bool celix::GenericProvidedService::isServiceRegistered() {
    std::lock_guard<std::mutex> lck{mutex};
    return !registration.empty();
}
