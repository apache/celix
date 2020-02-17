#include <utility>

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

#include <unordered_map>
#include <mutex>
#include <set>
#include <utility>
#include <future>
#include <climits>

#include <glog/logging.h>
#include <cassert>
#include <celix/ServiceRegistry.h>


#include "celix/Constants.h"
#include "celix/ServiceRegistry.h"
#include "celix/Filter.h"

#include "ServiceTrackerImpl.h"


/**********************************************************************************************************************
  Impl classes
 **********************************************************************************************************************/

class celix::ServiceRegistry::Impl {
private:
    const std::shared_ptr<const celix::IResourceBundle> emptyBundle = std::shared_ptr<const celix::IResourceBundle>{new celix::EmptyResourceBundle{}};
    const std::string regName;

    struct {
        mutable std::mutex mutex{};
        long nextSvcId = 1L;

        /* Services entries are always stored for it specific svc name.
         * When using services is it always expected that the svc name is provided, and as such
         * storing per svc name is more logical.
         *
         * Note that this also means that the classic 99% use cases used OSGi filter (objectClass=<svcName>)
         * is not needed anymore -> simpler and faster.
         *
         * map key is svcName and the map value is a set of celix::impl::SvcEntry structs.
         * note: The celix::impl::SvcEntry set is ordered (i.e. not a hashset with O(1) access) to ensure that service ranking and
         * service id can be used for their position in the set (see celix::impl::SvcEntryLess).
         */
        std::unordered_map<std::string, std::set<std::shared_ptr<const celix::impl::SvcEntry>, celix::impl::SvcEntryLess>> registry{};

        //Cache map for faster and easier access based on svcId.
        std::unordered_map<long, std::shared_ptr<const celix::impl::SvcEntry>> cache{};
    } services{};

    struct {
        mutable std::mutex mutex{};
        long nextTrackerId = 1L;

        /* Note same ordering as services registry, expect the ranking order requirement,
         * so that it is easier and faster to update the trackers.
         */
        std::unordered_map<std::string, std::set<std::shared_ptr<celix::impl::SvcTrackerEntry>>> registry{};
        std::unordered_map<long, std::shared_ptr<celix::impl::SvcTrackerEntry>> cache{};
    } trackers{};
public:
    Impl(std::string _regName) : regName{std::move(_regName)} {}

    celix::ServiceRegistration registerAnyService(
            const std::string &svcName,
            std::shared_ptr<void> service,
            std::shared_ptr<celix::IServiceFactory<void>> factoryService,
            celix::Properties props,
            const std::shared_ptr<celix::IResourceBundle> &owner) {
        assert(!svcName.empty()); //TODO create and throw celix invalid argument/svcName exception
        props[celix::SERVICE_NAME] = svcName;

        std::lock_guard<std::mutex> lock{services.mutex};
        long svcId = services.nextSvcId++;
        props[celix::SERVICE_ID] = std::to_string(svcId);

        //Add to registry
        std::shared_ptr<const celix::IResourceBundle> bnd = owner ? owner : emptyBundle;
        props[celix::SERVICE_BUNDLE_ID] = std::to_string(bnd->id());

        assert(! (factoryService && service)); //cannot register both factory and 'normal' service

        if (factoryService) {
            DLOG(INFO) << "Registering service factory '" << svcName << "' from bundle id " << bnd->id() << std::endl;
        } else {
            //note service can be nullptr. TODO is this allowed?
            DLOG(INFO) << "Registering service '" << svcName << "' from bundle id " << bnd->id() << std::endl;
        }

        const auto it = services.registry[svcName].emplace(new celix::impl::SvcEntry{std::move(bnd), svcId, svcName, std::move(service), std::move(factoryService), std::move(props)});
        assert(it.second); //should always lead to a new entry
        const std::shared_ptr<const celix::impl::SvcEntry> &entry = *it.first;

        //Add to svcId cache
        services.cache[entry->svcId] = entry;

        //update trackers
        auto future = std::async([&]{updateTrackers(entry, true);});
        future.wait();
        entry->decrUsage(); //note usage started at 1 during creation


        //create unregister callback
        std::function<void()> unreg = [this, svcId]() -> void {
            this->unregisterService(svcId);
        };
        auto *impl = celix::impl::createServiceRegistrationImpl(entry, std::move(unreg), true);

        return celix::ServiceRegistration{impl};
    }

