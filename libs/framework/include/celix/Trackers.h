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

#include <cstdint>
#include <memory>
#include <mutex>
#include <atomic>
#include <cassert>
#include <set>
#include <unordered_map>
#include <functional>
#include <thread>

#include "celix_utils.h"
#include "celix_bundle_context.h"
#include "celix_framework.h"
#include "celix/FrameworkExceptions.h"
#include "celix/Properties.h"
#include "celix/Utils.h"
#include "celix/Bundle.h"
#include "celix/Constants.h"
#include "celix/Filter.h"

namespace celix {


    /**
     * @brief The tracker state.
     */
    enum class TrackerState : std::uint8_t {
        OPENING,
        OPEN,
        CLOSING,
        CLOSED
    };

    /**
     * @brief The AbstractTracker class is the base of all C++ Celix trackers.
     *
     * It defines how trackers are closed and manages the tracker state.
     *
     * This class can be used to create a vector of different (shared ptr)
     * trackers (i.e. ServiceTracker, BundleTracker, MetaTracker).
     *
     * AbstractTracker <-----------------------------------
     *  ^                               |                |
     *  |                               |                |
     *  GenericServiceTracker      BundleTracker    MetaTracker
     *  ^
     *  |
     *  ServiceTracker<I>
     *
     * @note Thread safe.
     */
    class AbstractTracker {
    public:
        explicit AbstractTracker(std::shared_ptr<celix_bundle_context_t> _cCtx) :
            cCtx{std::move(_cCtx)} {}

        virtual ~AbstractTracker() noexcept = default;

        /**
         * @brief Check if the tracker is open (state == OPEN)
         */
        bool isOpen() const {
            std::lock_guard<std::mutex> lck{mutex};
            return state == TrackerState::OPEN;
        }

        /**
         * @brief Get the current state of the tracker.
         */
        TrackerState getState() const {
            std::lock_guard<std::mutex> lck{mutex};
            return state;
        }

        /**
         * @brief Close the tracker (of the state is not CLOSED or CLOSING).
         *
         * This will be done sync so then the close() method return the
         * tracker is closed and all the needed callbacks have been called.
         */
        void close() {
            long localTrkId = -1;
            {
                std::lock_guard<std::mutex> lck{mutex};
                if (state == TrackerState::OPEN || state == TrackerState::OPENING) {
                    //not yet closed
                    state = TrackerState::CLOSING;
                    localTrkId = trkId;
                    trkId = -1;
                }
            }
            if (localTrkId >= 0) {
                celix_bundleContext_stopTracker(cCtx.get(), localTrkId);
                {
                    std::lock_guard<std::mutex> lck{mutex};
                    state = TrackerState::CLOSED;
                }
            }
        }

        /**
         * @brief Open the tracker (if the state is not OPEN or OPENING).
         *
         * This is done async, meaning that the actual opening of the tracker will be
         * done a a Celix event processed on the Celix event thread.
         *
         * @throws celix::Exception
         */
        virtual void open() = 0;

        /**
         * @brief Wait until a service tracker is completely OPEN or CLOSED.
         *
         * This method cannot be called on the Celix event thread.
         */
        void wait() const {
            bool needWaitOpening = false;
            bool needWaitClosing = false;
            long localId;
            {
                std::lock_guard<std::mutex> lck{mutex};
                localId = trkId;
                if (state == TrackerState::OPENING) {
                    needWaitOpening = true;
                } else if (state == TrackerState::CLOSING) {
                    needWaitClosing = true;
                }
            }
            if (needWaitOpening) {
                celix_bundleContext_waitForAsyncTracker(cCtx.get(), localId);
            }
            if (needWaitClosing) {
                celix_bundleContext_waitForAsyncStopTracker(cCtx.get(), localId);
            }
        }
    protected:
        /**
         * @brief Wait (if not on the Celix event thread) for the tracker to be OPEN or CLOSED.
         */
        void waitIfAble() const {
            auto* fw = celix_bundleContext_getFramework(cCtx.get());
            if (!celix_framework_isCurrentThreadTheEventLoop(fw)) {
                wait();
            }
        }

