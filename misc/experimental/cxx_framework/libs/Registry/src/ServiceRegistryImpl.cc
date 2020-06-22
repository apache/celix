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

#include "celix/ServiceRegistry.h"


#include <unordered_map>
#include <mutex>
#include <set>
#include <future>
#include <cassert>

#include "celix/Constants.h"
#include "celix/Filter.h"

#include "ServiceTrackerImpl.h"

#define LOGGER celix::getLogger("celix::ServiceRegistry")

/**********************************************************************************************************************
  Impl classes
 **********************************************************************************************************************/

namespace celix::impl {

    class ServiceRegistryImpl : public celix::ServiceRegistry {
    public:
        explicit ServiceRegistryImpl(std::string_view _regName) : regName{std::string{_regName}} {}

        ~ServiceRegistryImpl() override = default;

        celix::ServiceRegistration registerAnyService(const std::string& svcName, std::shared_ptr<void> service, celix::Properties props, const std::shared_ptr<celix::IResourceBundle>& owner) override {
            return registerAnyServiceOrServiceFactory(svcName, std::move(service), nullptr, std::move(props), owner);
        };

        celix::ServiceRegistration registerAnyServiceFactory(const std::string& svcName, std::shared_ptr<celix::IServiceFactory<void>> factory, celix::Properties props, const std::shared_ptr<celix::IResourceBundle>& owner) override {
            return registerAnyServiceOrServiceFactory(svcName, nullptr, std::move(factory), std::move(props), owner);
        }

        std::vector<std::string> listAllRegisteredServiceNames() const override {
            std::vector<std::string> result{};
            std::lock_guard<std::mutex> lck{services.mutex};
            for (const auto &pair : services.registry) {
                result.emplace_back(std::string{pair.first});
            }
            return result;
        }

        celix::ServiceTracker trackAnyServices(const std::string &svcName, celix::ServiceTrackerOptions<void> opts,
                                               const std::shared_ptr<celix::IResourceBundle> &requester) override {
            //TODO create new tracker event and start new thread to update track trackers
            assert(!svcName.empty()); //TODO support empty name
            auto req = requester ? requester : emptyBundle;
            long trkId = trackers.nextTrackerId.fetch_add(1);

            auto trkEntry = std::make_shared<celix::impl::SvcTrackerEntry>(trkId, svcName, req, std::move(opts));
            if (trkEntry->valid()) {

                //find initial services and add new tracker to registry.
                //NOTE two locks to ensure no new services can be added/removed during initial tracker setup
                std::vector<std::shared_ptr<celix::impl::SvcEntry>> namedServices{};
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
                LOGGER->error("Cannot create tracker. Invalid filter?");
                return celix::ServiceTracker{nullptr};
            }
        }

        int useAnyServices(const std::string &svcName, celix::UseAnyServiceOptions opts,
                           const std::shared_ptr<celix::IResourceBundle> &requester) const override {
            auto entries = collectSvcEntries(svcName, opts.filter);

            if (opts.limit == 1 && entries.empty() && opts.waitFor > std::chrono::milliseconds{0}) {
                //wait til entries has at least one entry of time period is expired
                //TODO add condition on services and use wait for on condition
                auto start = std::chrono::steady_clock::now();
                auto now = std::chrono::steady_clock::now();
                while ((now - start) < opts.waitFor) {
                    std::this_thread::sleep_for(std::chrono::milliseconds{1});
                    now = std::chrono::steady_clock::now();
                    entries = collectSvcEntries(svcName, opts.filter);
                    if (!entries.empty()) {
                        break;
                    }
                }
            }

            if (svcName.empty()) {
                LOGGER->trace("Using ALL services with filter '{}'. Nr found {}.", opts.filter.toString(),
                              entries.size());
            } else {
                LOGGER->trace("Using services '{}' with filter '{}'. Nr found {}.", svcName, opts.filter.toString(),
                              entries.size());
            }

            int count = 0;
            for (const std::shared_ptr<celix::impl::SvcEntry> &entry : entries) {
                std::shared_ptr<void> rawSvc = entry->service(*requester);
                std::shared_ptr<void> svc{rawSvc.get(), [entry](void *) {
                    entry->decrUsage();
                }};
                if (opts.limit == 0 || count < opts.limit) {
                    //TODO call on thread pool
                    if (opts.use) {
                        opts.use(svc);
                    }
                    if (opts.useWithProperties) {
                        opts.useWithProperties(svc, entry->props);
                    }
                    if (opts.useWithOwner) {
                        opts.useWithOwner(svc, entry->props, *entry->owner);
                    }
                    ++count;
                }
            }

            return count;
        }

        std::vector<long> findAnyServices(const std::string &svcName, const celix::Filter &filter) const override {
            auto entries = collectSvcEntries(svcName, filter);

            if (svcName.empty()) {
                LOGGER->trace("Finding ALL services with filter '{}'. Nr found {}.", filter.toString(), entries.size());
            } else {
                LOGGER->trace("Finding services '{}' with filter '{}'. Nr found {}.", svcName, filter.toString(),
                              entries.size());
            }

            std::vector<long> result{};
            result.reserve(entries.size());
            for (const auto &entry: entries) {
                result.push_back(entry->svcId);
                entry->decrUsage();
            }
            return result;
        }

        long nrOfServiceTrackers() const override {
            std::lock_guard<std::mutex> lck{trackers.mutex};
            return trackers.cache.size();
        }

        long nrOfRegisteredServices() const override {
            std::lock_guard<std::mutex> lock{services.mutex};
            return services.cache.size();
        }

        const std::string &name() const override {
            return regName;
        }

