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

#include <utility>
#include <vector>
#include <functional>
#include <memory>

#include "celix/Constants.h"
#include "celix/Properties.h"
#include "celix/Filter.h"
#include "celix/Utils.h"
#include "celix/IResourceBundle.h"
#include "celix/IServiceFactory.h"
#include "celix/ServiceTracker.h"
#include "celix/ServiceRegistration.h"

namespace celix {

    template<typename I>
    struct ServiceTrackerOptions {
        celix::Filter filter{};

        std::function<void(const std::shared_ptr<I> &svc)> set{};
        std::function<void(const std::shared_ptr<I> &svc, const celix::Properties &props)> setWithProperties{};
        std::function<void(const std::shared_ptr<I> &svc, const celix::Properties &props, const celix::IResourceBundle &owner)> setWithOwner{};

        std::function<void(const std::shared_ptr<I> &svc)> add{};
        std::function<void(const std::shared_ptr<I> &svc, const celix::Properties &props)> addWithProperties{};
        std::function<void(const std::shared_ptr<I> &svc, const celix::Properties &props, const celix::IResourceBundle &owner)> addWithOwner{};

        std::function<void(const std::shared_ptr<I> &svc)> remove{};
        std::function<void(const std::shared_ptr<I> &svc, const celix::Properties &props)> removeWithProperties{};
        std::function<void(const std::shared_ptr<I> &svc, const celix::Properties &props, const celix::IResourceBundle &owner)> removeWithOwner{};

        std::function<void(std::vector<std::shared_ptr<I>> rankedServices)> update{};
        std::function<void(std::vector<std::tuple<std::shared_ptr<I>, const celix::Properties*>> rankedServices)> updateWithProperties{};
        std::function<void(std::vector<std::tuple<std::shared_ptr<I>, const celix::Properties*, const celix::IResourceBundle*>> rankedServices)> updateWithOwner{};

        //TODO lock free update calls atomics, rcu, hazard pointers ??

        /**
         * pre and post update hooks, can be used if a trigger is needed before or after an service update.
         */
        std::function<void()> preServiceUpdateHook{};
        std::function<void()> postServiceUpdateHook{};
    };

//    template<typename F>
//    struct FunctionServiceTrackerOptions : public celix::ServiceTrackerOptions<F> {
//        //TODO remove inheritance
//        explicit FunctionServiceTrackerOptions(const std::string &fn) : functionName{fn} {}
//        const std::string functionName;
//    };

    template<typename I>
    struct UseServiceOptions {
        celix::Filter filter{};
        long targetServiceId{-1L}; //note -1 means not targeting a specific service id.
        std::function<void(const std::shared_ptr<I> &svc)> use{};
        std::function<void(const std::shared_ptr<I> &svc, const celix::Properties &props)> useWithProperties{};
        std::function<void(const std::shared_ptr<I> &svc, const celix::Properties &props, const celix::IResourceBundle &bundle)> useWithOwner{};
    };

    template<typename F>
    struct UseFunctionServiceOptions {
        explicit UseFunctionServiceOptions(const std::string &fn) : functionName{fn} {}
        const std::string functionName;
        celix::Filter filter{};
        long targetServiceId{-1L}; //note -1 means not targeting a specific service id.
        std::function<void(const std::function<F>& func)> use{};
        std::function<void(const std::function<F>& func, const celix::Properties &props)> useWithProperties{};
        std::function<void(const std::function<F>& func, const celix::Properties &props, const celix::IResourceBundle &bundle)> useWithOwner{};
    };

    //NOTE access thread safe
    class ServiceRegistry {
    public:
        explicit ServiceRegistry(std::string name);

        ~ServiceRegistry();

        ServiceRegistry(celix::ServiceRegistry &&rhs) noexcept;

        ServiceRegistry& operator=(celix::ServiceRegistry &&rhs);

        ServiceRegistry& operator=(ServiceRegistry &rhs) = delete;

        ServiceRegistry(const ServiceRegistry &rhs) = delete;

        const std::string &name() const;

