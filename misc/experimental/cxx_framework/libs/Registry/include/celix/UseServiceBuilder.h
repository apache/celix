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
    class UseServiceBuilder {
    public:
        UseServiceBuilder(std::shared_ptr<celix::IResourceBundle> requester, std::shared_ptr<celix::ServiceRegistry> registry);
        UseServiceBuilder(UseServiceBuilder<I> &&); //TODO noexcept
        UseServiceBuilder<I>& operator=(UseServiceBuilder<I>&&);

        UseServiceBuilder<I>& setFilter(celix::Filter f);

        UseServiceBuilder<I>& setCallback(std::function<void(I &svc)> f);
        UseServiceBuilder<I>& setCallback(std::function<void(I &svc, const celix::Properties &props)> f);
        UseServiceBuilder<I>& setCallback(std::function<void(I &svc, const celix::Properties &props, const celix::IResourceBundle &bnd)> f);

        UseServiceBuilder<I>& setServiceId(long svcId);

        bool use() const;
        int useAll() const;
        UseServiceBuilder<I> copy() const;
    private:
        UseServiceBuilder(const UseServiceBuilder<I>&);
        UseServiceBuilder<I>& operator=(const UseServiceBuilder<I>&);


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

        bool use() const;
        int useAll() const;
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
inline celix::UseServiceBuilder<I>::UseServiceBuilder(
        std::shared_ptr<celix::IResourceBundle> _requester,
        std::shared_ptr<celix::ServiceRegistry> _registry) : requester{std::move(_requester)}, registry{std::move(_registry)} {}

template<typename I>
inline celix::UseServiceBuilder<I>::UseServiceBuilder(celix::UseServiceBuilder<I> &&) = default;

template<typename I>
inline celix::UseServiceBuilder<I> &celix::UseServiceBuilder<I>::operator=(celix::UseServiceBuilder<I> &&) = default;

template<typename I>
inline celix::UseServiceBuilder<I> &celix::UseServiceBuilder<I>::setFilter(celix::Filter f) {
    options.filter = std::move(f);
    return *this;
}

template<typename I>
inline celix::UseServiceBuilder<I> &
celix::UseServiceBuilder<I>::setCallback(std::function<void(I&)> f) {
    options.use = std::move(f);
    return *this;
}

template<typename I>
inline celix::UseServiceBuilder<I> &celix::UseServiceBuilder<I>::setCallback(std::function<void(I&, const celix::Properties &)> f) {
    options.useWithProperties = std::move(f);
    return *this;
}

template<typename I>
inline celix::UseServiceBuilder<I> &celix::UseServiceBuilder<I>::setCallback(std::function<void(I&, const celix::Properties &, const celix::IResourceBundle &)> f) {
    options.useWithOwner = std::move(f);
    return *this;
}


template<typename I>
inline celix::UseServiceBuilder<I>& celix::UseServiceBuilder<I>::setServiceId(long svcId) {
    options.targetServiceId = svcId;
    return *this;
}

template<typename I>
inline bool celix::UseServiceBuilder<I>::use() const {
    return registry->useService(options, requester);
}

template<typename I>
inline int celix::UseServiceBuilder<I>::useAll() const {
    return registry->useServices(options, requester);
}

template<typename I>
inline celix::UseServiceBuilder<I> celix::UseServiceBuilder<I>::copy() const {
    return *this;
}

template<typename I>
inline celix::UseServiceBuilder<I>::UseServiceBuilder(const UseServiceBuilder<I> &) = default;

template<typename I>
inline celix::UseServiceBuilder<I> &celix::UseServiceBuilder<I>::operator=(const UseServiceBuilder<I> &) = default;

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
inline bool celix::UseFunctionServiceBuilder<F>::use() const {
    return registry->useFunctionService(options, requester);
}

template<typename F>
inline int celix::UseFunctionServiceBuilder<F>::useAll() const {
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