        template<typename T>
        static std::function<void(T*)> delCallback() {
            return [](T *tracker) {
                if (tracker->getState() == TrackerState::CLOSED) {
                    delete tracker;
                } else {
                    /*
                     * if open/opening -> close() -> new event on the Celix event thread
                     * if closing -> nop close() -> there is already a event on the Celix event thread to close
                     */
                    tracker->close();

                    /*
                     * Creating event on the Event loop, this will be after the close is done
                     */
                    auto *fw = celix_bundleContext_getFramework(tracker->cCtx.get());
                    auto *bnd = celix_bundleContext_getBundle(tracker->cCtx.get());
                    long bndId = celix_bundle_getId(bnd);
                    celix_framework_fireGenericEvent(
                            fw,
                            -1,
                            bndId,
                            "celix::AbstractTracker delete callback",
                            tracker,
                            [](void *data) {
                                auto *t = static_cast<AbstractTracker *>(data);
                                delete t;
                            },
                            nullptr,
                            nullptr);
                }
            };
        }

        const std::shared_ptr<celix_bundle_context_t> cCtx;

        mutable std::mutex mutex{}; //protects below
        long trkId{-1L};
        TrackerState state{TrackerState::CLOSED};
    };

    /**
     * @brief he GenericServiceTracker class is a specialization of the AbstractTracker
     * for managing a service tracker.
     *
     * It defines how service trackers are opened ands manages some shared service tracker
     * fields.
     *
     * @note Thread safe.
     */
    class GenericServiceTracker : public AbstractTracker {
    public:
        GenericServiceTracker(std::shared_ptr<celix_bundle_context_t> _cCtx, std::string _svcName,
                              std::string _svcVersionRange, celix::Filter _filter) : AbstractTracker{std::move(_cCtx)}, svcName{std::move(_svcName)},
                                                                                   svcVersionRange{std::move(_svcVersionRange)}, filter{std::move(_filter)} {
            setupServiceTrackerOptions();
        }

        ~GenericServiceTracker() override = default;

        /**
         * @see celix::AbstractTracker::open
         */
        void open() override {
            std::lock_guard<std::mutex> lck{mutex};
            if (state == TrackerState::CLOSED || state == TrackerState::CLOSING) {
                state = TrackerState::OPENING;

                //NOTE assuming the opts already configured the callbacks
                trkId = celix_bundleContext_trackServicesWithOptionsAsync(cCtx.get(), &opts);
                if (trkId < 0) {
                    throw celix::TrackerException{"Cannot open service tracker"};
                }
            }
        }

        /**
         * @brief The service name tracked by this service tracker.
         */
        const std::string& getServiceName() const { return svcName; }

        /**
         * @brief The service version range tracked by this service tracker.
         */
        const std::string& getServiceRange() const { return svcVersionRange; }

        /**
         * @brief The additional filter for services tracked by this service tracker.
         *
         * This filter is additional to the service name and optional service
         * version range.
         */
        const celix::Filter& getFilter() const { return filter; }

        /**
         * @brief The nr of services currently tracked by this tracker.
         */
        std::size_t getServiceCount() const {
            return svcCount;
        }
    protected:
        const std::string svcName;
        const std::string svcVersionRange;
        const celix::Filter filter;
        celix_service_tracking_options opts{}; //note only set in the ctor
        std::atomic<size_t> svcCount{0};

    private:
        void setupServiceTrackerOptions() {
            opts.trackerCreatedCallbackData = this;
            opts.trackerCreatedCallback = [](void *data) {
                auto* trk = static_cast<GenericServiceTracker*>(data);
                {
                    std::lock_guard<std::mutex> callbackLock{trk->mutex};
                    trk->state = TrackerState::OPEN;
                }
            };
        }
    };