    void unregisterService(long svcId) {
        if (svcId <= 0) {
            return; //silent ignore
        }

        std::shared_ptr<const celix::impl::SvcEntry> match{nullptr};

        {
            std::lock_guard<std::mutex> lock{services.mutex};
            const auto cacheIter = services.cache.find(svcId);
            if (cacheIter != services.cache.end()) {
                match = cacheIter->second;
                services.cache.erase(cacheIter);
                const auto svcSetIter = services.registry.find(match->svcName);
                svcSetIter->second.erase(match);
                if (svcSetIter->second.empty()) {
                    //last entry in the registry for this service name.
                    services.registry.erase(svcSetIter);
                }
            }
        }

        if (match) {
            auto future = std::async([&]{updateTrackers(match, false);});
            future.wait();
            match->waitTillUnused();
        } else {
            LOG(WARNING) << "Cannot unregister service. Unknown service id: " << svcId << "." << std::endl;
        }
    }

    void removeTracker(long trkId) {
        if (trkId <= 0) {
            return; //silent ignore
        }

        std::shared_ptr<celix::impl::SvcTrackerEntry> match{nullptr};
        {
            std::lock_guard<std::mutex> lock{trackers.mutex};
            const auto it = trackers.cache.find(trkId);
            if (it != trackers.cache.end()) {
                match = it->second;
                trackers.cache.erase(it);
                trackers.registry.at(match->svcName).erase(match);
            }
        }

        if (match) {
            match->waitTillUnused();
            auto future = std::async([&]{match->clear();}); //ensure that all service are removed using the callbacks
            future.wait();
        } else {
            LOG(WARNING) << "Cannot remove tracker. Unknown tracker id: " << trkId << "." << std::endl;
        }
    }

    void updateTrackers(const std::shared_ptr<const celix::impl::SvcEntry> &entry, bool adding) {
        std::vector<std::shared_ptr<celix::impl::SvcTrackerEntry>> matchedTrackers{};
        {
            std::lock_guard<std::mutex> lock{trackers.mutex};
            for (auto &tracker : trackers.registry[entry->svcName]) {
                if (tracker->match(*entry)) {
                    tracker->incrUsage();
                    matchedTrackers.push_back(tracker);
                }
            }
        }

        for (auto &match : matchedTrackers) {
            if (adding) {
                match->addMatch(entry);
            } else {
                match->remMatch(entry);
            }
            match->decrUsage();
        }
    }

    std::vector<std::string> listAllRegisteredServiceNames() const {
        std::vector<std::string> result{};
        std::lock_guard<std::mutex> lck{services.mutex};
        for (const auto& pair : services.registry) {
            result.emplace_back(std::string{pair.first});
        }
        return result;
    }

    celix::ServiceTracker trackAnyServices(const std::string &svcName, celix::ServiceTrackerOptions<void> opts, const std::shared_ptr<celix::IResourceBundle>& requester) {
        //TODO create new tracker event and start new thread to update track trackers
        assert(!svcName.empty()); //TODO support empty name
        auto req = requester ? requester : emptyBundle;
        long trkId = 0;
        {
            std::lock_guard<std::mutex> lck{trackers.mutex};
            trkId = trackers.nextTrackerId++;
        }

        auto trkEntry = std::shared_ptr<celix::impl::SvcTrackerEntry>{
                new celix::impl::SvcTrackerEntry{trkId, svcName, req, std::move(opts)}};
        if (trkEntry->valid()) {

            //find initial services and add new tracker to registry.
            //NOTE two locks to ensure no new services can be added/removed during initial tracker setup
            std::vector<std::shared_ptr<const celix::impl::SvcEntry>> namedServices{};
            {
                std::lock_guard<std::mutex> lck1{services.mutex};
                for (auto &svcEntry : services.registry[trkEntry->svcName]) {
                    if (trkEntry->match(*svcEntry)) {
                        svcEntry->incrUsage();
                        namedServices.push_back(svcEntry);
                    }
                }

                std::lock_guard<std::mutex> lck2{trackers.mutex};
                trackers.registry[trkEntry->svcName].insert(trkEntry);
                trackers.cache[trkEntry->id] = trkEntry;
            }
            auto future = std::async([&] {
                for (auto &svcEntry : namedServices) {
                    trkEntry->addMatch(svcEntry);
                    svcEntry->decrUsage();
                }
            });
            future.wait();
            trkEntry->decrUsage(); //note trkEntry usage started at 1

            auto untrack = [this, trkId]() -> void {
                this->removeTracker(trkId);
            };
            auto *impl = celix::impl::createServiceTrackerImpl(trkEntry, std::move(untrack), true);
            return celix::ServiceTracker{impl};
        } else {
            trkEntry->decrUsage(); //note usage is 1 at creation
            LOG(ERROR) << "Cannot create tracker. Invalid filter?" << std::endl;
            return celix::ServiceTracker{nullptr};
        }
    }