        template<typename I>
        celix::ServiceRegistration registerService(std::shared_ptr<I> svc, celix::Properties props = {}, const std::shared_ptr<celix::IResourceBundle>& owner = {});

        template<typename I>
        celix::ServiceRegistration registerServiceFactory(std::shared_ptr<celix::IServiceFactory<I>> factory, celix::Properties props = {}, const std::shared_ptr<celix::IResourceBundle>& owner = {});

        template<typename F>
        celix::ServiceRegistration registerFunctionService(const std::string &functionName, std::function<F> function, celix::Properties props = {}, const std::shared_ptr<celix::IResourceBundle>& owner = {});

        template<typename I>
        celix::ServiceTracker trackServices(celix::ServiceTrackerOptions<I> opts, const std::shared_ptr<celix::IResourceBundle>& requester = {});

//        template<typename F>
//        celix::ServiceTracker trackFunctionServices(celix::FunctionServiceTrackerOptions<F> opts, const std::shared_ptr<celix::IResourceBundle>& requester = {});

        template<typename I>
        int useServices(celix::UseServiceOptions<I> opts, const std::shared_ptr<celix::IResourceBundle>& requester = {}) const;

        template<typename I>
        bool useService(celix::UseServiceOptions<I> opts, const std::shared_ptr<celix::IResourceBundle>& requester = {}) const;

        template<typename F>
        int useFunctionServices(celix::UseFunctionServiceOptions<F> opts, const std::shared_ptr<celix::IResourceBundle>& requester = {}) const;

        template<typename F>
        bool useFunctionService(celix::UseFunctionServiceOptions<F> opts, const std::shared_ptr<celix::IResourceBundle>& requester = {}) const;

        template<typename I>
        //NOTE C++17 typename std::enable_if<!std::is_callable<I>::value, long>::type
        long findService(const celix::Filter& filter = celix::Filter{}) const {
            auto services = findServices<I>(filter);
            return services.size() > 0 ? services[0] : -1L;
        }

        template<typename F>
        //NOTE C++17 typename std::enable_if<std::is_callable<I>::value, long>::type
        long findFunctionService(const std::string &functionName, const celix::Filter& filter = celix::Filter{}) const {
            auto services = findFunctionServices<F>(functionName, filter);
            return services.size() > 0 ? services[0] : -1L;
        }

        template<typename I>
        //NOTE C++17 typename std::enable_if<!std::is_callable<I>::value, std::vector<long>>::type
        std::vector<long> findServices(const celix::Filter& filter = celix::Filter{}) const {
            auto svcName = celix::serviceName<I>();
            return findAnyServices(svcName, filter);
        }

        template<typename F>
        //NOTE C++17 typename std::enable_if<std::is_callable<I>::value, std::vector<long>>::type
        std::vector<long> findFunctionServices(const std::string &functionName, const celix::Filter& filter = celix::Filter{}) const {
            auto svcName = celix::functionServiceName<F>(functionName);
            return findAnyServices(svcName, filter);
        }


        //GENERIC / ANY calls. note these work on void use with care

        int useAnyServices(const std::string &svcOrFunctionName, celix::UseServiceOptions<void> opts, const std::shared_ptr<celix::IResourceBundle>& requester = {}) const;

        //int useAnyFunctionServices(celix::UseFunctionServiceOptions<std::function<void()>> opts, const std::shared_ptr<celix::IResourceBundle>& requester = {}) const;

        bool useAnyService(const std::string &svcOrFunctionName, celix::UseServiceOptions<void> opts, const std::shared_ptr<celix::IResourceBundle>& requester = {}) const;

        //bool useAnyFunctionService(celix::UseFunctionServiceOptions<std::function<void()>> opts, const std::shared_ptr<celix::IResourceBundle>& requester = {}) const;

        celix::ServiceRegistration registerAnyService(const std::string& svcName, std::shared_ptr<void> service, celix::Properties props = {}, const std::shared_ptr<celix::IResourceBundle>& owner = {});