    /**
     * @brief The ServiceTracker class tracks services
     *
     * Tracking in this case means that the ServiceTracker maintains and informs
     * - through the use of callbacks - a set services matching
     * the service tracking criteria (matches the service name, fits int the optional service
     * version range and matches with the LDAP filter).
     *
     *
     * @note Thread safe.
     * \tparam I The service type to track
     */
    template<typename I>
    class ServiceTracker : public GenericServiceTracker {
    public:
        /**
         * @brief Creates a new service tracker and opens the tracker.
         * @param cCtx The c bundle context.
         * @param svcName The service name to filter for.
         * @param svcVersionRange The optional service range to filter for.
         * @param filter The optional (and additional with respect to svcName and svcVersionRange) LDAP filter to filter for.
         * @param setCallbacks The callback which is called when a new service needs te be set which matches the trackers filter.
         * @param addCallbacks The callback which is called when a new service is added to the Celix framework which matches the trackers filter.
         * @param remCallbacks The callback which is called when a service is removed from the Celix framework which matches the trackers filter.
         * @return The new service tracker as shared ptr.
         * @throws celix::Exception
         */
        static std::shared_ptr<ServiceTracker<I>> create(
                std::shared_ptr<celix_bundle_context_t> cCtx,
                std::string svcName,
                std::string svcVersionRange,
                celix::Filter filter,
                std::vector<std::function<void(const std::shared_ptr<I>&, const std::shared_ptr<const celix::Properties>&, const std::shared_ptr<const celix::Bundle>&)>> setCallbacks,
                std::vector<std::function<void(const std::shared_ptr<I>&, const std::shared_ptr<const celix::Properties>&, const std::shared_ptr<const celix::Bundle>&)>> addCallbacks,
                std::vector<std::function<void(const std::shared_ptr<I>&, const std::shared_ptr<const celix::Properties>&, const std::shared_ptr<const celix::Bundle>&)>> remCallbacks) {
            auto tracker = std::shared_ptr<ServiceTracker<I>>{
                new ServiceTracker<I>{
                    std::move(cCtx),
                    std::move(svcName),
                    std::move(svcVersionRange),
                    std::move(filter),
                    std::move(setCallbacks),
                    std::move(addCallbacks),
                    std::move(remCallbacks)},
                AbstractTracker::delCallback<ServiceTracker<I>>()};
            tracker->open();
            return tracker;
        }

        /**
         * @brief Get the current highest ranking service tracked by this tracker.
         *
         * Note that this can be a nullptr if there are no services found.
         *
         * The return shared ptr should not be stored and only be used shortly, otherwise the
         * framework can hangs during service un-registrations.
         */
        std::shared_ptr<I> getHighestRankingService() {
            waitIfAble();
            std::shared_ptr<I> result{};
            std::lock_guard<std::mutex> lck{mutex};
            auto it = entries.begin();
            if (it != entries.end()) {
                result = (*it)->svc;
            }
            return result;
        }

        /**
         * @brief Get a vector of all the currently found services for this tracker.
         *
         * This vector is ordered by service ranking (descending, highest ranking service first).
         *
         * The returned result not be stored and only be used shortly, otherwise the
         * framework can hangs during service un-registrations.
         */
        std::vector<std::shared_ptr<I>> getServices() {
            waitIfAble();
            std::vector<std::shared_ptr<I>> result{};
            std::lock_guard<std::mutex> lck{mutex};
            result.reserve(entries.size());
            for (auto& e : entries) {
                result.push_back(e->svc);
            }
            return result;
        }

        /**
         * @brief Applies the provided function to each service being tracked.
         *
         * @tparam F A function or callable object type. The function signature should be equivalent to the following:
         *           `void func(I& svc)`
         *           where I is the service type being tracked.
         * @param f The function or callable object to apply to each service.
         * @return The number of services to which the function was applied.
         */
        template<typename F>
        size_t useServices(const F& f) {
            return this->useServicesInternal(
                [&f](I& svc, const celix::Properties&, const celix::Bundle&) { f(svc); });
        }

        /**
         * @brief Applies the provided function to each service being tracked, along with its properties.
         *
         * @tparam F A function or callable object type. The function signature should be equivalent to the following:
         *           `void func(I& svc, const celix::Properties& props)`
         *           where I is the service type being tracked.
         * @param f The function or callable object to apply to each service.
         * @return The number of services to which the function was applied.
         */
        template<typename F>
        size_t useServicesWithProperties(const F& f) {
            return this->useServicesInternal(
                [&f](I& svc, const celix::Properties& props, const celix::Bundle&) { f(svc, props); });
        }

        /**
         * @brief Applies the provided function to each service being tracked, along with its properties and owner
         * bundle.
         *
         * @tparam F A function or callable object type. The function signature should be equivalent to the following:
         *           `void func(I& svc, const celix::Properties& props, const celix::Bundle& bnd)`
         *           where I is the service type being tracked.
         * @param f The function or callable object to apply to each service.
         * @return The number of services to which the function was applied.
         */
        template<typename F>
        size_t useServicesWithOwner(const F& f) {
            return this->useServicesInternal(
                [&f](I& svc, const celix::Properties& props, const celix::Bundle& bnd) { f(svc, props, bnd); });
        }

