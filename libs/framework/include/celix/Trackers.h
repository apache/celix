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

#include <memory>
#include <mutex>
#include <atomic>
#include <cassert>

#include "celix/Properties.h"
#include "celix/Utils.h"
#include "celix/Bundle.h"
#include "celix/Constants.h"
#include "celix/Filter.h"
#include "celix_bundle_context.h"

namespace celix {

    /**
     * TODO
     * GenericTracker <-----------------------------------
     *  ^                               |                |
     *  |                               |                |
     *  GenericServiceTracker      BundleTracker    MetaTracker
     *  ^
     *  |
     *  ServiceTracker<I>
     */

    enum class TrackerState {
        OPENING,
        OPEN,
        CLOSING,
        CLOSED
    };

    /**
     * TODO
     * \note Thread safe.
     */
    class AbstractTracker {
    public:
        explicit AbstractTracker(std::shared_ptr<celix_bundle_context_t> _cCtx) : cCtx{std::move(_cCtx)} {}

        virtual ~AbstractTracker() noexcept {
            //NOTE that the tracker needs to be closed in the specialization (when all fields are still valid)
            assert(getCurrentState() == TrackerState::CLOSED);
        }

        bool isOpen() const {
            std::lock_guard<std::mutex> lck{mutex};
            return state == TrackerState::OPEN;
        }

        TrackerState getCurrentState() const {
            std::lock_guard<std::mutex> lck{mutex};
            return state;
        }

        void close() {
            std::lock_guard<std::mutex> lck{mutex};
            if (state == TrackerState::OPEN) {
                //not yet closed
                state = TrackerState::CLOSING;

                celix_bundleContext_stopTrackerAsync(cCtx.get(), trkId, this, [](void *data) {
                    auto trk = static_cast<AbstractTracker*>(data);
                    std::lock_guard<std::mutex> lck{trk->mutex};
                    trk->state = TrackerState::CLOSED;
                    trk->trkId = -1L;
                });
            }
        }