    int useAnyServices(const std::string& svcName, celix::UseServiceOptions<void> opts, const std::shared_ptr<celix::IResourceBundle>& requester) const {
        std::vector<std::shared_ptr<const celix::impl::SvcEntry>> matches{};
        {
            std::lock_guard<std::mutex> lock{services.mutex};
            if (svcName.empty()) {
                //LOG(DEBUG) << "finding ALL services";
                for (const auto& pair : services.cache) {
                    if (opts.filter.isEmpty() || opts.filter.match(pair.second->props)) {
                        pair.second->incrUsage();
                        matches.push_back(pair.second);
                    }
                }
            } else {
                const auto it = services.registry.find(svcName);
                if (it != services.registry.end()) {
                    const auto &matchingServices = it->second;
                    for (const std::shared_ptr<const celix::impl::SvcEntry> &entry : matchingServices) {
                        if (opts.filter.isEmpty() || opts.filter.match(entry->props)) {
                            entry->incrUsage();
                            matches.push_back(entry);
                        }
                    }
                }
            }
        }

        for (const std::shared_ptr<const celix::impl::SvcEntry> &entry : matches) {
            std::shared_ptr<void> rawSvc = entry->service(*requester);
            std::shared_ptr<void> svc{rawSvc.get(), [entry](void *) {
                entry->decrUsage();
            }};
            if (opts.use) {
                opts.use(svc);
            }
            if (opts.useWithProperties) {
                opts.useWithProperties(svc, entry->props);
            }
            if (opts.useWithOwner) {
                opts.useWithOwner(svc, entry->props, *entry->owner);
            }
        }

        return (int)matches.size();
    }

    bool useAnyService(const std::string &svcName, celix::UseServiceOptions<void> opts, const std::shared_ptr<celix::IResourceBundle>& requester) const {
        std::shared_ptr<const celix::impl::SvcEntry> match = nullptr;
        {
            std::lock_guard<std::mutex> lock{services.mutex};
            if (svcName.empty()) {
                //LOG(DEBUG) << "finding ALL services";
                for (const auto& pair : services.cache) {
                    if (opts.filter.isEmpty() || opts.filter.match(pair.second->props)) {
                        pair.second->incrUsage();
                        match = pair.second;
                        break;
                    }
                }
            } else {
                const auto it = services.registry.find(svcName);
                if (it != services.registry.end()) {
                    const auto &namedServices = it->second;
                    for (const std::shared_ptr<const celix::impl::SvcEntry> &visit : namedServices) {
                        if (opts.filter.isEmpty() || opts.filter.match(visit->props)) {
                            visit->incrUsage();
                            match = visit;
                            break;
                        }
                    }
                }
            }
        }

        if (match != nullptr) {
            std::shared_ptr<void> rawSvc = match->service(*requester);
            std::shared_ptr<void> svc{rawSvc.get(), [match](void *) {
                match->decrUsage();
            }};
            if (opts.use) {
                opts.use(svc);
            }
            if (opts.useWithProperties) {
                opts.useWithProperties(svc, match->props);
            }
            if (opts.useWithOwner) {
                opts.useWithOwner(svc, match->props, *match->owner);
            }
        }

        return match != nullptr;
    }

    std::vector<long> findAnyServices(const std::string &svcName, const celix::Filter& filter) const {
        //TODO order by rank
        std::vector<long> result{};

        std::lock_guard<std::mutex> lock{services.mutex};
        if (svcName.empty()) {
            //LOG(DEBUG) << "finding ALL services";
            for (const auto &pair : services.cache) {
                if (filter.isEmpty() || filter.match(pair.second->props)) {
                    result.push_back(pair.first);
                }
            }
        } else {
            const auto it = services.registry.find(svcName);
            if (it != services.registry.end()) {
                const auto &namedServices = it->second;
                for (const auto &visit : namedServices) {
                    if (filter.isEmpty() || filter.match(visit->props)) {
                        result.push_back(visit->svcId);
                    }
                }
            }
        }
        return result;
    }

    long nrOfServiceTrackers() const {
        std::lock_guard<std::mutex> lck{trackers.mutex};
        return trackers.cache.size();
    }