        celix::ServiceRegistration registerAnyServiceFactory(const std::string& svcName, std::shared_ptr<celix::IServiceFactory<void>> factory, celix::Properties props = {}, const std::shared_ptr<celix::IResourceBundle>& owner = {});

        // TODO now registered through a service factory. Can this be improved?
        // celix::ServiceRegistration registerAnyFunctionService(const std::string& functionName, std::function<void>&& function, celix::Properties props = {}, const std::shared_ptr<celix::IResourceBundle>& owner = {});

        celix::ServiceTracker trackAnyServices(const std::string &svcName, celix::ServiceTrackerOptions<void> opts, const std::shared_ptr<celix::IResourceBundle>& requester = {});

//        celix::ServiceTracker trackAnyFunctionService(celix::FunctionServiceTrackerOptions<void()> opts, const std::shared_ptr<celix::IResourceBundle>& requester = {});

        std::vector<long> findAnyServices(const std::string &svcName, const celix::Filter& filter = celix::Filter{}) const;

        //some additional registry info
        std::vector<std::string> listAllRegisteredServiceNames() const;

        long nrOfRegisteredServices() const;

        long nrOfServiceTrackers() const;

    private:
        class Impl;

        std::unique_ptr<celix::ServiceRegistry::Impl> pimpl;
    };
}

std::ostream& operator<<(std::ostream &out, celix::ServiceRegistration& serviceRegistration);









/**********************************************************************************************************************
  Service Registration Implementation
 **********************************************************************************************************************/


template<typename I>
inline celix::ServiceRegistration celix::ServiceRegistry::registerService(std::shared_ptr<I> svc, celix::Properties props, const std::shared_ptr<celix::IResourceBundle>& owner) {
    auto svcName = celix::serviceName<I>();
    std::shared_ptr<void> anySvc = std::static_pointer_cast<void>(svc);
    return registerAnyService(std::move(svcName), std::move(anySvc), std::move(props), owner);
}

template<typename I>
inline celix::ServiceRegistration celix::ServiceRegistry::registerServiceFactory(std::shared_ptr<celix::IServiceFactory<I>> outerFactory, celix::Properties props, const std::shared_ptr<celix::IResourceBundle>& owner) {
    class VoidServiceFactory : public celix::IServiceFactory<void> {
    public:
        explicit VoidServiceFactory(std::shared_ptr<celix::IServiceFactory<I>> _factory) : factory{std::move(_factory)} {}
        ~VoidServiceFactory() override = default;

        std::shared_ptr<void> createBundleSpecificService(const celix::IResourceBundle &bnd, const celix::Properties &props) override {
            std::shared_ptr<I> typedSvc = factory->createBundleSpecificService(bnd, props);
            return std::static_pointer_cast<void>(typedSvc);
        }
        void bundleSpecificServiceRemoved(const celix::IResourceBundle &bnd, const celix::Properties &props) override {
            factory->bundleSpecificServiceRemoved(bnd, props);
        }
    private:
        std::shared_ptr<celix::IServiceFactory<I>> factory;
    };
    auto anyFactory = std::make_shared<VoidServiceFactory>(outerFactory);
    auto svcName = celix::serviceName<I>();
    return registerAnyService(std::move(svcName), std::move(anyFactory), std::move(props), owner);
}

template<typename F>
inline celix::ServiceRegistration celix::ServiceRegistry::registerFunctionService(const std::string& functionName, std::function<F> outerFunction, celix::Properties props, const std::shared_ptr<celix::IResourceBundle>& owner) {
    class FunctionServiceFactory : public celix::IServiceFactory<void> {
    public:
        explicit FunctionServiceFactory(std::function<F> _function) : function{_function /*TODO move*/}, ptr{&function, [](std::function<F>*){/*nop*/}} {}

        ~FunctionServiceFactory() override {
            assert(ptr.use_count() == 1); //note service should not be used any more. TODO improve handling
        }

        std::shared_ptr<void> createBundleSpecificService(const celix::IResourceBundle &, const celix::Properties &) override {
            return std::static_pointer_cast<void>(ptr);
        }
        void bundleSpecificServiceRemoved(const celix::IResourceBundle &, const celix::Properties &) override {
            //nop;
        }
    private:
        std::function<F> function;
        std::shared_ptr<std::function<F>> ptr;
    };
    auto factory = std::shared_ptr<celix::IServiceFactory<void>>{new FunctionServiceFactory{std::move(outerFunction)}};

    auto svcName = celix::functionServiceName<F>(functionName);
    return registerAnyServiceFactory(svcName, std::move(factory), std::move(props), owner);
}


