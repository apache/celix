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

#include "celix/ServiceRegistry.h"

namespace celix {

    template<typename I>
    class ServiceTrackerBuilder {
    public:
        ServiceTrackerBuilder(std::shared_ptr<celix::IResourceBundle> requester, std::shared_ptr<celix::ServiceRegistry> registry);
        ServiceTrackerBuilder(ServiceTrackerBuilder<I> &&); //TODO noexcept
        ServiceTrackerBuilder<I>& operator=(ServiceTrackerBuilder<I>&&);

        ServiceTrackerBuilder<I>& setFilter(celix::Filter f);

        ServiceTrackerBuilder<I>& setCallback(std::function<void(const std::shared_ptr<I> &svc)> f);
        ServiceTrackerBuilder<I>& setCallback(std::function<void(const std::shared_ptr<I> &svc, const celix::Properties &props)> f);
        ServiceTrackerBuilder<I>& setCallback(std::function<void(const std::shared_ptr<I> &svc, const celix::Properties &props, const celix::IResourceBundle &bnd)> f);

        //TODO rest of the callbacks

        ServiceTracker build() const;
        ServiceTrackerBuilder<I> copy() const;
        bool valid() const;
    private:
        ServiceTrackerBuilder(const ServiceTrackerBuilder<I>&);
        ServiceTrackerBuilder<I>& operator=(const ServiceTrackerBuilder<I>&);


        const std::shared_ptr<celix::IResourceBundle> requester;
        const std::shared_ptr<celix::ServiceRegistry> registry;
        celix::ServiceTrackerOptions<I> options{};
    };

    //TODO FunctionServiceTrackerBuilder
}

/**********************************************************************************************************************
  ServiceTrackerBuilder Implementation
 **********************************************************************************************************************/

template<typename I>
inline celix::ServiceTrackerBuilder<I>::ServiceTrackerBuilder(
        std::shared_ptr<celix::IResourceBundle> _requester,
        std::shared_ptr<celix::ServiceRegistry> _registry) : requester{std::move(_requester)}, registry{std::move(_registry)} {}

template<typename I>
inline celix::ServiceTrackerBuilder<I>::ServiceTrackerBuilder(celix::ServiceTrackerBuilder<I> &&) = default;

template<typename I>
inline celix::ServiceTrackerBuilder<I> &celix::ServiceTrackerBuilder<I>::operator=(celix::ServiceTrackerBuilder<I> &&) = default;

template<typename I>
inline celix::ServiceTrackerBuilder<I> &celix::ServiceTrackerBuilder<I>::setFilter(celix::Filter f) {
    options.filter = std::move(f);
    return *this;
}

template<typename I>
inline celix::ServiceTrackerBuilder<I> &
celix::ServiceTrackerBuilder<I>::setCallback(std::function<void(const std::shared_ptr<I> &)> f) {
    options.set = std::move(f);
    return *this;
}

template<typename I>
inline celix::ServiceTrackerBuilder<I> &celix::ServiceTrackerBuilder<I>::setCallback(
        std::function<void(const std::shared_ptr<I> &, const celix::Properties &)> f) {
    options.setWithProperties = std::move(f);
    return *this;
}

template<typename I>
inline celix::ServiceTrackerBuilder<I> &celix::ServiceTrackerBuilder<I>::setCallback(
        std::function<void(const std::shared_ptr<I> &, const celix::Properties &, const celix::IResourceBundle &)> f) {
    options.setWithOwner = std::move(f);
    return *this;
}

template<typename I>
inline celix::ServiceTracker celix::ServiceTrackerBuilder<I>::build() const {
    if (valid()) {
        //TODO note using copy, make also make a buildAndClear? or make build() also clear and add a buildAndKeep()?
        return registry->trackServices(options, requester);
    } else {
        //LOG(ERROR) << "Invalid builder. Builder should have a service, function service or factory service instance";
        abort();
        //TODO return invalid svc reg
    }
}

template<typename I>
inline celix::ServiceTrackerBuilder<I> celix::ServiceTrackerBuilder<I>::copy() const {
    return *this;
}

template<typename I>
inline bool celix::ServiceTrackerBuilder<I>::valid() const {
    return true;
}

template<typename I>
inline celix::ServiceTrackerBuilder<I>::ServiceTrackerBuilder(const ServiceTrackerBuilder<I> &) = default;

template<typename I>
inline celix::ServiceTrackerBuilder<I> &celix::ServiceTrackerBuilder<I>::operator=(const ServiceTrackerBuilder<I> &) = default;