    long nrOfRegisteredServices() const {
        std::lock_guard<std::mutex> lock{services.mutex};
        return services.cache.size();
    }

    const std::string& name() const {
        return regName;
    }
};



/**********************************************************************************************************************
  Service Registry Implementation
 **********************************************************************************************************************/


celix::ServiceRegistry::ServiceRegistry(std::string name) : pimpl{new ServiceRegistry::Impl{std::move(name)}} {}
celix::ServiceRegistry::ServiceRegistry(celix::ServiceRegistry &&) noexcept = default;
celix::ServiceRegistry& celix::ServiceRegistry::operator=(celix::ServiceRegistry &&) = default;
celix::ServiceRegistry::~ServiceRegistry() {
    if (pimpl) {
        //TODO
    }
}

int celix::ServiceRegistry::useAnyServices(const std::string& svcName, celix::UseServiceOptions<void> opts, const std::shared_ptr<celix::IResourceBundle>& requester) const {
    return pimpl->useAnyServices(svcName, std::move(opts), requester);
}

//int celix::ServiceRegistry::useAnyFunctionServices(celix::UseFunctionServiceOptions<std::function<void()>> /*opts*/, const std::shared_ptr<celix::IResourceBundle>& /*requester*/) const {
//    //TODO
//    LOG(ERROR) << "TODO IMPL";
//    return 0;
//}

bool celix::ServiceRegistry::useAnyService(const std::string &svcName, celix::UseServiceOptions<void> opts, const std::shared_ptr<celix::IResourceBundle>& requester) const {
    return pimpl->useAnyService(svcName, std::move(opts), requester);
}

//bool celix::ServiceRegistry::useAnyFunctionService(celix::UseFunctionServiceOptions<std::function<void()>> /*opts*/, const std::shared_ptr<celix::IResourceBundle>& /*requester*/) const {
//    //TODO
//    LOG(ERROR) << "TODO IMPL";
//    return false;
//}

std::vector<long> celix::ServiceRegistry::findAnyServices(const std::string &svcName, const celix::Filter& filter) const {
    return pimpl->findAnyServices(svcName, filter);
}


celix::ServiceTracker celix::ServiceRegistry::trackAnyServices(const std::string &svcName, celix::ServiceTrackerOptions<void> opts, const std::shared_ptr<celix::IResourceBundle>& requester) {
    return pimpl->trackAnyServices(svcName, std::move(opts), requester);
}

//celix::ServiceTracker celix::ServiceRegistry::trackAnyFunctionService(celix::FunctionServiceTrackerOptions<void()> /*opts*/, const std::shared_ptr<celix::IResourceBundle>& /*requester*/) {
//    //TODO return pimpl->trackAnyFunctionService(std::move(opts));
//    LOG(ERROR) << "TODO IMPL";
//    return celix::ServiceTracker{nullptr};
//}

celix::ServiceRegistration celix::ServiceRegistry::registerAnyService(
        const std::string &svcName,
        std::shared_ptr<void> svc,
        celix::Properties props,
        const std::shared_ptr<celix::IResourceBundle> &owner) {
    return pimpl->registerAnyService(svcName, std::move(svc), nullptr, std::move(props), owner);
}

celix::ServiceRegistration celix::ServiceRegistry::registerAnyServiceFactory(
        const std::string &svcName,
        std::shared_ptr<celix::IServiceFactory<void>> factory,
        celix::Properties props,
        const std::shared_ptr<celix::IResourceBundle> &owner) {
    return pimpl->registerAnyService(svcName, nullptr, std::move(factory), std::move(props), owner);
}

//celix::ServiceRegistration celix::ServiceRegistry::registerAnyFunctionService(
//        const std::string &functionName,
//        std::function<void> &&function,
//        celix::Properties props,
//        const std::shared_ptr<celix::IResourceBundle> &owner) {
//    return pimpl->registerAnyFunctionService(functionName, std::forward<std::function<void>>(function), std::move(props), owner);
//}

long celix::ServiceRegistry::nrOfServiceTrackers() const {
    return pimpl->nrOfServiceTrackers();
}

long celix::ServiceRegistry::nrOfRegisteredServices() const {
    return pimpl->nrOfRegisteredServices();
}

std::vector<std::string> celix::ServiceRegistry::listAllRegisteredServiceNames() const {
    return pimpl->listAllRegisteredServiceNames();
}

const std::string& celix::ServiceRegistry::name() const {
    return pimpl->name();
}