        /**
         * @brief Applies the provided function to the highest ranking service being tracked.
         *
         * @tparam F A function or callable object type. The function signature should be equivalent to the following:
         *           `void func(I& svc)`
         *           where I is the service type being tracked.
         * @param f The function or callable object to apply to the highest ranking service.
         * @return True if the function was applied to a service, false otherwise.
         */
        template<typename F>
        bool useService(const F& f) {
            return this->useServiceInternal(
                [&f](I& svc, const celix::Properties&, const celix::Bundle&) { f(svc); });
        }

        /**
         * @brief Applies the provided function to the highest ranking service being tracked, along with its properties.
         *
         * @tparam F A function or callable object type. The function signature should be equivalent to the following:
         *           `void func(I& svc, const celix::Properties& props)`
         *           where I is the service type being tracked.
         * @param f The function or callable object to apply to the highest ranking service.
         * @return True if the function was applied to a service, false otherwise.
         */
        template<typename F>
        bool useServiceWithProperties(const F& f) {
            return this->useServiceInternal(
                [&f](I& svc, const celix::Properties& props, const celix::Bundle&) { f(svc, props); });
        }

        /**
         * @brief Applies the provided function to the highest ranking service being tracked, along with its properties
         * and owner bundle.
         *
         * @tparam F A function or callable object type. The function signature should be equivalent to the following:
         *           `void func(I& svc, const celix::Properties& props, const celix::Bundle& bnd)`
         *           where I is the service type being tracked.
         * @param f The function or callable object to apply to the highest ranking service.
         * @return True if the function was applied to a service, false otherwise.
         */
        template<typename F>
        bool useServiceWithOwner(const F& f) {
            return this->useServiceInternal(
                [&f](I& svc, const celix::Properties& props, const celix::Bundle& bnd) { f(svc, props, bnd); });
        }
    protected:
        struct SvcEntry {
            SvcEntry(long _svcId, long _svcRanking, std::shared_ptr<I> _svc,
                     std::shared_ptr<const celix::Properties> _properties,
                     std::shared_ptr<const celix::Bundle> _owner) : svcId(_svcId), svcRanking(_svcRanking),
                                                                          svc(std::move(_svc)),
                                                                          properties(std::move(_properties)),
                                                                          owner(std::move(_owner)) {}
            long svcId;
            long svcRanking;
            std::shared_ptr<I> svc;
            std::shared_ptr<const celix::Properties> properties;
            std::shared_ptr<const celix::Bundle> owner;
        };


        ServiceTracker(std::shared_ptr<celix_bundle_context_t> _cCtx, std::string _svcName,
                       std::string _svcVersionRange, celix::Filter _filter,
                       std::vector<std::function<void(const std::shared_ptr<I>&, const std::shared_ptr<const celix::Properties>&, const std::shared_ptr<const celix::Bundle>&)>> _setCallbacks,
                       std::vector<std::function<void(const std::shared_ptr<I>&, const std::shared_ptr<const celix::Properties>&, const std::shared_ptr<const celix::Bundle>&)>> _addCallbacks,
                       std::vector<std::function<void(const std::shared_ptr<I>&, const std::shared_ptr<const celix::Properties>&, const std::shared_ptr<const celix::Bundle>&)>> _remCallbacks) :
                GenericServiceTracker{std::move(_cCtx), std::move(_svcName), std::move(_svcVersionRange), std::move(_filter)},
                setCallbacks{std::move(_setCallbacks)},
                addCallbacks{std::move(_addCallbacks)},
                remCallbacks{std::move(_remCallbacks)} {
            setupServiceTrackerOptions();
        }

        static std::shared_ptr<SvcEntry> createEntry(void* voidSvc, const celix_properties_t* cProps, const celix_bundle_t* cBnd) {
            long svcId = celix_properties_getAsLong(cProps, CELIX_FRAMEWORK_SERVICE_ID, -1L);
            long svcRanking = celix_properties_getAsLong(cProps, CELIX_FRAMEWORK_SERVICE_RANKING, 0);
            auto svc = std::shared_ptr<I>{static_cast<I*>(voidSvc), [](I*){/*nop*/}};
            auto props = std::make_shared<const celix::Properties>(celix::Properties::wrap(cProps));
            auto owner = std::make_shared<celix::Bundle>(const_cast<celix_bundle_t*>(cBnd));
            return std::make_shared<SvcEntry>(svcId, svcRanking, svc, props, owner);
        }