template<typename I>
inline int celix::ServiceRegistry::useServices(celix::UseServiceOptions<I> opts, const std::shared_ptr<celix::IResourceBundle>& requester) const {
    auto voidUse = [opts](std::shared_ptr<void> svc, celix::Properties &props, celix::IResourceBundle &bnd) -> void {
        std::shared_ptr<I> typedSvc = std::static_pointer_cast<I>(svc);
        if (opts.use) {
            use(typedSvc);
        }
        if (opts.useWithProps) {
            useWithProps(typedSvc, props);
        }
        if (opts.useWithOwner) {
            useWithOwner(typedSvc, props, bnd);
        }
    };

    celix::UseServiceOptions<void> anyOpts{};
    anyOpts.filter = std::move(opts.filter);
    anyOpts.targetServiceId = opts.targetServiceId;
    anyOpts.useWithOwner = std::move(voidUse);
    auto svcName = celix::serviceName<I>();
    return useAnyServices(std::move(svcName), std::move(anyOpts), requester);
}

template<typename I>
inline bool celix::ServiceRegistry::useService(celix::UseServiceOptions<I> opts, const std::shared_ptr<celix::IResourceBundle>& requester) const {

    auto voidUse = [opts](std::shared_ptr<void> svc, const celix::Properties &props, const celix::IResourceBundle &bnd) -> void {
        std::shared_ptr<I> typedSvc = std::static_pointer_cast<I>(svc);
        if (opts.use) {
            opts.use(typedSvc);
        }
        if (opts.useWithProperties) {
            opts.useWithProperties(typedSvc, props);
        }
        if (opts.useWithOwner) {
            opts.useWithOwner(typedSvc, props, bnd);
        }
    };

    celix::UseServiceOptions<void> anyOpts{};
    anyOpts.filter = std::move(opts.filter);
    anyOpts.targetServiceId = opts.targetServiceId;
    anyOpts.useWithOwner = std::move(voidUse);
    auto svcName = celix::serviceName<I>();
    return useAnyService(std::move(svcName), std::move(anyOpts), requester);
}



template<typename F>
int celix::ServiceRegistry::useFunctionServices(celix::UseFunctionServiceOptions<F> opts, const std::shared_ptr<celix::IResourceBundle>& requester) const {
    auto voidUse = [opts](const std::shared_ptr<void>& rawFunction, const celix::Properties &props, const celix::IResourceBundle &bnd) -> void {
        std::shared_ptr<std::function<F>> function = std::static_pointer_cast<std::function<F>>(rawFunction);
        if (opts.use) {
            opts.use(*function);
        }
        if (opts.useWithProps) {
            opts.useWithProps(*function, props);
        }
        if (opts.useWithOwner) {
            opts.useWithOwner(*function, props, bnd);
        }
    };

    celix::UseServiceOptions<void> anyOpts{};
    anyOpts.filter = std::move(opts.filter);
    anyOpts.targetServiceId = opts.targetServiceId;
    anyOpts.useWithOwner = std::move(voidUse);
    return useAnyServices(opts.functionName, std::move(anyOpts), requester);
}

