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
    class UseServicesBuilder {
    public:
        UseServicesBuilder(std::shared_ptr<celix::IResourceBundle> requester, std::shared_ptr<celix::ServiceRegistry> registry);
        UseServicesBuilder(UseServicesBuilder<I> &&); //TODO noexcept
        UseServicesBuilder<I>& operator=(UseServicesBuilder<I>&&);

        UseServicesBuilder<I>& setFilter(celix::Filter f);

        template<typename Rep, typename Period>
        UseServicesBuilder<I>& waitFor(const std::chrono::duration<Rep, Period>& period);

        UseServicesBuilder<I>& setCallback(std::function<void(I &svc)> f);
        UseServicesBuilder<I>& setCallback(std::function<void(I &svc, const celix::Properties &props)> f);
        UseServicesBuilder<I>& setCallback(std::function<void(I &svc, const celix::Properties &props, const celix::IResourceBundle &bnd)> f);

        UseServicesBuilder<I>& setServiceId(long svcId);

        UseServicesBuilder<I>& setLimit(int limit);

        int use() const;
        UseServicesBuilder<I> copy() const;
    private:
        UseServicesBuilder(const UseServicesBuilder<I>&);
        UseServicesBuilder<I>& operator=(const UseServicesBuilder<I>&);


        const std::shared_ptr<celix::IResourceBundle> requester;
        const std::shared_ptr<celix::ServiceRegistry> registry;
        celix::UseServiceOptions<I> options{};
    };

    template<typename F>
    class UseFunctionServiceBuilder {
    public:
        UseFunctionServiceBuilder(std::shared_ptr<celix::IResourceBundle> requester, std::shared_ptr<celix::ServiceRegistry> registry, std::string functionName);
        UseFunctionServiceBuilder(UseFunctionServiceBuilder &&); //TODO noexcept
        UseFunctionServiceBuilder& operator=(UseFunctionServiceBuilder&&);

        UseFunctionServiceBuilder& setFilter(celix::Filter f);

        UseFunctionServiceBuilder& setCallback(std::function<void(const F& func)> f);
        UseFunctionServiceBuilder& setCallback(std::function<void(const F& func, const celix::Properties &props)> f);
        UseFunctionServiceBuilder& setCallback(std::function<void(const F& func, const celix::Properties &props, const celix::IResourceBundle &bnd)> f);

        UseFunctionServiceBuilder& setServiceId(long svcId);

        UseFunctionServiceBuilder& setLimit(int limit);


        int use() const;
        UseFunctionServiceBuilder copy() const;
    private:
        UseFunctionServiceBuilder(const UseFunctionServiceBuilder&);
        UseFunctionServiceBuilder& operator=(const UseFunctionServiceBuilder&);


        const std::shared_ptr<celix::IResourceBundle> requester;
        const std::shared_ptr<celix::ServiceRegistry> registry;
        celix::UseFunctionServiceOptions<F> options;
    };
    
}

/**********************************************************************************************************************
  UseServiceBuilder Implementation
 **********************************************************************************************************************/

template<typename I>
inline celix::UseServicesBuilder<I>::UseServicesBuilder(
        std::shared_ptr<celix::IResourceBundle> _requester,
        std::shared_ptr<celix::ServiceRegistry> _registry) : requester{std::move(_requester)}, registry{std::move(_registry)} {}

template<typename I>
inline celix::UseServicesBuilder<I>::UseServicesBuilder(celix::UseServicesBuilder<I> &&) = default;

template<typename I>
inline celix::UseServicesBuilder<I> &celix::UseServicesBuilder<I>::operator=(celix::UseServicesBuilder<I> &&) = default;

template<typename I>
inline celix::UseServicesBuilder<I> &celix::UseServicesBuilder<I>::setFilter(celix::Filter f) {
    options.filter = std::move(f);
    return *this;
}

template<typename I>
inline celix::UseServicesBuilder<I> &
celix::UseServicesBuilder<I>::setCallback(std::function<void(I&)> f) {
    options.use = std::move(f);
    return *this;
}

template<typename I>
inline celix::UseServicesBuilder<I> &celix::UseServicesBuilder<I>::setCallback(std::function<void(I&, const celix::Properties &)> f) {
    options.useWithProperties = std::move(f);
    return *this;
}