        void waitForExpiredSvcEntry(std::shared_ptr<SvcEntry>& entry) {
            if (entry) {
                std::weak_ptr<void> svcObserve = entry->svc;
                std::weak_ptr<const celix::Properties> propsObserve = entry->properties;
                std::weak_ptr<const celix::Bundle> ownerObserve = entry->owner;
                entry->svc = nullptr;
                entry->properties = nullptr;
                entry->owner = nullptr;
                waitForExpired(svcObserve, entry->svcId, "service");
                waitForExpired(propsObserve, entry->svcId, "service properties");
                waitForExpired(ownerObserve, entry->svcId, "service bundle (owner)");
            }
        }

        template<typename U>
        void waitForExpired(std::weak_ptr<U> observe, long svcId, const char* objName) {
            auto start = std::chrono::steady_clock::now();
            while (!observe.expired()) {
                auto now = std::chrono::steady_clock::now();
                auto durationInMilli = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
                if (durationInMilli > warningTimoutForNonExpiredSvcObject) {
                    celix_bundleContext_log(cCtx.get(), CELIX_LOG_LEVEL_WARNING, "Cannot remove %s associated with service.id %li, because it is still in use. Current shared_ptr use count is %i\n", objName, svcId, (int)observe.use_count());
                    start = now;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds{50});
            }
        }

        void invokeUpdateCallbacks() {
            if (!updateCallbacks.empty()) {
                std::vector<std::shared_ptr<I>> updateVector{};
                {
                    std::lock_guard<std::mutex> lck{mutex};
                    updateVector.reserve(entries.size());
                    for (const auto &entry :  entries) {
                        updateVector.push_back(entry->svc);
                    }
                }
                for (const auto& cb : updateCallbacks) {
                    cb(updateVector);
                }
            }
            if (!updateWithPropertiesCallbacks.empty()) {
                std::vector<std::pair<std::shared_ptr<I>, std::shared_ptr<const celix::Properties>>> updateVector{};
                updateVector.reserve(entries.size());
                {
                    std::lock_guard<std::mutex> lck{mutex};
                    for (const auto &entry :  entries) {
                        updateVector.emplace_back(entry->svc, entry->properties);
                    }
                }
                for (const auto& cb : updateWithPropertiesCallbacks) {
                    cb(updateVector);
                }
            }
            if (!updateWithOwnerCallbacks.empty()) {
                std::vector<std::tuple<std::shared_ptr<I>, std::shared_ptr<const celix::Properties>, std::shared_ptr<const celix::Bundle>>> updateVector{};
                updateVector.reserve(entries.size());
                {
                    std::lock_guard<std::mutex> lck{mutex};
                    for (const auto &entry :  entries) {
                        updateVector.emplace_back(entry->svc, entry->properties, entry->owner);
                    }
                }
                for (const auto& cb : updateWithOwnerCallbacks) {
                    cb(updateVector);
                }
            }
        }

        const std::chrono::milliseconds warningTimoutForNonExpiredSvcObject{1000};

        const std::vector<std::function<void(const std::shared_ptr<I>&, const std::shared_ptr<const celix::Properties>&, const std::shared_ptr<const celix::Bundle>&)>> setCallbacks;
        const std::vector<std::function<void(const std::shared_ptr<I>&, const std::shared_ptr<const celix::Properties>&, const std::shared_ptr<const celix::Bundle>&)>> addCallbacks;
        const std::vector<std::function<void(const std::shared_ptr<I>&, const std::shared_ptr<const celix::Properties>&, const std::shared_ptr<const celix::Bundle>&)>> remCallbacks;

        //NOTE update callbacks cannot yet be configured
        const std::vector<std::function<void(const std::vector<std::shared_ptr<I>>)>> updateCallbacks{};
        const std::vector<std::function<void(const std::vector<std::pair<std::shared_ptr<I>, std::shared_ptr<const celix::Properties>>>)>> updateWithPropertiesCallbacks{};
        const std::vector<std::function<void(const std::vector<std::tuple<std::shared_ptr<I>, std::shared_ptr<const celix::Properties>, std::shared_ptr<const celix::Bundle>>>)>> updateWithOwnerCallbacks{};

        struct SvcEntryCompare {
            bool operator() (const std::shared_ptr<SvcEntry>& a, const std::shared_ptr<SvcEntry>& b) const {
                int cmp = celix_utils_compareServiceIdsAndRanking(a->svcId, a->svcRanking, b->svcId, b->svcRanking);
                return cmp < 0;
            }
        };

