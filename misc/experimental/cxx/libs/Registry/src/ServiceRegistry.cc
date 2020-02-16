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
#include <assert.h>
#include <celix/ServiceRegistry.h>


#include "celix/Constants.h"
#include "celix/ServiceRegistry.h"
#include "celix/Filter.h"

static celix::Filter emptyFilter{""};
static std::string emptyString{};

namespace {

    struct SvcEntry {
        explicit SvcEntry(std::shared_ptr<const celix::IResourceBundle> _owner, long _svcId, std::string _svcName, std::shared_ptr<void> _svc, std::shared_ptr<celix::IServiceFactory<void>> _svcFac,
                 celix::Properties &&_props) :
                owner{std::move(_owner)}, svcId{_svcId}, svcName{std::move(_svcName)},
                props{std::forward<celix::Properties>(_props)},
                ranking{celix::getPropertyAsLong(props, celix::SERVICE_RANKING, 0L)},
                svc{_svc}, svcFactory{_svcFac} {}

        SvcEntry(SvcEntry &&rhs) = delete;
        SvcEntry &operator=(SvcEntry &&rhs) = delete;
        SvcEntry(const SvcEntry &rhs) = delete;
        SvcEntry &operator=(const SvcEntry &rhs) = delete;

        const std::shared_ptr<const celix::IResourceBundle> owner;
        const long svcId;
        const std::string svcName;
        const celix::Properties props;
        const long ranking;

        void *service(const celix::IResourceBundle &requester) const {
            if (svcFactory) {
                return svcFactory->getService(requester, props);
            } else {
                return svc.get();
            }
        }

        bool factory() const { return svcFactory != nullptr; }

        void incrUsage() const {
            //TODO look at atomics or shared_ptr to handled to counts / sync
            std::lock_guard<std::mutex> lck{mutex};
            usage += 1;
        }

        void decrUsage() const {
            std::lock_guard<std::mutex> lck{mutex};
            if (usage == 0) {
                LOG(ERROR) << "Usage count decrease below 0!" << std::endl;
            } else {
                usage -= 1;
            }
            cond.notify_all();
        }

        void waitTillUnused() const {
            std::unique_lock<std::mutex> lock{mutex};
            cond.wait(lock, [this]{return usage == 0;});
        }
    private:
        //svc, svcSharedPtr or svcFactory is set
        const std::shared_ptr<void> svc;
        const std::shared_ptr<celix::IServiceFactory<void>> svcFactory;


        //sync TODO refactor to atomics
        mutable std::mutex mutex{};
        mutable std::condition_variable cond{};
        mutable int usage{1};
    };

    struct SvcEntryLess {
        bool operator()( const std::shared_ptr<const SvcEntry>& lhs, const std::shared_ptr<const SvcEntry>& rhs ) const {
            if (lhs->ranking == rhs->ranking) {
                return lhs->svcId < rhs->svcId;
            } else {
                return lhs->ranking > rhs->ranking; //note inverse -> higher rank first in set
            }
        }
    };

    struct SvcTrackerEntry {
        const long id;
        const std::shared_ptr<const celix::IResourceBundle> owner;
        const std::string svcName;
        const celix::ServiceTrackerOptions<void> opts;
        const celix::Filter filter;

        explicit SvcTrackerEntry(long _id, std::shared_ptr<const celix::IResourceBundle> _owner, std::string _svcName, celix::ServiceTrackerOptions<void> _opts) :
            id{_id}, owner{std::move(_owner)}, svcName{std::move(_svcName)}, opts{std::move(_opts)}, filter{opts.filter} {}

        SvcTrackerEntry(SvcTrackerEntry &&rhs) = delete;
        SvcTrackerEntry &operator=(SvcTrackerEntry &&rhs) = delete;
        SvcTrackerEntry(const SvcTrackerEntry &rhs) = delete;
        SvcTrackerEntry &operator=(const SvcTrackerEntry &rhs) = delete;
        ~SvcTrackerEntry() {}

        void clear() {
            std::vector<std::shared_ptr<const SvcEntry>> removeEntries{};
            {
                std::lock_guard<std::mutex> lck{tracked.mutex};
                for (const auto &entry : tracked.entries) {
                    removeEntries.push_back(entry.first);
                }
            }
            for (auto &entry : removeEntries) {
                remMatch(entry);
            }
        }