        //note needs override, open is different per tracker type
        virtual void open() {}

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
        template<typename T>
        static std::function<void(T*)> delCallback() {
            return [](T *tracker) {
                if (tracker->getCurrentState() == TrackerState::CLOSED) {
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
     * TODO
     * \note Thread safe.
     */
    class GenericServiceTracker : public AbstractTracker {
    public:
        GenericServiceTracker(std::shared_ptr<celix_bundle_context_t> _cCtx, std::string _svcName,
                              std::string _svcVersionRange, celix::Filter _filter) : AbstractTracker{std::move(_cCtx)}, svcName{std::move(_svcName)},
                                                                                   svcVersionRange{std::move(_svcVersionRange)}, filter{std::move(_filter)} {
            opts.trackerCreatedCallbackData = this;
            opts.trackerCreatedCallback = [](void *data) {
                auto* trk = static_cast<GenericServiceTracker*>(data);
                std::lock_guard<std::mutex> callbackLock{trk->mutex};
                trk->state = TrackerState::OPEN;
            };
        }

        ~GenericServiceTracker() override = default;

        void open() override {
            std::lock_guard<std::mutex> lck{mutex};
            if (state == TrackerState::CLOSED) {
                state = TrackerState::OPENING;

                //NOTE assuming the opts already configured the callbacks
                trkId = celix_bundleContext_trackServicesWithOptionsAsync(cCtx.get(), &opts);
            }
        }

        const std::string& getServiceName() const { return svcName; }

        const std::string& getServiceRange() const { return svcVersionRange; }

        const celix::Filter& getFilter() const { return filter; }

        std::size_t getServiceCount() const {
            return svcCount;
        }
    protected:
        const std::string svcName;
        const std::string svcVersionRange;
        const celix::Filter filter;
        celix_service_tracking_options opts{}; //note only set in the ctor
        std::atomic<size_t> svcCount{0};
    };

    /**
     * \note Thread safe.
     * @tparam I The service type to track
     */
    template<typename I>
    class ServiceTracker : public GenericServiceTracker {
    public:
        static std::shared_ptr<ServiceTracker<I>> create(
                std::shared_ptr<celix_bundle_context_t> cCtx,
                std::string svcName,
                std::string svcVersionRange,
                celix::Filter filter,
                std::vector<std::function<void(std::shared_ptr<I>, std::shared_ptr<const celix::Properties>, std::shared_ptr<const celix::Bundle>)>> setCallbacks,
                std::vector<std::function<void(std::shared_ptr<I>, std::shared_ptr<const celix::Properties>, const celix::Bundle&)>> addCallbacks,
                std::vector<std::function<void(std::shared_ptr<I>, std::shared_ptr<const celix::Properties>, const celix::Bundle&)>> remCallbacks) {

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
    private:
        ServiceTracker(std::shared_ptr<celix_bundle_context_t> _cCtx, std::string _svcName,
                       std::string _svcVersionRange, celix::Filter _filter,
                       std::vector<std::function<void(std::shared_ptr<I>, std::shared_ptr<const celix::Properties>, std::shared_ptr<const celix::Bundle>)>> _setCallbacks,
                       std::vector<std::function<void(std::shared_ptr<I>, std::shared_ptr<const celix::Properties>, const celix::Bundle&)>> _addCallbacks,
                       std::vector<std::function<void(std::shared_ptr<I>, std::shared_ptr<const celix::Properties>, const celix::Bundle&)>> _remCallbacks) :
                GenericServiceTracker{std::move(_cCtx), std::move(_svcName), std::move(_svcVersionRange), std::move(_filter)},
                setCallbacks{std::move(_setCallbacks)},
                addCallbacks{std::move(_addCallbacks)},
                remCallbacks{std::move(_remCallbacks)} {

            opts.filter.serviceName = svcName.empty() ? nullptr : svcName.c_str();
            opts.filter.versionRange = svcVersionRange.empty() ? nullptr : svcVersionRange.c_str();
            opts.filter.filter = filter.empty() ? nullptr : filter.getFilterCString();
            opts.callbackHandle = this;
            opts.addWithOwner = [](void *handle, void *voidSvc, const celix_properties_t* cProps, const celix_bundle_t* cBnd) {
                auto tracker = static_cast<ServiceTracker<I>*>(handle);
                long svcId = celix_properties_getAsLong(cProps, OSGI_FRAMEWORK_SERVICE_ID, -1L);
                auto svc = tracker->getServiceForId(svcId, voidSvc);
                auto props = tracker->getPropertiesForId(svcId, cProps);
                //TODO auto bnd = tracker->getBundleForId(svcId, cBnd);
                auto bnd = celix::Bundle{const_cast<celix_bundle_t*>(cBnd)};
                for (const auto& func : tracker->addCallbacks) {
                    func(svc, props, bnd);
                }
                tracker->svcCount.fetch_add(1, std::memory_order_relaxed);
            };
            opts.removeWithOwner = [](void *handle, void *voidSvc, const celix_properties_t* cProps, const celix_bundle_t* cBnd) {
                auto tracker = static_cast<ServiceTracker<I>*>(handle);
                long svcId = celix_properties_getAsLong(cProps, OSGI_FRAMEWORK_SERVICE_ID, -1L);
                auto svc = tracker->getServiceForId(svcId, voidSvc);
                auto props = tracker->getPropertiesForId(svcId, cProps);
                //TODO auto bnd = tracker->getBundleForId(svcId, cBnd);
                auto bnd = celix::Bundle{const_cast<celix_bundle_t*>(cBnd)};
                for (const auto& func : tracker->remCallbacks) {
                    func(svc, props, bnd);
                }
                tracker->svcCount.fetch_sub(1, std::memory_order_relaxed);

                tracker->removeAddRemEntriesForId(svcId);
                tracker->waitForExpire(std::move(svc), svcId, "Service");
                tracker->waitForExpire(std::move(props), svcId, "celix::Properties");
            };
            opts.setWithOwner = [](void *handle, void *voidSvc, const celix_properties_t* cProps, const celix_bundle_t* cBnd) {
                auto tracker = static_cast<ServiceTracker<I>*>(handle);
                std::shared_ptr<I> svc{};
                std::shared_ptr<const celix::Properties> props{};
                std::shared_ptr<const celix::Bundle> bnd{};
                if (voidSvc != nullptr) {
                    svc = std::shared_ptr<I>{static_cast<I*>(voidSvc), [](I*){/*nop*/}};
                }
                if (cProps != nullptr) {
                    props = celix::Properties::wrap(cProps);
                }
                if (cBnd != nullptr) {
                    bnd = std::make_shared<celix::Bundle>(const_cast<celix_bundle_t*>(cBnd));
                }

                long prevSvcId = -1L;
                std::shared_ptr<I> prevSvc{};
                std::shared_ptr<const celix::Properties> prevProps{};
                std::shared_ptr<const celix::Bundle> prevBnd{};
                {
                    std::lock_guard<std::mutex> lck{tracker->mutex};
                    if (tracker->currentSetServiceId >= 0) {
                        prevSvcId = tracker->currentSetServiceId;
                        prevSvc = tracker->currentSetService;
                        prevProps = tracker->currentSetProperties;
                        prevBnd = tracker->currentSetBundle;
                    }
                    tracker->currentSetService = svc;
                    tracker->currentSetProperties = props;
                    tracker->currentSetBundle = bnd;
                    tracker->currentSetServiceId = props ? props->getAsLong(celix::SERVICE_ID, -1) : -1L;
                }
                for (const auto& func : tracker->setCallbacks) {
                    func(svc, props, bnd);
                }
                if (prevSvcId >= 0) {
                    tracker->waitForExpire(std::move(prevSvc), prevSvcId, "service");
                    tracker->waitForExpire(std::move(prevProps), prevSvcId, "celix::Properties");
                    tracker->waitForExpire(std::move(prevBnd), prevSvcId, "celix::Bundle");
                }
            };
        }

        std::shared_ptr<I> getServiceForId(long id, void* voidSvc) {
            std::lock_guard<std::mutex> lck{mutex};
            auto it = services.find(id);
            if (it == services.end()) {
                //new
                auto svc = std::shared_ptr<I>{static_cast<I*>(voidSvc), [](I*){/*nop*/}};
                services[id] = svc;
                return svc;
            } else {
                return it->second;
            }
        }

        std::shared_ptr<const celix::Properties> getPropertiesForId(long id, const celix_properties_t * cProps) {
            std::lock_guard<std::mutex> lck{mutex};
            auto it = properties.find(id);
            if (it == properties.end()) {
                //new
                auto props = celix::Properties::wrap(cProps);
                properties[id] = props;
                return props;
            } else {
                return it->second;
            }
        }

        void waitForExpire(std::shared_ptr<const void> ptr, long observeSvcId, const char *objName) {
            //observe.expired() not working
            std::weak_ptr<const void> observe = ptr;
            ptr = nullptr;
            while (!observe.expired()) {
                celix_bundleContext_log(cCtx.get(), CELIX_LOG_LEVEL_WARNING, "Cannot remove %s associated with service.id %li, because it is in use. Current shared_ptr use count is %i\n", objName, observeSvcId, (int)observe.use_count());
                std::this_thread::sleep_for(std::chrono::seconds (1));
            }
        }

        void removeAddRemEntriesForId(long svcId) {
            std::lock_guard<std::mutex> lck{mutex};
            services.erase(svcId);
            properties.erase(svcId);
            //bundles.erase(svcId);
        }

        const std::vector<std::function<void(std::shared_ptr<I>, std::shared_ptr<const celix::Properties>, std::shared_ptr<const celix::Bundle>)>> setCallbacks;
        const std::vector<std::function<void(std::shared_ptr<I>, std::shared_ptr<const celix::Properties>, const celix::Bundle&)>> addCallbacks;
        const std::vector<std::function<void(std::shared_ptr<I>, std::shared_ptr<const celix::Properties>, const celix::Bundle&)>> remCallbacks;

        mutable std::mutex mutex{}; //protect below

        std::unordered_map<long, std::shared_ptr<I>> services{};
        std::unordered_map<long, std::shared_ptr<const celix::Properties>> properties{};
        //std::unordered_map<long, std::shared_ptr<const celix::Bundle>> bundles{}; TODO

        std::shared_ptr<I> currentSetService{};
        std::shared_ptr<const celix::Properties> currentSetProperties{};
        std::shared_ptr<const celix::Bundle> currentSetBundle{};
        long currentSetServiceId{-1L};

//        std::unordered_map<SvcOrder, SvcEntry<I>{}; //TODO for update
    };

    /**
     * \note Thread safe.
     */
    class BundleTracker : public AbstractTracker {
    public:
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

        void open() override {
            std::lock_guard<std::mutex> lck{mutex};
            if (state == TrackerState::CLOSED) {
                state = TrackerState::OPENING;

                //NOTE the opts already configured the callbacks
                trkId = celix_bundleContext_trackBundlesWithOptionsAsync(cCtx.get(), &opts);
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
                auto bnd = celix::Bundle{const_cast<celix_bundle_t *>(cBnd)};
                for (const auto& cb : tracker->onInstallCallbacks) {
                    cb(bnd);
                }
            };
            opts.onStarted = [](void *handle, const celix_bundle_t *cBnd) {
                auto tracker = static_cast<BundleTracker *>(handle);
                auto bnd = celix::Bundle{const_cast<celix_bundle_t *>(cBnd)};
                for (const auto& cb : tracker->onStartCallbacks) {
                    cb(bnd);
                }
            };
            opts.onStopped = [](void *handle, const celix_bundle_t *cBnd) {
                auto tracker = static_cast<BundleTracker *>(handle);
                auto bnd = celix::Bundle{const_cast<celix_bundle_t *>(cBnd)};
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
     * A trivial struct containing information about a service tracker.
     */
    struct ServiceTrackerInfo {
        /**
         * The service name the service tracker is tracking.
         * Will be '*' if the service tracker is tracking any services.
         */
        const std::string serviceName;

        /**
         * The service filter the service tracker is using for tracking.
         */
        const celix::Filter filter;

        /**
         * The bundle id of the owner of the service tracker.
         */
        const long trackerOwnerBundleId;
    };

    /**
     * \note Thread safe.
     */
    class MetaTracker : public AbstractTracker {
    public:
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

        void open() override {
            std::lock_guard<std::mutex> lck{mutex};
            if (state == TrackerState::CLOSED) {
                state = TrackerState::OPENING;

                //NOTE the opts already configured the callbacks
                trkId = celix_bundleContext_trackServiceTrackersAsync(
                        cCtx.get(),
                        serviceName.empty() ? nullptr : serviceName.c_str(),
                        static_cast<void*>(this),
                        [](void *handle, const celix_service_tracker_info_t *cInfo) {
                            auto *trk = static_cast<MetaTracker *>(handle);
                            ServiceTrackerInfo info{cInfo->serviceName, celix::Filter::wrap(cInfo->filter), cInfo->bundleId};
                            for (const auto& cb : trk->onTrackerCreated) {
                                cb(info);
                            }
                        },
                        [](void *handle, const celix_service_tracker_info_t *cInfo) {
                            auto *trk = static_cast<MetaTracker *>(handle);
                            ServiceTrackerInfo info{cInfo->serviceName, celix::Filter::wrap(cInfo->filter), cInfo->bundleId};
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
        const std::vector<std::function<void(const ServiceTrackerInfo&)>> onTrackerCreated; //TODO check if the callback are done on started/stopped or created/destroyed
        const std::vector<std::function<void(const ServiceTrackerInfo&)>> onTrackerDestroyed;
    };

}