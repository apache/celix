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


#include "celix/Framework.h"
#include "celix/IBundle.h"
#include "celix/ServiceTrackerBuilder.h"
#include "celix/ServiceRegistrationBuilder.h"
#include "celix/UseServiceBuilder.h"

namespace celix {

    class BundleContext {
    public:
        explicit BundleContext(std::shared_ptr<celix::IBundle>);
        ~BundleContext();

        BundleContext(celix::BundleContext &&) = delete;
        BundleContext(const celix::BundleContext &) = delete;
        BundleContext& operator=(celix::BundleContext &&) = delete;
        BundleContext& operator=(const celix::BundleContext &) = delete;

        template<typename I>
        celix::ServiceRegistration registerService(std::shared_ptr<I> svc, celix::Properties props = {});

        template<typename I>
        celix::ServiceRegistration registerServiceFactory(std::shared_ptr<celix::IServiceFactory<I>> factory, celix::Properties props = {});

        template<typename F>
        celix::ServiceRegistration registerFunctionService(std::string functionName, F&& function, celix::Properties props = {});

        template<typename I>
        celix::ServiceRegistrationBuilder<I> buildServiceRegistration();

        template<typename F>
        celix::FunctionServiceRegistrationBuilder<F> buildFunctionServiceRegistration(std::string functionName);

        template<typename I>
        celix::ServiceTrackerBuilder<I> buildServiceTracker();

//        template<typename F>
//        celix::FunctionServiceTrackerBuilder<F> buildFunctionServiceTracker();

        template<typename I>
        celix::UseServiceBuilder<I> buildUseService();

        template<typename F>
        celix::UseFunctionServiceBuilder<F> buildUseFunctionService(std::string functionName);

        //TODO reg svc factories

        bool useBundle(long bndId, std::function<void(const celix::IBundle &bnd)> use) const;

        int useBundles(std::function<void(const celix::IBundle &bnd)> use, bool includeFrameworkBundle = true) const;

        bool stopBundle(long bndId);

        bool startBundle(long bndId);

        //TODO install / uninstall bundles, use & track bundles

        const std::shared_ptr<celix::IBundle>& bundle() const;
        const std::shared_ptr<celix::ServiceRegistry>& registry() const;
    private:

        class Impl;
        std::unique_ptr<Impl> pimpl;
    };
}






/**********************************************************************************************************************
  Bundle Context Implementation
 **********************************************************************************************************************/

template<typename I>
inline celix::ServiceRegistration celix::BundleContext::registerService(std::shared_ptr<I> svc, celix::Properties props) {
    return registry()->registerService<I>(std::move(svc), std::move(props), bundle());
}

template<typename I>
inline celix::ServiceRegistration celix::BundleContext::registerServiceFactory(std::shared_ptr<celix::IServiceFactory<I>> factory, celix::Properties props) {
    return registry()->registerServiceFactory(std::move(factory), std::move(props), bundle());
}

template<typename F>
inline celix::ServiceRegistration celix::BundleContext::registerFunctionService(std::string functionName, F&& function, celix::Properties props) {
    return registry()->registerFunctionService(std::move(functionName), std::forward<F>(function), std::move(props), bundle());
}

inline bool celix::BundleContext::useBundle(long bndId, std::function<void(const celix::IBundle &bnd)> use) const {
    return bundle()->framework().useBundle(bndId, std::move(use));
}

inline int celix::BundleContext::useBundles(std::function<void(const celix::IBundle &bnd)> use, bool includeFrameworkBundle) const {
    return bundle()->framework().useBundles(std::move(use), includeFrameworkBundle);
}

inline bool celix::BundleContext::stopBundle(long bndId) {
    return bundle()->framework().stopBundle(bndId);
}

inline bool celix::BundleContext::startBundle(long bndId) {
    return bundle()->framework().startBundle(bndId);
}

template<typename I>
inline celix::ServiceRegistrationBuilder<I> celix::BundleContext::buildServiceRegistration() {
    return celix::ServiceRegistrationBuilder<I>{bundle(), registry()};
}

template<typename F>
inline celix::FunctionServiceRegistrationBuilder<F> celix::BundleContext::buildFunctionServiceRegistration(std::string functionName) {
    return celix::FunctionServiceRegistrationBuilder<F>{bundle(), registry(), std::move(functionName)};
}

template<typename I>
inline celix::ServiceTrackerBuilder<I> celix::BundleContext::buildServiceTracker() {
    return celix::ServiceTrackerBuilder<I>{bundle(), registry()};
}

template<typename I>
inline celix::UseServiceBuilder<I> celix::BundleContext::buildUseService() {
    return celix::UseServiceBuilder<I>{bundle(), registry()};
}

template<typename F>
inline celix::UseFunctionServiceBuilder<F> celix::BundleContext::buildUseFunctionService(std::string functionName) {
    return celix::UseFunctionServiceBuilder<F>{bundle(), registry(), std::move(functionName)};
}