        mutable std::mutex mutex{}; //protect below
        std::set<std::shared_ptr<SvcEntry>, SvcEntryCompare> entries{};
        std::unordered_map<long, std::shared_ptr<SvcEntry>> cachedEntries{};
        std::shared_ptr<SvcEntry> highestRankingServiceEntry{};

    private:
        void setupServiceTrackerOptions() {
            opts.filter.serviceName = svcName.empty() ? nullptr : svcName.c_str();
            opts.filter.versionRange = svcVersionRange.empty() ? nullptr : svcVersionRange.c_str();
            opts.filter.filter = filter.empty() ? nullptr : filter.getFilterCString();
            opts.callbackHandle = this;
            opts.addWithOwner = [](void *handle, void *voidSvc, const celix_properties_t* cProps, const celix_bundle_t* cBnd) {
                auto tracker = static_cast<ServiceTracker<I>*>(handle);
                auto entry = createEntry(voidSvc, cProps, cBnd);
                {
                    std::lock_guard<std::mutex> lck{tracker->mutex};
                    tracker->entries.insert(entry);
                    tracker->cachedEntries[entry->svcId] = entry;
                }
                tracker->svcCount.fetch_add(1, std::memory_order_relaxed);
                for (const auto& cb : tracker->addCallbacks) {
                    cb(entry->svc, entry->properties, entry->owner);
                }
                tracker->invokeUpdateCallbacks();
            };
            opts.removeWithOwner = [](void *handle, void*, const celix_properties_t* cProps, const celix_bundle_t*) {
                auto tracker = static_cast<ServiceTracker<I>*>(handle);
                long svcId = celix_properties_getAsLong(cProps, CELIX_FRAMEWORK_SERVICE_ID, -1L);
                std::shared_ptr<SvcEntry> entry{};
                {
                    std::lock_guard<std::mutex> lck{tracker->mutex};
                    auto it = tracker->cachedEntries.find(svcId);
                    assert(it != tracker->cachedEntries.end()); //should not happen, added during add callback
                    entry = it->second;
                    tracker->cachedEntries.erase(it);
                    tracker->entries.erase(entry);
                }
                for (const auto& cb : tracker->remCallbacks) {
                    cb(entry->svc, entry->properties, entry->owner);
                }
                tracker->invokeUpdateCallbacks();
                tracker->svcCount.fetch_sub(1, std::memory_order_relaxed);
                tracker->waitForExpiredSvcEntry(entry);
            };
            opts.setWithOwner = [](void *handle, void *voidSvc, const celix_properties_t *cProps, const celix_bundle_t *cBnd) {
                auto tracker = static_cast<ServiceTracker<I>*>(handle);
                std::unique_lock<std::mutex> lck{tracker->mutex};
                auto prevEntry = tracker->highestRankingServiceEntry;
                if (voidSvc) {
                    tracker->highestRankingServiceEntry = createEntry(voidSvc, cProps, cBnd);
                } else {
                    tracker->highestRankingServiceEntry = nullptr;
                }
                for (const auto& cb : tracker->setCallbacks) {
                    if (tracker->highestRankingServiceEntry) {
                        auto& e = tracker->highestRankingServiceEntry;
                        cb(e->svc, e->properties, e->owner);
                    } else /*"unset"*/ {
                        cb(nullptr, nullptr, nullptr);
                    }
                }
                lck.unlock();
                tracker->waitForExpiredSvcEntry(prevEntry);
            };
        }

        template<typename F>
        size_t useServicesInternal(const F& f) {
            size_t count = 0;
            std::lock_guard<std::mutex> lck{mutex};
            for (auto& e : entries) {
                I& svc = *e->svc;
                const celix::Properties& props = *e->properties;
                const celix::Bundle& owner = *e->owner;
                f(svc, props, owner);
                count++;
            }
            return count;
        }

        template<typename F>
        bool useServiceInternal(const F& f) {
            std::lock_guard<std::mutex> lck{mutex};
            if (highestRankingServiceEntry) {
                I& svc = *highestRankingServiceEntry->svc;
                const celix::Properties& props = *highestRankingServiceEntry->properties;
                const celix::Bundle& owner = *highestRankingServiceEntry->owner;
                f(svc, props, owner);
                return true;
            }
            return false;
        }
    };