        bool valid() const {
            return !svcName.empty() && filter.isValid();
        }

        bool match(const SvcEntry& entry) const {
            //note should only called for the correct svcName
            assert(svcName == entry.svcName);

            if (filter.isEmpty()) {
                return true;
            } else {
                return filter.match(entry.props);
            }
        }

        void addMatch(std::shared_ptr<const SvcEntry> entry) {
            //increase usage so that services cannot be removed while a service tracker is still active

            //new custom deleter which arranges the count & sync for the used services

            entry->incrUsage();

            void *rawSvc = entry->service(*owner);
            //NOTE creating a shared_ptr with a custom deleter, so that the SvcEntry usage is synced with this shared_ptr.
            auto svc = std::shared_ptr<void>{rawSvc, [entry](void *) {
                entry->decrUsage();
            }};

            {
                std::lock_guard<std::mutex> lck{tracked.mutex};
                tracked.entries.emplace(entry, svc);
            }

            //call callbacks
            if (opts.preServiceUpdateHook) {
                opts.preServiceUpdateHook();
            }
            callSetCallbacks();
            callAddRemoveCallbacks(entry, svc, true);
            callUpdateCallbacks();
            if (opts.postServiceUpdateHook) {
                opts.postServiceUpdateHook();
            }
        }

        void remMatch(const std::shared_ptr<const SvcEntry> &entry) {
            std::shared_ptr<void> svc{};
            {
                std::lock_guard<std::mutex> lck{tracked.mutex};
                auto it = tracked.entries.find(entry);
                svc = it->second;
                tracked.entries.erase(it);
            }

            //call callbacks
            if (opts.preServiceUpdateHook) {
                opts.preServiceUpdateHook();
            }
            callSetCallbacks(); //note also removed highest if that was set to this svc
            callAddRemoveCallbacks(entry, svc, false);
            callUpdateCallbacks();
            if (opts.postServiceUpdateHook) {
                opts.postServiceUpdateHook();
            }

            //note sync will be done on the SvcEntry usage, which is controlled by the tracker svc shared ptr
        }

        void callAddRemoveCallbacks(const std::shared_ptr<const SvcEntry>& entry, std::shared_ptr<void>& svc, bool add) {
            auto& update = add ? opts.add : opts.remove;
            auto& updateWithProps = add ? opts.addWithProperties : opts.removeWithProperties;
            auto& updateWithOwner = add ? opts.addWithOwner : opts.removeWithOwner;
            //The more explicit callbacks are called first (i.e first WithOwner)
            if (updateWithOwner) {
                updateWithOwner(svc, entry->props, *entry->owner);
            }
            if (updateWithProps) {
                updateWithProps(svc, entry->props);
            }
            if (update) {
                update(svc);
            }
        }

        void callSetCallbacks() {
            std::shared_ptr<void> currentHighestSvc{};
            std::shared_ptr<const SvcEntry> currentHighestSvcEntry{};
            bool highestUpdated = false;
            {
                std::lock_guard<std::mutex> lck{tracked.mutex};
                auto begin = tracked.entries.begin();
                if (begin != tracked.entries.end()) {
                    currentHighestSvc = begin->second;
                    currentHighestSvcEntry = begin->first;
                }
                if (currentHighestSvc != tracked.highest) {
                    tracked.highest = currentHighestSvc;
                    highestUpdated = true;
                }
            }

            //TODO race condition. highest can be updated because lock is released.

            if (highestUpdated) {
                static celix::Properties emptyProps{};
                static celix::EmptyResourceBundle emptyOwner{};
                //The more explicit callbacks are called first (i.e first WithOwner)
                if (opts.setWithOwner) {
                    opts.setWithOwner(currentHighestSvc,
                                      currentHighestSvc == nullptr ? emptyProps : currentHighestSvcEntry->props,
                                      currentHighestSvc == nullptr ? emptyOwner : *currentHighestSvcEntry->owner);
                }
                if (opts.setWithProperties) {
                    opts.setWithProperties(currentHighestSvc, currentHighestSvc == nullptr ? emptyProps : currentHighestSvcEntry->props);
                }
                if (opts.set) {
                    opts.set(currentHighestSvc); //note can be nullptr
                }
            }
        }

