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


#include "ServiceRegistationImpl.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>


#include "ServiceRegistationImpl.h"
#include "celix/Constants.h"

static auto logger = spdlog::stdout_color_mt("celix::ServiceRegistration");


class celix::ServiceRegistration::Impl {
public:
    const std::shared_ptr<const celix::impl::SvcEntry> entry;
    const std::function<void()> unregisterCallback;
    bool registered; //TODO make atomic?
};


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
        logger->debug("Unregistering service {} with id {}", pimpl->entry->svcName, pimpl->entry->svcId);
        pimpl->registered = false; //TODO make thread safe
        pimpl->unregisterCallback();
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


celix::ServiceRegistration::Impl* celix::impl::createServiceRegistrationImpl(std::shared_ptr<const celix::impl::SvcEntry> entry, std::function<void()> unregisterCallback, bool registered) {
    return new celix::ServiceRegistration::Impl{.entry = std::move(entry), .unregisterCallback = std::move(unregisterCallback), .registered = registered};
}