    /**
     * @brief The BundleTracker class tracks bundles
     * @note Thread safe.
     */
    class BundleTracker : public AbstractTracker {
    public:
        /**
         * @brief Creates a new bundle tracker and opens the tracker.
         *
         * @param cCtx The c bundle context.
         * @param includeFrameworkBundle  Whether the Celix framework bundle should be included in the callbacks.
         * @param onInstallCallbacks The callback which is called for every bundle being installed (also called retroactively for already installed bundles).
         * @param onStartCallbacks The callback which is called for every bundle being started (also called retroactively for already started bundles).
         * @param onStopCallbacks The callback which is called for every bundle being stopped (also called retroactively for already stopped bundles).
         * @return The new bundle tracker as shared ptr.
         * @throws celix::Exception.
         */
        static std::shared_ptr<BundleTracker> create(
                std::shared_ptr<celix_bundle_context_t> cCtx,
                bool includeFrameworkBundle,
                std::vector<std::function<void(const celix::Bundle&)>> onInstallCallbacks,
                std::vector<std::function<void(const celix::Bundle&)>> onStartCallbacks,
                std::vector<std::function<void(const celix::Bundle&)>> onStopCallbacks) {

            auto tracker = std::shared_ptr<BundleTracker>{
                    new BundleTracker{
                            std::move(cCtx),
                            includeFrameworkBundle,
                            std::move(onInstallCallbacks),
                            std::move(onStartCallbacks),
                            std::move(onStopCallbacks)},
                    AbstractTracker::delCallback<BundleTracker>()};
            tracker->open();
            return tracker;
        }

        /**
         * @see AbstractTracker::open
         */
        void open() override {
            std::lock_guard<std::mutex> lck{mutex};
            if (state == TrackerState::CLOSED || state == TrackerState::CLOSING) {
                state = TrackerState::OPENING;
                //NOTE the opts already configured the callbacks
                trkId = celix_bundleContext_trackBundlesWithOptionsAsync(cCtx.get(), &opts);
                if (trkId < 0) {
                    throw celix::TrackerException{"Cannot open bundle tracker"};
                }
            }
        }
    private:
        BundleTracker(
                std::shared_ptr<celix_bundle_context_t> _cCtx,
                bool _includeFrameworkBundle,
                std::vector<std::function<void(const celix::Bundle&)>> _onInstallCallbacks,
                std::vector<std::function<void(const celix::Bundle&)>> _onStartCallbacks,
                std::vector<std::function<void(const celix::Bundle&)>> _onStopCallbacks) :
                AbstractTracker{std::move(_cCtx)},
                includeFrameworkBundle{_includeFrameworkBundle},
                onInstallCallbacks{std::move(_onInstallCallbacks)},
                onStartCallbacks{std::move(_onStartCallbacks)},
                onStopCallbacks{std::move(_onStopCallbacks)} {


            opts.includeFrameworkBundle = includeFrameworkBundle;
            opts.callbackHandle = this;
            opts.onInstalled = [](void *handle, const celix_bundle_t *cBnd) {
                auto tracker = static_cast<BundleTracker *>(handle);
                celix::Bundle bnd{const_cast<celix_bundle_t *>(cBnd)};
                for (const auto& cb : tracker->onInstallCallbacks) {
                    cb(bnd);
                }
            };
            opts.onStarted = [](void *handle, const celix_bundle_t *cBnd) {
                auto tracker = static_cast<BundleTracker *>(handle);
                celix::Bundle bnd{const_cast<celix_bundle_t *>(cBnd)};
                for (const auto& cb : tracker->onStartCallbacks) {
                    cb(bnd);
                }
            };
            opts.onStopped = [](void *handle, const celix_bundle_t *cBnd) {
                auto tracker = static_cast<BundleTracker *>(handle);
                celix::Bundle bnd{const_cast<celix_bundle_t *>(cBnd)};
                for (const auto& cb : tracker->onStopCallbacks) {
                    cb(bnd);
                }
            };
            opts.trackerCreatedCallbackData = this;
            opts.trackerCreatedCallback = [](void *data) {
                auto* trk = static_cast<BundleTracker*>(data);
                std::lock_guard<std::mutex> callbackLock{trk->mutex};
                trk->state = TrackerState::OPEN;
            };
        }

        const bool includeFrameworkBundle;
        const std::vector<std::function<void(const celix::Bundle&)>> onInstallCallbacks;
        const std::vector<std::function<void(const celix::Bundle&)>> onStartCallbacks;
        const std::vector<std::function<void(const celix::Bundle&)>> onStopCallbacks;
        celix_bundle_tracking_options_t opts{}; //note only set in the ctor
    };