        void callUpdateCallbacks() {
            std::vector<std::tuple<std::shared_ptr<void>, const celix::Properties*, const celix::IResourceBundle*>> rankedServices{};
            if (opts.update || opts.updateWithProperties || opts.updateWithOwner) {
                //fill vector
                std::lock_guard<std::mutex> lck{tracked.mutex};
                rankedServices.reserve(tracked.entries.size());
                for (auto &entry : tracked.entries) {
                    rankedServices.push_back(std::make_tuple(entry.second, &entry.first->props, entry.first->owner.get()));
                }
            }
            //The more explicit callbacks are called first (i.e first WithOwner)
            if (opts.updateWithOwner) {
                opts.updateWithOwner(rankedServices);
            }
            if (opts.updateWithProperties) {
                std::vector<std::tuple<std::shared_ptr<void>, const celix::Properties*>> rnk{};
                for (auto &tuple : rankedServices) {
                    rnk.push_back(std::make_pair(std::get<0>(tuple), std::get<1>(tuple)));
                }
                opts.updateWithProperties(std::move(rnk));
            }
            if (opts.update) {
                std::vector<std::shared_ptr<void>> rnk{};
                for (auto &tuple : rankedServices) {
                    rnk.push_back(std::get<0>(tuple));
                }
                opts.update(std::move(rnk));
            }
        }

        int count() const {
            std::lock_guard<std::mutex> lck{tracked.mutex};
            return (int)tracked.entries.size();
        }

        void incrUsage() const {
            std::lock_guard<std::mutex> lck{mutex};
            usage += 1;
        }

        void decrUsage() const {
            std::lock_guard<std::mutex> lck{mutex};
            if (usage == 0) {
                LOG(ERROR) << "Usage count decrease below 0!" << std::endl;
            } else {
                usage -= 1;
            }
            cond.notify_all();
        }

        void waitTillUnused() const {
            std::unique_lock<std::mutex> lock{mutex};
            cond.wait(lock, [this]{return usage == 0;});
        }
    private:
        struct {
            mutable std::mutex mutex{}; //protects matchedEntries & highestRanking
            std::map<std::shared_ptr<const SvcEntry>, std::shared_ptr<void>, SvcEntryLess> entries{};
            std::shared_ptr<void> highest{};
        } tracked{};


        mutable std::mutex mutex{};
        mutable std::condition_variable cond{};
        mutable int usage{1};
    };
}

/**********************************************************************************************************************
  Impl classes
 **********************************************************************************************************************/

class celix::ServiceRegistration::Impl {
public:
    const std::shared_ptr<const SvcEntry> entry;
    const std::function<void()> unregisterCallback;
    bool registered; //TODO make atomic?
};

class celix::ServiceTracker::Impl {
public:
    const std::shared_ptr<SvcTrackerEntry> entry;
    const std::function<void()> untrackCallback;
    bool active; //TODO make atomic?
};

class celix::ServiceRegistry::Impl {
public:
    Impl(std::string _regName) : regName{std::move(_regName)} {}

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
         * map key is svcName and the map value is a set of SvcEntry structs.
         * note: The SvcEntry set is ordered (i.e. not a hashset with O(1) access) to ensure that service ranking and
         * service id can be used for their position in the set (see SvcEntryLess).
         */
        std::unordered_map<std::string, std::set<std::shared_ptr<const SvcEntry>, SvcEntryLess>> registry{};