template<typename F>
bool celix::ServiceRegistry::useFunctionService(celix::UseFunctionServiceOptions<F> opts, const std::shared_ptr<celix::IResourceBundle>& requester) const {
    auto voidUse = [opts](const std::shared_ptr<void>& rawFunction, const celix::Properties &props, const celix::IResourceBundle &bnd) -> void {
        std::shared_ptr<std::function<F>> function = std::static_pointer_cast<std::function<F>>(rawFunction);
        if (opts.use) {
            opts.use(*function);
        }
        if (opts.useWithProperties) {
            opts.useWithProperties(*function, props);
        }
        if (opts.useWithOwner) {
            opts.useWithOwner(*function, props, bnd);
        }
    };

    celix::UseServiceOptions<void> anyOpts{};
    anyOpts.filter = std::move(opts.filter);
    anyOpts.targetServiceId = opts.targetServiceId;
    anyOpts.useWithOwner = std::move(voidUse);
    return useAnyServices(opts.functionName, std::move(anyOpts), requester);
}

template<typename I>
inline celix::ServiceTracker celix::ServiceRegistry::trackServices(celix::ServiceTrackerOptions<I> opts, const std::shared_ptr<celix::IResourceBundle>& requester) {

    ServiceTrackerOptions<void> anyOpts{};
    anyOpts.filter = std::move(opts.filter);

    if (opts.setWithOwner != nullptr) {
        auto set = std::move(opts.setWithOwner);
        anyOpts.setWithOwner = [set](const std::shared_ptr<void>& svc, const celix::Properties &props, const celix::IResourceBundle &owner){
            auto typedSvc = std::static_pointer_cast<I>(svc);
            set(typedSvc, props, owner);
        };
    }
    if (opts.setWithProperties != nullptr) {
        auto set = std::move(opts.setWithProperties);
        anyOpts.setWithProperties = [set](const std::shared_ptr<void>& svc, const celix::Properties &props){
            auto typedSvc = std::static_pointer_cast<I>(svc);
            set(typedSvc, props);
        };
    }
    if (opts.set != nullptr) {
        auto set = std::move(opts.set);
        anyOpts.set = [set](const std::shared_ptr<void>& svc){
            auto typedSvc = std::static_pointer_cast<I>(svc);
            set(typedSvc);
        };
    }

    if (opts.addWithOwner != nullptr) {
        auto add = std::move(opts.addWithOwner);
        anyOpts.addWithOwner = [add](const std::shared_ptr<void>& svc, const celix::Properties &props, const celix::IResourceBundle &bnd) {
            auto typedSvc = std::static_pointer_cast<I>(svc);
            add(typedSvc, props, bnd);
        };
    }
    if (opts.addWithProperties != nullptr) {
        auto add = std::move(opts.addWithProperties);
        anyOpts.addWithProperties = [add](const std::shared_ptr<void>& svc, const celix::Properties &props) {
            auto typedSvc = std::static_pointer_cast<I>(svc);
            add(typedSvc, props);
        };
    }
    if (opts.add != nullptr) {
        auto add = std::move(opts.add);
        anyOpts.add = [add](const std::shared_ptr<void>& svc) {
            auto typedSvc = std::static_pointer_cast<I>(svc);
            add(typedSvc);
        };
    }

    if (opts.removeWithOwner != nullptr) {
        auto rem = std::move(opts.removeWithOwner);
        anyOpts.removeWithOwner = [rem](const std::shared_ptr<void>& svc, const celix::Properties &props, const celix::IResourceBundle &bnd) {
            auto typedSvc = std::static_pointer_cast<I>(svc);
            rem(typedSvc, props, bnd);
        };
    }
    if (opts.removeWithProperties != nullptr) {
        auto rem = std::move(opts.removeWithProperties);
        anyOpts.removeWithProperties = [rem](const std::shared_ptr<void>& svc, const celix::Properties &props) {
            auto typedSvc = std::static_pointer_cast<I>(svc);
            rem(typedSvc, props);
        };
    }
    if (opts.remove != nullptr) {
        auto rem = std::move(opts.remove);
        anyOpts.remove = [rem](const std::shared_ptr<void>& svc) {
            auto typedSvc = std::static_pointer_cast<I>(svc);
            rem(typedSvc);
        };
    }

    if (opts.updateWithOwner != nullptr) {
        auto update = std::move(opts.updateWithOwner);
        anyOpts.updateWithOwner = [update](std::vector<std::tuple<std::shared_ptr<void>, const celix::Properties *, const celix::IResourceBundle*>> rankedServices) {
            std::vector<std::tuple<std::shared_ptr<I>, const celix::Properties*, const celix::IResourceBundle*>> typedServices{};
            typedServices.reserve(rankedServices.size());
            for (auto &tuple : rankedServices) {
                auto typedSvc = std::static_pointer_cast<I>(std::get<0>(tuple));
                typedServices.push_back(std::make_tuple(typedSvc, std::get<1>(tuple), std::get<2>(tuple)));
            }
            update(std::move(typedServices));
        };
    }
    if (opts.updateWithProperties != nullptr) {
        auto update = std::move(opts.updateWithProperties);
        anyOpts.updateWithProperties = [update](std::vector<std::tuple<std::shared_ptr<void>, const celix::Properties *>> rankedServices) {
            std::vector<std::tuple<std::shared_ptr<I>, const celix::Properties*>> typedServices{};
            typedServices.reserve(rankedServices.size());
            for (auto &tuple : rankedServices) {
                auto typedSvc = std::static_pointer_cast<I>(std::get<0>(tuple));
                typedServices.push_back(std::make_tuple(typedSvc, std::get<1>(tuple)));
            }
            update(std::move(typedServices));
        };
    }
    if (opts.update != nullptr) {
        auto update = std::move(opts.update);
        anyOpts.update = [update](std::vector<std::shared_ptr<void>> rankedServices) {
            std::vector<std::shared_ptr<I>> typedServices{};
            typedServices.reserve(rankedServices.size());
            for (auto &svc : rankedServices) {
                auto typedSvc = std::static_pointer_cast<I>(svc);
                typedServices.push_back(typedSvc);
            }
            update(std::move(typedServices));
        };
    }

    if (opts.preServiceUpdateHook) {
        anyOpts.preServiceUpdateHook = std::move(opts.preServiceUpdateHook);
    }
    if (opts.postServiceUpdateHook) {
        anyOpts.postServiceUpdateHook = std::move(opts.postServiceUpdateHook);
    }

    auto svcName = celix::serviceName<I>();

    return trackAnyServices(svcName, std::move(anyOpts), requester);
}

