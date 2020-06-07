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

#pragma once

#include <memory>
#include <cassert>
#include <map>

#include "celix/IResourceBundle.h"
#include "celix/ServiceRegistry.h"
#include "celix/Filter.h"
#include "ServiceRegistationImpl.h"

namespace celix {
    namespace impl {

        //TODO move impl from header to cc file.
        struct SvcTrackerEntry {
            const long id;
            const std::string svcName;
            const std::shared_ptr<const celix::IResourceBundle> requester;
            const celix::ServiceTrackerOptions<void> opts;
            const celix::Filter filter;

            explicit SvcTrackerEntry(long _id, std::string _svcName, std::shared_ptr<const celix::IResourceBundle> _requester, celix::ServiceTrackerOptions<void> _opts) :
                    id{_id}, svcName{std::move(_svcName)}, requester{std::move(_requester)}, opts{std::move(_opts)}, filter{opts.filter} {}

            SvcTrackerEntry(SvcTrackerEntry &&rhs) = delete;
            SvcTrackerEntry &operator=(SvcTrackerEntry &&rhs) = delete;
            SvcTrackerEntry(const SvcTrackerEntry &rhs) = delete;
            SvcTrackerEntry &operator=(const SvcTrackerEntry &rhs) = delete;
            ~SvcTrackerEntry() = default;

            void clear() {
                std::vector<std::shared_ptr<const celix::impl::SvcEntry>> removeEntries{};
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
                return !svcName.empty();
            }

            bool match(const celix::impl::SvcEntry& entry) const {
                //note should only called for the correct svcName
                assert(svcName == entry.svcName);

                if (filter.isEmpty()) {
                    return true;
                } else {
                    return filter.match(entry.props);
                }
            }

            void addMatch(std::shared_ptr<const celix::impl::SvcEntry> entry) {
                //increase usage so that services cannot be removed while a service tracker is still active

                //new custom deleter which arranges the count & sync for the used services

                entry->incrUsage();

                std::shared_ptr<void> rawSvc = entry->service(*requester);
                //NOTE creating a shared_ptr with a custom deleter, so that the celix::impl::SvcEntry usage is synced with this shared_ptr.
                auto svc = std::shared_ptr<void>{rawSvc.get(), [entry](void *) {
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

            void remMatch(const std::shared_ptr<const celix::impl::SvcEntry> &entry) {
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

                //note sync will be done on the celix::impl::SvcEntry usage, which is controlled by the tracker svc shared ptr
            }

            void callAddRemoveCallbacks(const std::shared_ptr<const celix::impl::SvcEntry>& entry, const std::shared_ptr<void>& svc, bool add) {
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
                std::shared_ptr<const celix::impl::SvcEntry> currentHighestSvcEntry{};
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
                    //TODO move to cc
                    //logger->error("Usage count decrease below 0!");
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
                std::map<std::shared_ptr<const celix::impl::SvcEntry>, std::shared_ptr<void>, celix::impl::SvcEntryLess> entries{};
                std::shared_ptr<void> highest{};
            } tracked{};


            mutable std::mutex mutex{};
            mutable std::condition_variable cond{};
            mutable int usage{1};
        };

        celix::ServiceTracker::Impl* createServiceTrackerImpl(std::shared_ptr<celix::impl::SvcTrackerEntry> entry, std::function<void()> untrackCallback, bool active);

    }
}