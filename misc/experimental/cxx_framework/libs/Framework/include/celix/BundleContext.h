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
#include "celix/UseServicesBuilder.h"
#include "celix/ComponentManager.h"

namespace celix {

    class BundleContext {
    public:
        BundleContext() = default;
        virtual ~BundleContext() = default;

        BundleContext(celix::BundleContext &&) = delete;
        BundleContext(const celix::BundleContext &) = delete;
        BundleContext& operator=(celix::BundleContext &&) = delete;
        BundleContext& operator=(const celix::BundleContext &) = delete;

        template<typename I>
        celix::ServiceRegistration registerService(std::shared_ptr<I> svc, celix::Properties props = {});

        template<typename I>
        celix::ServiceRegistration registerServiceFactory(std::shared_ptr<celix::IServiceFactory<I>> factory, celix::Properties props = {});

        template<typename F>
        celix::ServiceRegistration registerFunctionService(std::string_view functionName, F&& function, celix::Properties props = {});

        template<typename I>
        celix::ServiceRegistrationBuilder<I> buildServiceRegistration();

        template<typename F>
        celix::FunctionServiceRegistrationBuilder<F> buildFunctionServiceRegistration(std::string_view functionName);

        template<typename I>
        celix::ServiceTrackerBuilder<I> buildServiceTracker();

//        template<typename F>
//        celix::FunctionServiceTrackerBuilder<F> buildFunctionServiceTracker();

        template<typename I>
        celix::UseServicesBuilder<I> buildUseService();

        template<typename F>
        celix::UseFunctionServiceBuilder<F> buildUseFunctionService(std::string_view functionName);

        template<typename T>
        celix::ComponentManager<T> createComponentManager(std::shared_ptr<T> cmpInstance); //TODO factory / lazy component variant

        template< typename I>
        long findService(const celix::Filter& filter = celix::Filter{});

        template<typename F>
        long findFunctionService(const std::string &functionName, const celix::Filter& filter = celix::Filter{});
        //TODO reg svc factories

        virtual bool useBundle(long bndId, std::function<void(const celix::IBundle &bnd)> use) const = 0;

        virtual int useBundles(std::function<void(const celix::IBundle &bnd)> use, bool includeFrameworkBundle = true) const = 0;

        virtual bool stopBundle(long bndId) = 0;

        virtual bool startBundle(long bndId) = 0;

        //TODO install / uninstall bundles, use & track bundles

        virtual std::shared_ptr<celix::IBundle> bundle() const = 0;
        virtual std::shared_ptr<celix::ServiceRegistry> registry() const = 0;
    };
}






/**********************************************************************************************************************
  Bundle Context Template Implementation
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
inline celix::ServiceRegistration celix::BundleContext::registerFunctionService(std::string_view functionName, F&& function, celix::Properties props) {
    return registry()->registerFunctionService(std::string{functionName}, std::forward<F>(function), std::move(props), bundle());
}

template<typename I>
inline celix::ServiceRegistrationBuilder<I> celix::BundleContext::buildServiceRegistration() {
    return celix::ServiceRegistrationBuilder<I>{bundle(), registry()};
}

template<typename F>
inline celix::FunctionServiceRegistrationBuilder<F> celix::BundleContext::buildFunctionServiceRegistration(std::string_view functionName) {
    return celix::FunctionServiceRegistrationBuilder<F>{bundle(), registry(), std::string{functionName}};
}

template<typename I>
inline celix::ServiceTrackerBuilder<I> celix::BundleContext::buildServiceTracker() {
    return celix::ServiceTrackerBuilder<I>{bundle(), registry()};
}

template<typename I>
inline celix::UseServicesBuilder<I> celix::BundleContext::buildUseService() {
    return celix::UseServicesBuilder<I>{bundle(), registry()};
}

template<typename F>
inline celix::UseFunctionServiceBuilder<F> celix::BundleContext::buildUseFunctionService(std::string_view functionName) {
    return celix::UseFunctionServiceBuilder<F>{bundle(), registry(), std::string{functionName}};
}

template<typename I>
inline long celix::BundleContext::findService(const celix::Filter &filter) {
    return registry()->findService<I>(filter);
}

template<typename F>
inline long celix::BundleContext::findFunctionService(const std::string &functionName, const celix::Filter &filter) {
    return registry()->findFunctionService<F>(functionName, filter);
}

template<typename T>
inline celix::ComponentManager<T> celix::BundleContext::createComponentManager(std::shared_ptr<T> cmpInstance) {
    return celix::ComponentManager<T>{bundle(), registry(), std::move(cmpInstance)};
}