template<typename I>
inline celix::UseServicesBuilder<I> &celix::UseServicesBuilder<I>::setCallback(std::function<void(I&, const celix::Properties &, const celix::IResourceBundle &)> f) {
    options.useWithOwner = std::move(f);
    return *this;
}


template<typename I>
inline celix::UseServicesBuilder<I>& celix::UseServicesBuilder<I>::setServiceId(long svcId) {
    options.targetServiceId = svcId;
    return *this;
}

template<typename I>
inline celix::UseServicesBuilder<I>& celix::UseServicesBuilder<I>::setLimit(int limit) {
    options.limit = limit;
    return *this;
}

template<typename I>
inline int celix::UseServicesBuilder<I>::use() const {
    return registry->useServices(options, requester);
}

template<typename I>
inline celix::UseServicesBuilder<I> celix::UseServicesBuilder<I>::copy() const {
    return *this;
}

template<typename I>
template<typename Rep, typename Period>
inline celix::UseServicesBuilder<I> &celix::UseServicesBuilder<I>::waitFor(const std::chrono::duration<Rep, Period> &period) {
    options.waitFor = period;
    return *this;
}

template<typename I>
inline celix::UseServicesBuilder<I>::UseServicesBuilder(const UseServicesBuilder<I> &) = default;

template<typename I>
inline celix::UseServicesBuilder<I> &celix::UseServicesBuilder<I>::operator=(const UseServicesBuilder<I> &) = default;

/**********************************************************************************************************************
  UseFunctionServiceBuilder Implementation
 **********************************************************************************************************************/

template<typename F>
inline celix::UseFunctionServiceBuilder<F>::UseFunctionServiceBuilder(
        std::shared_ptr<celix::IResourceBundle> _requester,
        std::shared_ptr<celix::ServiceRegistry> _registry,
        std::string _functionName) : requester{std::move(_requester)}, registry{std::move(_registry)}, options{std::move(_functionName)} {}

template<typename F>
inline celix::UseFunctionServiceBuilder<F>::UseFunctionServiceBuilder(celix::UseFunctionServiceBuilder<F> &&) = default;

template<typename F>
inline celix::UseFunctionServiceBuilder<F> &celix::UseFunctionServiceBuilder<F>::operator=(celix::UseFunctionServiceBuilder<F> &&) = default;

template<typename F>
inline celix::UseFunctionServiceBuilder<F> &celix::UseFunctionServiceBuilder<F>::setFilter(celix::Filter f) {
    options.filter = std::move(f);
    return *this;
}

template<typename F>
inline celix::UseFunctionServiceBuilder<F> &
celix::UseFunctionServiceBuilder<F>::setCallback(std::function<void(const F& func)> f) {
    options.use = std::move(f);
    return *this;
}

template<typename F>
inline celix::UseFunctionServiceBuilder<F> &celix::UseFunctionServiceBuilder<F>::setCallback(
        std::function<void(const F& func, const celix::Properties &)> f) {
    options.useWithProperties = std::move(f);
    return *this;
}

template<typename F>
inline celix::UseFunctionServiceBuilder<F> &celix::UseFunctionServiceBuilder<F>::setCallback(
        std::function<void(const F& func, const celix::Properties &, const celix::IResourceBundle &)> f) {
    options.useWithOwner = std::move(f);
    return *this;
}


template<typename F>
inline celix::UseFunctionServiceBuilder<F>& celix::UseFunctionServiceBuilder<F>::setServiceId(long svcId) {
    options.targetServiceId = svcId;
    return *this;
}

template<typename F>
inline celix::UseFunctionServiceBuilder<F>& celix::UseFunctionServiceBuilder<F>::setLimit(int limit) {
    options.limit = limit;
    return *this;
}

template<typename F>
inline int celix::UseFunctionServiceBuilder<F>::use() const {
    return registry->useFunctionServices(options, requester);
}

template<typename F>
inline celix::UseFunctionServiceBuilder<F> celix::UseFunctionServiceBuilder<F>::copy() const {
    return *this;
}

template<typename F>
inline celix::UseFunctionServiceBuilder<F>::UseFunctionServiceBuilder(const UseFunctionServiceBuilder<F> &) = default;

template<typename F>
inline celix::UseFunctionServiceBuilder<F> &celix::UseFunctionServiceBuilder<F>::operator=(const UseFunctionServiceBuilder<F> &) = default;