        //Cache map for faster and easier access based on svcId.
        std::unordered_map<long, std::shared_ptr<const SvcEntry>> cache{};
    } services{};

    struct {
        mutable std::mutex mutex{};
        long nextTrackerId = 1L;

        /* Note same ordering as services registry, expect the ranking order requirement,
         * so that it is easier and faster to update the trackers.
         */
        std::unordered_map<std::string, std::set<std::shared_ptr<SvcTrackerEntry>>> registry{};
        std::unordered_map<long, std::shared_ptr<SvcTrackerEntry>> cache{};
    } trackers{};

    celix::ServiceRegistration registerService(std::string serviceName, std::shared_ptr<void> svc, std::shared_ptr<celix::IServiceFactory<void>> factory, celix::Properties props, std::shared_ptr<const celix::IResourceBundle> owner) {
        props[celix::SERVICE_NAME] = std::move(serviceName);
        std::string &svcName = props[celix::SERVICE_NAME];

        std::lock_guard<std::mutex> lock{services.mutex};
        long svcId = services.nextSvcId++;
        props[celix::SERVICE_ID] = std::to_string(svcId);

        //Add to registry
        std::shared_ptr<const celix::IResourceBundle> bnd = owner ? owner : emptyBundle;
        props[celix::SERVICE_BUNDLE] = std::to_string(bnd->id());

        if (factory) {
            DLOG(INFO) << "Registering service factory '" << svcName << "' from bundle id " << bnd->id() << std::endl;
        } else {
            DLOG(INFO) << "Registering service '" << svcName << "' from bundle id " << bnd->id() << std::endl;
        }

        const auto it = services.registry[svcName].emplace(new SvcEntry{std::move(bnd), svcId, svcName, std::move(svc), std::move(factory), std::move(props)});
        assert(it.second); //should always lead to a new entry
        const std::shared_ptr<const SvcEntry> &entry = *it.first;

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
        auto *impl = new celix::ServiceRegistration::Impl{
                .entry = entry,
                .unregisterCallback = std::move(unreg),
                .registered = true
        };

        return celix::ServiceRegistration{impl};
    }

    void unregisterService(long svcId) {
        if (svcId <= 0) {
            return; //silent ignore
        }

        std::shared_ptr<const SvcEntry> match{nullptr};

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

        std::shared_ptr<SvcTrackerEntry> match{nullptr};
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

    void updateTrackers(const std::shared_ptr<const SvcEntry> &entry, bool adding) {
        std::vector<std::shared_ptr<SvcTrackerEntry>> matchedTrackers{};
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
};



/**********************************************************************************************************************
  Service Registry Implementation
 **********************************************************************************************************************/


celix::ServiceRegistry::ServiceRegistry(std::string name) : pimpl{new ServiceRegistry::Impl{std::move(name)}} {}
celix::ServiceRegistry::ServiceRegistry(celix::ServiceRegistry &&) = default;
celix::ServiceRegistry& celix::ServiceRegistry::operator=(celix::ServiceRegistry &&) = default;
celix::ServiceRegistry::~ServiceRegistry() {
    if (pimpl) {
        //TODO
    }
}

const std::string& celix::ServiceRegistry::name() const { return pimpl->regName; }

celix::ServiceRegistration celix::ServiceRegistry::registerGenericService(std::string svcName,
                                                                          std::shared_ptr<void> svc,
                                                                          celix::Properties props,
                                                                          std::shared_ptr<const celix::IResourceBundle> owner) {
    return pimpl->registerService(std::move(svcName), std::move(svc), {}, std::move(props), std::move(owner));
}

celix::ServiceRegistration celix::ServiceRegistry::registerServiceFactory(std::string svcName, std::shared_ptr<celix::IServiceFactory<void>> factory, celix::Properties props, std::shared_ptr<const celix::IResourceBundle> owner) {
    return pimpl->registerService(std::move(svcName), {}, std::move(factory), std::move(props), std::move(owner));
}

//TODO add useService(s) call to ServiceTracker object for fast service access

//TODO move to Impl
celix::ServiceTracker celix::ServiceRegistry::trackAnyServices(std::string svcName,
                                                               celix::ServiceTrackerOptions<void> options,
                                                               std::shared_ptr<const celix::IResourceBundle> requester) {
    //TODO create new tracker event and start new thread to update track trackers
    long trkId = 0;
    {
        std::lock_guard<std::mutex> lck{pimpl->trackers.mutex};
        trkId = pimpl->trackers.nextTrackerId++;
    }

    auto trkEntry = std::shared_ptr<SvcTrackerEntry>{new SvcTrackerEntry{trkId, requester, std::move(svcName), std::move(options)}};
    if (trkEntry->valid()) {

        //find initial services and add new tracker to registry.
        //NOTE two locks to ensure no new services can be added/removed during initial tracker setup
        std::vector<std::shared_ptr<const SvcEntry>> services{};
        {
            std::lock_guard<std::mutex> lck1{pimpl->services.mutex};
            for (auto &svcEntry : pimpl->services.registry[trkEntry->svcName]) {
                if (trkEntry->match(*svcEntry)) {
                    svcEntry->incrUsage();
                    services.push_back(svcEntry);
                }
            }

            std::lock_guard<std::mutex> lck2{pimpl->trackers.mutex};
            pimpl->trackers.registry[trkEntry->svcName].insert(trkEntry);
            pimpl->trackers.cache[trkEntry->id] = trkEntry;
        }
        auto future = std::async([&]{
            for (auto &svcEntry : services) {
                trkEntry->addMatch(svcEntry);
                svcEntry->decrUsage();
            }
        });
        future.wait();
        trkEntry->decrUsage(); //note trkEntry usage started at 1

        auto untrack = [this, trkId]() -> void {
            this->pimpl->removeTracker(trkId);
        };
        auto *impl = new celix::ServiceTracker::Impl{
                .entry = trkEntry,
                .untrackCallback = std::move(untrack),
                .active = true
        };
        return celix::ServiceTracker{impl};
    } else {
        trkEntry->decrUsage(); //note usage is 1 at creation
        LOG(ERROR) << "Cannot create tracker. Invalid filter?" << std::endl;
        return celix::ServiceTracker{nullptr};
    }
}

//TODO move to Impl
long celix::ServiceRegistry::nrOfServiceTrackers() const {
    std::lock_guard<std::mutex> lck{pimpl->trackers.mutex};
    return pimpl->trackers.cache.size();
}
        
//TODO unregister tracker with remove tracker event in a new thread
//TODO move to Impl
std::vector<long> celix::ServiceRegistry::findAnyServices(const std::string &name, const std::string &f) const {
    std::vector<long> result{};
    celix::Filter filter = f;
    if (!filter.isValid()) {
        LOG(WARNING) << "Invalid filter (" << f << ") provided. Cannot find services" << std::endl;
        return result;
    }

    std::lock_guard<std::mutex> lock{pimpl->services.mutex};
    const auto it = pimpl->services.registry.find(name);
    if (it != pimpl->services.registry.end()) {
        const auto &services = it->second;
        for (const auto &visit : services) {
            if (filter.isEmpty() || filter.match(visit->props)) {
                result.push_back(visit->svcId);
            }
        }
    }
    return result;
}

//TODO move to Impl
long celix::ServiceRegistry::nrOfRegisteredServices() const {
    std::lock_guard<std::mutex> lock{pimpl->services.mutex};
    return pimpl->services.cache.size();
}

//TODO move to Impl
int celix::ServiceRegistry::useAnyServices(const std::string &svcName,
                                           const std::function<void(const std::shared_ptr<void> &svc, const celix::Properties &props,
                                                              const celix::IResourceBundle &bnd)> &use,
                                           const std::string &f,
                                           std::shared_ptr<const celix::IResourceBundle> requester) const {

    celix::Filter filter = f;
    if (!filter.isValid()) {
        LOG(WARNING) << "Invalid filter (" << f << ") provided. Cannot find services" << std::endl;
        return 0;
    }

    std::vector<std::shared_ptr<const SvcEntry>> matches{};
    {
        std::lock_guard<std::mutex> lock{pimpl->services.mutex};
        const auto it = pimpl->services.registry.find(svcName);
        if (it != pimpl->services.registry.end()) {
            const auto &services = it->second;
            for (const std::shared_ptr<const SvcEntry> &entry : services) {
                if (filter.isEmpty() || filter.match(entry->props)) {
                    entry->incrUsage();
                    matches.push_back(entry);
                }
            }
        }
    }

    for (const std::shared_ptr<const SvcEntry> &entry : matches) {
        void *rawSvc = entry->service(*requester);
        std::shared_ptr<void> svc{rawSvc, [entry](void *) {
            entry->decrUsage();
        }};
        use(svc, entry->props, *entry->owner);
    }

    return (int)matches.size();
}

//TODO move to Impl
bool celix::ServiceRegistry::useAnyService(
        const std::string &svcName,
        const std::function<void(const std::shared_ptr<void> &svc, const celix::Properties &props, const celix::IResourceBundle &bnd)> &use,
        const std::string &f,
        std::shared_ptr<const celix::IResourceBundle> requester) const {

    celix::Filter filter = f;
    if (!filter.isValid()) {
        LOG(WARNING) << "Invalid filter (" << f << ") provided. Cannot find services" << std::endl;
        return false;
    }

    std::shared_ptr<const SvcEntry> match = nullptr;
    {
        std::lock_guard<std::mutex> lock{pimpl->services.mutex};
        const auto it = pimpl->services.registry.find(svcName);
        if (it != pimpl->services.registry.end()) {
            const auto &services = it->second;
            for (const std::shared_ptr<const SvcEntry> &visit : services) {
                if (filter.isEmpty() || filter.match(visit->props)) {
                    visit->incrUsage();
                    match = visit;
                    break;
                }
            }
        }
    }

    if (match != nullptr) {
        void *rawSvc = match->service(*requester);
        std::shared_ptr<void> svc{rawSvc, [match](void *) {
            match->decrUsage();
        }};
        use(svc, match->props, *match->owner);
    }

    return match != nullptr;
}

std::vector<std::string> celix::ServiceRegistry::listAllRegisteredServiceNames() const {
    return pimpl->listAllRegisteredServiceNames();
}

/**********************************************************************************************************************
  Service Registration Implementation
 **********************************************************************************************************************/

celix::ServiceRegistration::ServiceRegistration() : pimpl{nullptr} {}

celix::ServiceRegistration::ServiceRegistration(celix::ServiceRegistration::Impl *impl) : pimpl{impl} {}
celix::ServiceRegistration::ServiceRegistration(celix::ServiceRegistration &&) noexcept = default;
celix::ServiceRegistration& celix::ServiceRegistration::operator=(celix::ServiceRegistration &&) noexcept = default;
celix::ServiceRegistration::~ServiceRegistration() { unregister(); }

long celix::ServiceRegistration::serviceId() const { return pimpl ? pimpl->entry->svcId : -1L; }
bool celix::ServiceRegistration::valid() const { return serviceId() >= 0; }
bool celix::ServiceRegistration::factory() const { return pimpl && pimpl->entry->factory(); }
bool celix::ServiceRegistration::registered() const {return pimpl && pimpl->registered; }

void celix::ServiceRegistration::unregister() {
    if (pimpl && pimpl->registered) {
        pimpl->registered = false; //TODO make thread safe
        pimpl->unregisterCallback();
        DLOG(INFO) << "Unregister " << *this;
    }
}

const celix::Properties& celix::ServiceRegistration::properties() const {
    static const celix::Properties empty{};
    return pimpl ? pimpl->entry->props : empty;
}

const std::string& celix::ServiceRegistration::serviceName() const {
    static const std::string empty{};
    if (pimpl) {
        return celix::getProperty(pimpl->entry->props, celix::SERVICE_NAME, empty);
    }
    return empty;
}

std::ostream& operator<<(std::ostream &out, const celix::ServiceRegistration& reg) {
    out << "ServiceRegistration[service_id=" << reg.serviceId() << ",service_name=" << reg.serviceName() << "]";
    return out;
}




/**********************************************************************************************************************
  Service Tracker Implementation
 **********************************************************************************************************************/

celix::ServiceTracker::ServiceTracker() : pimpl{nullptr} {}

celix::ServiceTracker::ServiceTracker(celix::ServiceTracker::Impl *impl) : pimpl{impl} {}

celix::ServiceTracker::~ServiceTracker() {
    if (pimpl && pimpl->active) {
        pimpl->untrackCallback();
    }
}


void celix::ServiceTracker::stop() {
    if (pimpl && pimpl->active) { //TODO make thread safe
        pimpl->untrackCallback();
        pimpl->active = false;
    }
}

celix::ServiceTracker::ServiceTracker(celix::ServiceTracker &&) noexcept = default;
celix::ServiceTracker& celix::ServiceTracker::operator=(celix::ServiceTracker &&) noexcept = default;

int celix::ServiceTracker::trackCount() const { return pimpl ? pimpl->entry->count() : 0; }
const std::string& celix::ServiceTracker::serviceName() const { return pimpl? pimpl->entry->svcName : emptyString; }
const celix::Filter& celix::ServiceTracker::filter() const { return pimpl ? pimpl->entry->filter : emptyFilter; }
bool celix::ServiceTracker::valid() const { return pimpl != nullptr; }