//template<typename F>
//celix::ServiceTracker celix::ServiceRegistry::trackFunctionServices(celix::FunctionServiceTrackerOptions<F> opts, const std::shared_ptr<celix::IResourceBundle>& requester) {
//    FunctionServiceTrackerOptions<void()> anyOpts{opts.functionName};
//    anyOpts.filter = std::move(opts.filter);
//    anyOpts.functionName = std::move(opts.functionName);
//
//    if (opts.setWithOwner != nullptr) {
//        auto set = std::move(opts.setWithOwner);
//        anyOpts.setWithOwner = [set](std::shared_ptr<void> svc, celix::Properties &props, celix::IResourceBundle &owner){
//            auto typedSvc = std::static_pointer_cast<F>(svc);
//            set(typedSvc, props, owner);
//        };
//    }
//    if (opts.setWithProperties != nullptr) {
//        auto set = std::move(opts.setWithProperties);
//        anyOpts.setWithProperties = [set](std::shared_ptr<void> svc, celix::Properties &props){
//            auto typedSvc = std::static_pointer_cast<F>(svc);
//            set(typedSvc, props);
//        };
//    }
//    if (opts.set != nullptr) {
//        auto set = std::move(opts.set);
//        anyOpts.set = [set](std::shared_ptr<void> svc){
//            auto typedSvc = std::static_pointer_cast<F>(svc);
//            set(typedSvc);
//        };
//    }
//
//    if (opts.addWithOwner != nullptr) {
//        auto add = std::move(opts.addWithOwner);
//        anyOpts.addWithOwner = [add](std::shared_ptr<void> svc, celix::Properties &props, celix::IResourceBundle &bnd) {
//            auto typedSvc = std::static_pointer_cast<F>(svc);
//            add(typedSvc, props, bnd);
//        };
//    }
//    if (opts.addWithProperties != nullptr) {
//        auto add = std::move(opts.addWithProperties);
//        anyOpts.addWithProperties = [add](std::shared_ptr<void> svc, celix::Properties &props) {
//            auto typedSvc = std::static_pointer_cast<F>(svc);
//            add(typedSvc, props);
//        };
//    }
//    if (opts.add != nullptr) {
//        auto add = std::move(opts.add);
//        anyOpts.add = [add](std::shared_ptr<void> svc) {
//            auto typedSvc = std::static_pointer_cast<F>(svc);
//            add(typedSvc);
//        };
//    }
//
//    if (opts.removeWithOwner != nullptr) {
//        auto rem = std::move(opts.removeWithOwner);
//        anyOpts.removeWithOwner = [rem](std::shared_ptr<void> svc, celix::Properties &props, celix::IResourceBundle &bnd) {
//            auto typedSvc = std::static_pointer_cast<F>(svc);
//            rem(typedSvc, props, bnd);
//        };
//    }
//    if (opts.removeWithProperties != nullptr) {
//        auto rem = std::move(opts.removeWithProperties);
//        anyOpts.removeWithProperties = [rem](std::shared_ptr<void> svc, celix::Properties &props) {
//            auto typedSvc = std::static_pointer_cast<F>(svc);
//            rem(typedSvc, props);
//        };
//    }
//    if (opts.remove != nullptr) {
//        auto rem = std::move(opts.remove);
//        anyOpts.remove = [rem](std::shared_ptr<void> svc) {
//            auto typedSvc = std::static_pointer_cast<F>(svc);
//            rem(typedSvc);
//        };
//    }
//
//    if (opts.updateWithOwner != nullptr) {
//        auto update = std::move(opts.updateWithOwner);
//        anyOpts.updateWithOwner = [update](std::vector<std::tuple<std::shared_ptr<void>, celix::Properties *, celix::IResourceBundle*>> rankedServices) {
//            std::vector<std::tuple<std::shared_ptr<F>, celix::Properties*, celix::IResourceBundle*>> typedServices{};
//            typedServices.reserve(rankedServices.size());
//            for (auto &tuple : rankedServices) {
//                auto typedSvc = std::static_pointer_cast<F>(std::get<0>(tuple));
//                typedServices.push_back(std::make_tuple(typedSvc, std::get<1>(tuple), std::get<2>(tuple)));
//            }
//            update(std::move(typedServices));
//        };
//    }
//    if (opts.updateWithProperties != nullptr) {
//        auto update = std::move(opts.updateWithProperties);
//        anyOpts.updateWithProperties = [update](std::vector<std::tuple<std::shared_ptr<void>, celix::Properties *>> rankedServices) {
//            std::vector<std::tuple<std::shared_ptr<F>, celix::Properties*>> typedServices{};
//            typedServices.reserve(rankedServices.size());
//            for (auto &tuple : rankedServices) {
//                auto typedSvc = std::static_pointer_cast<F>(std::get<0>(tuple));
//                typedServices.push_back(std::make_tuple(typedSvc, std::get<1>(tuple)));
//            }
//            update(std::move(typedServices));
//        };
//    }
//    if (opts.update != nullptr) {
//        auto update = std::move(opts.update);
//        anyOpts.update = [update](std::vector<std::shared_ptr<void>> rankedServices) {
//            std::vector<std::shared_ptr<F>> typedServices{};
//            typedServices.reserve(rankedServices.size());
//            for (auto &svc : rankedServices) {
//                auto typedSvc = std::static_pointer_cast<F>(svc);
//                typedServices.push_back(typedSvc);
//            }
//            update(std::move(typedServices));
//        };
//    }
//
//    if (opts.preServiceUpdateHook) {
//        anyOpts.preServiceUpdateHook = std::move(opts.preServiceUpdateHook);
//    }
//    if (opts.postServiceUpdateHook) {
//        anyOpts.postServiceUpdateHook = std::move(opts.postServiceUpdateHook);
//    }
//
//    return trackAnyFunctionService(std::move(anyOpts), requester);
//}