    private:
        celix::ServiceRegistration registerAnyServiceOrServiceFactory(
                const std::string &svcName,
                std::shared_ptr<void> service,
                std::shared_ptr<celix::IServiceFactory<void>> factoryService,
                celix::Properties props,
                const std::shared_ptr<celix::IResourceBundle> &owner) {
            assert(!svcName.empty()); //TODO create and throw celix invalid argument/svcName exception
            assert(!(factoryService && service)); //cannot register both factory and 'normal' service
            long svcId = services.nextSvcId.fetch_add(1);
            std::shared_ptr<const celix::IResourceBundle> bnd = owner ? owner : emptyBundle;

            //TODO more info component uuid?
            props[celix::SERVICE_NAME] = svcName;
            props[celix::SERVICE_BUNDLE_ID] = std::to_string(bnd->id());
            props[celix::SERVICE_ID] = std::to_string(svcId);

            if (factoryService) {
                LOGGER->debug("Registering service factory '{}' with service id {} for bundle {} (id={}).", svcName,
                              svcId, bnd->name(), bnd->id());
            } else {
                //note service can be nullptr. TODO is this allowed?
                LOGGER->debug("Registering service '{}' with service id {} for bundle {} (id={}).", svcName, svcId,
                              bnd->name(), bnd->id());
            }

            auto entry = std::make_shared<celix::impl::SvcEntry>(std::move(bnd), svcId, svcName, std::move(service),
                                                                 std::move(factoryService), std::move(props));

            auto asyncRegistration = [this, entry] {
                {
                    //add to registry
                    std::lock_guard<std::mutex> lck{services.mutex};
                    services.registry[entry->svcName].insert(entry);
                    //Add to svcId cache
                    services.cache[entry->svcId] = entry;
                }

                //update trackers
                updateTrackers(entry, true);

                //done registering
                entry->setState(celix::ServiceRegistration::State::Registered);
                entry->decrUsage(); //NOTE creation of entry started at 1, this is the decrease for that
            };
            std::thread t{std::move(asyncRegistration)}; //TODO do registration on thread pool
            t.detach();

            //create unregister callback
            std::function<void()> unreg = [this, entry]() -> void {
                //TODO unreg on async thread

                entry->wait(); //ensure service is done registering
                entry->setState(celix::ServiceRegistration::State::Unregistering);
                this->unregisterService(entry->svcId);
                entry->setState(celix::ServiceRegistration::State::Unregistered);
            };
            auto *impl = celix::impl::createServiceRegistrationImpl(entry, std::move(unreg));
            return celix::ServiceRegistration{impl};
        }

        void unregisterService(long svcId) {
            if (svcId <= 0) {
                return; //silent ignore
            }

            std::shared_ptr<celix::impl::SvcEntry> match{nullptr};

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
                auto future = std::async([&] { updateTrackers(match, false); });
                future.wait();
                match->waitTillUnused();
            } else {
                LOGGER->warn("Cannot unregister service. Unknown service id {}.", svcId);
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
                auto future = std::async(
                        [&] { match->clear(); }); //ensure that all service are removed using the callbacks
                future.wait();
            } else {
                LOGGER->warn("Cannot remove tracker. Unknown tracker id {}.", trkId);
            }
        }

        void updateTrackers(const std::shared_ptr<celix::impl::SvcEntry> &entry, bool adding) {
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

        std::set<std::shared_ptr<celix::impl::SvcEntry>, celix::impl::SvcEntryLess>
        collectSvcEntries(const std::string &svcName, const celix::Filter &filter) const {
            //Collection all matching services in a set; this means that services are always called on service ranking order
            std::set<std::shared_ptr<celix::impl::SvcEntry>, celix::impl::SvcEntryLess> entries{};
            std::lock_guard<std::mutex> lock{services.mutex};
            if (svcName.empty()) {
                for (const auto &pair : services.cache) {
                    if (filter.isEmpty() || filter.match(pair.second->props)) {
                        pair.second->incrUsage();
                        entries.insert(pair.second);
                    }
                }
            } else {
                const auto it = services.registry.find(svcName);
                if (it != services.registry.end()) {
                    const auto &matchingServices = it->second;
                    for (const std::shared_ptr<celix::impl::SvcEntry> &entry : matchingServices) {
                        if (filter.isEmpty() || filter.match(entry->props)) {
                            entry->incrUsage();
                            entries.insert(entry);
                        }
                    }
                }
            }
            return entries;
        }


        const std::shared_ptr<const celix::IResourceBundle> emptyBundle = std::shared_ptr<const celix::IResourceBundle>{
                new celix::EmptyResourceBundle{}};
        const std::string regName;

        struct {
            std::atomic<long> nextSvcId{1L};


            mutable std::mutex mutex{}; //protect below
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
            std::unordered_map<std::string, std::set<std::shared_ptr<celix::impl::SvcEntry>, celix::impl::SvcEntryLess>> registry{};

            //Cache map for faster and easier access based on svcId.
            std::unordered_map<long, std::shared_ptr<celix::impl::SvcEntry>> cache{};
        } services{};

        struct {
            std::atomic<long> nextTrackerId{1L};

            mutable std::mutex mutex{}; //protect below

            /* Note same ordering as services registry, expect the ranking order requirement,
             * so that it is easier and faster to update the trackers.
             */
            std::unordered_map<std::string, std::set<std::shared_ptr<celix::impl::SvcTrackerEntry>>> registry{};
            std::unordered_map<long, std::shared_ptr<celix::impl::SvcTrackerEntry>> cache{};
        } trackers{};
    };
}

std::shared_ptr<celix::ServiceRegistry> celix::ServiceRegistry::create(std::string_view name) {
    return std::make_shared<celix::impl::ServiceRegistryImpl>(name);
}