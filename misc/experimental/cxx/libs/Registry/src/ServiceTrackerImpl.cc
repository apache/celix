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

#include "ServiceTrackerImpl.h"




//#define LOGGER celix::getLogger("celix::ServiceTracker");

static celix::Filter emptyFilter{};
static std::string emptyString{};

class celix::ServiceTracker::Impl {
public:
    const std::shared_ptr<celix::impl::SvcTrackerEntry> entry;
    const std::function<void()> untrackCallback;
    bool active; //TODO make atomic?
};


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

celix::ServiceTracker::Impl *celix::impl::createServiceTrackerImpl(std::shared_ptr<celix::impl::SvcTrackerEntry> entry,
                                                                   std::function<void()> untrackCallback, bool active) {
    return new celix::ServiceTracker::Impl{.entry = std::move(entry), .untrackCallback = std::move(untrackCallback), .active = active};
}