    /**
     * @brief A trivial struct containing information about a service tracker.
     */
    struct ServiceTrackerInfo {
        /**
         * @brief The service name the service tracker is tracking.
         *
         * Will be '*' if the service tracker is tracking any services.
         */
        const std::string serviceName;

        /**
         * @brief The service filter the service tracker is using for tracking.
         */
        const celix::Filter filter;

        /**
         * @brief The bundle id of the owner of the service tracker.
         */
        const long trackerOwnerBundleId;
    };

    /**
     * @brief The MetaTracker track service trackers.
     * @throws celix::Exception
     * @note Thread safe.
     */
    class MetaTracker : public AbstractTracker {
    public:

        /**
         * @brief Creates a new meta tracker and opens the tracker.
         *
         * @param cCtx The c bundle context.
         * @param serviceName The service name used in the service tracker to track for.
         * @param onTrackerCreated The callback which will be called when the tracker is created.
         * @param onTrackerDestroyed The callback which will be called when the tracker is destroyed.
         * @return The new meta tracker as shared ptr.
         * @throws celix::Exception.
         */

        static std::shared_ptr<MetaTracker> create(
                std::shared_ptr<celix_bundle_context_t> cCtx,
                std::string serviceName,
                std::vector<std::function<void(const ServiceTrackerInfo&)>> onTrackerCreated,
                std::vector<std::function<void(const ServiceTrackerInfo&)>> onTrackerDestroyed) {
            auto tracker = std::shared_ptr<MetaTracker>{
                    new MetaTracker{
                            std::move(cCtx),
                            std::move(serviceName),
                            std::move(onTrackerCreated),
                            std::move(onTrackerDestroyed)},
                    AbstractTracker::delCallback<MetaTracker>()};
            tracker->open();
            return tracker;
        }

        /**
         * @see AbstractTracker::open
         */
        void open() override {
            std::lock_guard<std::mutex> lck{mutex};
            if (state == TrackerState::CLOSED || state == TrackerState::CLOSING) {
                state = TrackerState::OPENING;

                //NOTE the opts already configured the callbacks
                trkId = celix_bundleContext_trackServiceTrackersAsync(
                        cCtx.get(),
                        serviceName.empty() ? nullptr : serviceName.c_str(),
                        static_cast<void*>(this),
                        [](void *handle, const celix_service_tracker_info_t *cInfo) {
                            auto *trk = static_cast<MetaTracker *>(handle);
                            std::string svcName = cInfo->serviceName == nullptr ? "" : cInfo->serviceName;
                            ServiceTrackerInfo info{svcName, celix::Filter::wrap(cInfo->filter), cInfo->bundleId};
                            for (const auto& cb : trk->onTrackerCreated) {
                                cb(info);
                            }
                        },
                        [](void *handle, const celix_service_tracker_info_t *cInfo) {
                            auto *trk = static_cast<MetaTracker *>(handle);
                            std::string svcName = cInfo->serviceName == nullptr ? "" : cInfo->serviceName;
                            ServiceTrackerInfo info{svcName, celix::Filter::wrap(cInfo->filter), cInfo->bundleId};
                            for (const auto& cb : trk->onTrackerDestroyed) {
                                cb(info);
                            }
                        },
                        static_cast<void*>(this),
                        [](void *data) {
                            auto *trk = static_cast<MetaTracker *>(data);
                            std::lock_guard<std::mutex> callbackLock{trk->mutex};
                            trk->state = TrackerState::OPEN;
                        });
                if (trkId < 0) {
                    throw celix::TrackerException{"Cannot open meta tracker"};
                }
            }
        }

    private:
        MetaTracker(
                std::shared_ptr<celix_bundle_context_t> _cCtx,
                std::string _serviceName,
                std::vector<std::function<void(const ServiceTrackerInfo&)>> _onTrackerCreated,
                std::vector<std::function<void(const ServiceTrackerInfo&)>> _onTrackerDestroyed) :
                AbstractTracker{std::move(_cCtx)},
                serviceName{std::move(_serviceName)},
                onTrackerCreated{std::move(_onTrackerCreated)},
                onTrackerDestroyed{std::move(_onTrackerDestroyed)} {}

        const std::string serviceName;
        const std::vector<std::function<void(const ServiceTrackerInfo&)>> onTrackerCreated;
        const std::vector<std::function<void(const ServiceTrackerInfo&)>> onTrackerDestroyed;
    };

}
