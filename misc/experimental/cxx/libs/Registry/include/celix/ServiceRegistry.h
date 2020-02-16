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

#ifndef CXX_CELIX_SERVICEREGISTRY_H
#define CXX_CELIX_SERVICEREGISTRY_H

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

namespace celix {

    //forward declaration
    class ServiceRegistry;

    // RAII service registration: out of scope -> deregister service
    // NOTE access not thread safe -> TODO make thread save?
    class ServiceRegistration {
    public:
        ServiceRegistration();
        ServiceRegistration(ServiceRegistration &&rhs) noexcept;
        ServiceRegistration(const ServiceRegistration &rhs) = delete;
        ~ServiceRegistration();
        ServiceRegistration& operator=(ServiceRegistration &&rhs) noexcept;
        ServiceRegistration& operator=(const ServiceRegistration &rhs) = delete;

        long serviceId() const;
        bool valid() const;
        const celix::Properties& properties() const;
        const std::string& serviceName() const;
        bool factory() const;
        bool registered() const;

        void unregister();
    private:
        class Impl; //opaque impl class
        std::unique_ptr<celix::ServiceRegistration::Impl> pimpl;
        explicit ServiceRegistration(celix::ServiceRegistration::Impl *impl);
        friend ServiceRegistry;
    };

    template<typename I>
    struct ServiceTrackerOptions {
        std::string filter{};

        std::function<void(std::shared_ptr<I> svc)> set{};
        std::function<void(std::shared_ptr<I> svc, const celix::Properties &props)> setWithProperties{};
        std::function<void(std::shared_ptr<I> svc, const celix::Properties &props, const celix::IResourceBundle &owner)> setWithOwner{};

        std::function<void(std::shared_ptr<I> svc)> add{};
        std::function<void(std::shared_ptr<I> svc, const celix::Properties &props)> addWithProperties{};
        std::function<void(std::shared_ptr<I> svc, const celix::Properties &props, const celix::IResourceBundle &owner)> addWithOwner{};

        std::function<void(std::shared_ptr<I> svc)> remove{};
        std::function<void(std::shared_ptr<I> svc, const celix::Properties &props)> removeWithProperties{};
        std::function<void(std::shared_ptr<I> svc, const celix::Properties &props, const celix::IResourceBundle &owner)> removeWithOwner{};

        std::function<void(std::vector<std::shared_ptr<I>> rankedServices)> update{};
        std::function<void(std::vector<std::tuple<std::shared_ptr<I>, const celix::Properties*>> rankedServices)> updateWithProperties{};
        std::function<void(std::vector<std::tuple<std::shared_ptr<I>, const celix::Properties*, const celix::IResourceBundle *>> rankedServices)> updateWithOwner{};

        //TODO lock free update calls atomics, rcu, hazard pointers ??

        /**
         * pre and post update hooks, can be used if a trigger is needed before or after an service update.
         */
        std::function<void()> preServiceUpdateHook{};
        std::function<void()> postServiceUpdateHook{};
    };

    //RAII service tracker: out of scope -> stop tracker
    // NOTE access not thread safe -> TODO make thread save?
    class ServiceTracker {
    public:
        ServiceTracker();
        ServiceTracker(ServiceTracker &&rhs) noexcept;
        ServiceTracker(const ServiceTracker &rhs) = delete;
        ~ServiceTracker();
        ServiceTracker& operator=(ServiceTracker &&rhs) noexcept;
        ServiceTracker& operator=(const ServiceTracker &rhs) = delete;

        int trackCount() const;
        const std::string& serviceName() const;
        const celix::Filter& filter() const;
        bool valid() const;

        //TODO use(Function)Service(s) calls

        void stop();
    private:
        class Impl; //opaque impl class
        std::unique_ptr<celix::ServiceTracker::Impl> pimpl;
        explicit ServiceTracker(celix::ServiceTracker::Impl *impl);
        friend ServiceRegistry;
    };

    //NOTE access thread safe
    class ServiceRegistry {
    public:
        explicit ServiceRegistry(std::string name);
        ~ServiceRegistry();
        ServiceRegistry(celix::ServiceRegistry &&rhs);
        ServiceRegistry& operator=(celix::ServiceRegistry&& rhs);
        ServiceRegistry& operator=(ServiceRegistry &rhs) = delete;
        ServiceRegistry(const ServiceRegistry &rhs) = delete;

        const std::string& name() const;

        template<typename I>
        celix::ServiceRegistration registerService(I svc, celix::Properties props = {}, std::shared_ptr<const celix::IResourceBundle> owner = {});

        template<typename I>
        celix::ServiceRegistration registerService(std::shared_ptr<I> svc, celix::Properties props = {}, std::shared_ptr<const celix::IResourceBundle> owner = {}) {
            //TOOD refactor to using a service factory to store the shared or unique_ptr
            auto svcName = celix::serviceName<I>();
            auto voidSvc = std::static_pointer_cast<I>(svc);
            return registerGenericService(svcName, voidSvc, std::move(props), std::move(owner));
        }

        template<typename F>
        celix::ServiceRegistration registerFunctionService(
                const std::string &functionName,
                F&& func, celix::Properties props = {},
                std::shared_ptr<const celix::IResourceBundle> owner = {});

        template<typename I>
        celix::ServiceRegistration registerServiceFactory(
                std::shared_ptr<celix::IServiceFactory<I>> factory,
                celix::Properties props = {},
                std::shared_ptr<const celix::IResourceBundle> owner = {}) {
            auto svcName = celix::serviceName<I>();
            return registerServiceFactory(svcName, std::move(factory), std::move(props), std::move(owner));
        }

        template<typename F>
        celix::ServiceRegistration registerFunctionServiceFactory(
                const std::string &functionName,
                std::shared_ptr<celix::IServiceFactory<F>> factory,
                celix::Properties props = {},
                std::shared_ptr<const celix::IResourceBundle> owner = {}) {
            auto svcName = celix::functionServiceName<F>(functionName);
            return registerServiceFactory(svcName, std::move(factory), std::move(props), std::move(owner));
        }

        template<typename I>
        //NOTE C++17 typename std::enable_if<!std::is_callable<I>::value, long>::type
        long findService(const std::string &filter = "") const {
            auto services = findServices<I>(filter);
            return services.size() > 0 ? services[0] : -1L;
        }

        template<typename I>
        //NOTE C++17 typename std::enable_if<std::is_callable<I>::value, long>::type
        long findFunctionService(const std::string &functionName, const std::string &filter = "") const {
            auto services = findFunctionServices<I>(functionName, filter);
            return services.size() > 0 ? services[0] : -1L;
        }

        template<typename I>
        //NOTE C++17 typename std::enable_if<!std::is_callable<I>::value, std::vector<long>>::type
        std::vector<long> findServices(const std::string &filter = "") const {
            auto svcName = celix::serviceName<I>();
            return findAnyServices(svcName, filter);
        }

        template<typename F>
        //NOTE C++17 typename std::enable_if<std::is_callable<I>::value, std::vector<long>>::type
        std::vector<long> findFunctionServices(const std::string &functionName, const std::string &filter = "") const {
            auto svcName = celix::functionServiceName<F>(functionName);
            return findAnyServices(svcName, filter);
        }

        template<typename I>
        celix::ServiceTracker trackServices(celix::ServiceTrackerOptions<I> options = {}, std::shared_ptr<const celix::IResourceBundle> requester = {}) {
            auto svcName = celix::serviceName<I>();
            return trackServices<I>(svcName, std::move(options), std::move(requester));
        }

        template<typename F>
        celix::ServiceTracker trackFunctionServices(std::string functionName, celix::ServiceTrackerOptions<F> options = {}, std::shared_ptr<const celix::IResourceBundle> requester = {}) {
            auto svcName = celix::functionServiceName<F>(functionName);
            return trackServices<F>(svcName, std::move(options), std::move(requester));
        }

        template<typename I>
        int useServices(std::function<void(I& svc)> use, const std::string &filter = "", std::shared_ptr<const celix::IResourceBundle> requester = {}) const {
            auto svcName = celix::serviceName<I>();
            return useServices<I>(svcName, use, nullptr, nullptr, filter, std::move(requester));
        }

        template<typename I>
        int useServices(std::function<void(I& svc, const celix::Properties &props)> use, const std::string &filter = "", std::shared_ptr<const celix::IResourceBundle> requester = {}) const {
            auto svcName = celix::serviceName<I>();
            return useServices<I>(svcName, nullptr, use, nullptr, filter, std::move(requester));
        }

        template<typename I>
        int useServices(std::function<void(I& svc, const celix::Properties &props, const celix::IResourceBundle &bnd)> use, const std::string &filter = "", std::shared_ptr<const celix::IResourceBundle> requester = {}) const {
            auto svcName = celix::serviceName<I>();
            return useServices<I>(svcName, nullptr, nullptr, use, filter, std::move(requester));
        }

        template<typename F>
        int useFunctionServices(const std::string &functionName, std::function<void(F &function)> use, const std::string &filter = "", std::shared_ptr<const celix::IResourceBundle> requester = {}) const {
            auto svcName = celix::functionServiceName<F>(functionName);
            return useServices<F>(svcName, use, nullptr, nullptr, filter, std::move(requester));
        }

        template<typename F>
        int useFunctionServices(const std::string &functionName, std::function<void(F &function, const celix::Properties&)> use, const std::string &filter = "", std::shared_ptr<const celix::IResourceBundle> requester = {}) const {
            auto svcName = celix::functionServiceName<F>(functionName);
            return useServices<F>(svcName, nullptr, use, nullptr, filter, std::move(requester));
        }

        template<typename F>
        int useFunctionServices(const std::string &functionName, std::function<void(F &function, const celix::Properties&, const celix::IResourceBundle&)> use, const std::string &filter = "", std::shared_ptr<const celix::IResourceBundle> requester = {}) const {
            auto svcName = celix::functionServiceName<F>(functionName);
            return useServices<F>(svcName, nullptr, nullptr, use, filter, std::move(requester));
        }

        template<typename I>
        bool useServiceWithId(long svcId, std::function<void(I& svc)> use, std::shared_ptr<const celix::IResourceBundle> requester = {}) const {
            auto svcName = celix::serviceName<I>();
            std::string filter = std::string{"("} + celix::SERVICE_ID + "=" + std::to_string(svcId) + ")";
            return useService<I>(svcName, use, nullptr, nullptr, filter, std::move(requester));
        }

        template<typename I>
        bool useServiceWithId(long svcId, std::function<void(I& svc, const celix::Properties &props)> use, std::shared_ptr<const celix::IResourceBundle> requester = {}) const {
            auto svcName = celix::serviceName<I>();
            std::string filter = std::string{"("} + celix::SERVICE_ID + "=" + std::to_string(svcId) + ")";
            return useService<I>(svcName, nullptr, use, nullptr, filter, std::move(requester));
        }

        template<typename I>
        bool useServiceWithId(long svcId, std::function<void(I& svc, const celix::Properties &props, const celix::IResourceBundle &bnd)> use, std::shared_ptr<const celix::IResourceBundle> requester = {}) const {
            auto svcName = celix::serviceName<I>();
            std::string filter = std::string{"("} + celix::SERVICE_ID + "=" + std::to_string(svcId) + ")";
            return useService<I>(svcName, nullptr, nullptr, use, filter, std::move(requester));
        }

        template<typename I>
        bool useService(std::function<void(I& svc)> use, const std::string &filter = "", std::shared_ptr<const celix::IResourceBundle> requester = {}) const {
            auto svcName = celix::serviceName<I>();
            return useService<I>(svcName, use, nullptr, nullptr, filter, std::move(requester));
        }

        template<typename I>
        bool useService(std::function<void(I& svc, const celix::Properties &props)> use, const std::string &filter = "", std::shared_ptr<const celix::IResourceBundle> requester = {}) const {
            auto svcName = celix::serviceName<I>();
            return useService<I>(svcName, nullptr, use, nullptr, filter, std::move(requester));
        }

        template<typename I>
        bool useService(std::function<void(I& svc, const celix::Properties &props, const celix::IResourceBundle &bnd)> use, const std::string &filter = "", std::shared_ptr<const celix::IResourceBundle> requester = {}) const {
            auto svcName = celix::serviceName<I>();
            return useService<I>(svcName, nullptr, nullptr, use, filter, std::move(requester));
        }

        template<typename F>
        bool useFunctionServiceWithId(const std::string &functionName, long svcId, std::function<void(F &function)> use, std::shared_ptr<const celix::IResourceBundle> requester = {}) const {
            auto svcName = celix::functionServiceName<F>(functionName);
            std::string filter = std::string{"("} + celix::SERVICE_ID + "=" + std::to_string(svcId) + ")";
            return useService<F>(svcName, use, nullptr, nullptr, filter, std::move(requester));
        }

        template<typename F>
        bool useFunctionServiceWithId(const std::string &functionName, long svcId, std::function<void(F &function, const celix::Properties&)> use, std::shared_ptr<const celix::IResourceBundle> requester = {}) const {
            auto svcName = celix::functionServiceName<F>(functionName);
            std::string filter = std::string{"("} + celix::SERVICE_ID + "=" + std::to_string(svcId) + ")";
            return useService<F>(svcName, nullptr, use, nullptr, filter, std::move(requester));
        }

        template<typename F>
        bool useFunctionServiceWithId(const std::string &functionName, long svcId, std::function<void(F &function, const celix::Properties&, const celix::IResourceBundle&)> use, std::shared_ptr<const celix::IResourceBundle> requester = {}) const {
            auto svcName = celix::functionServiceName<F>(functionName);
            std::string filter = std::string{"("} + celix::SERVICE_ID + "=" + std::to_string(svcId) + ")";
            return useService<F>(svcName, nullptr, nullptr, use, filter, std::move(requester));
        }

        template<typename F>
        bool useFunctionService(const std::string &functionName, std::function<void(F &function)> use, const std::string &filter = "", std::shared_ptr<const celix::IResourceBundle> requester = {}) const {
            auto svcName = celix::functionServiceName<F>(functionName);
            return useService<F>(svcName, use, nullptr, nullptr, filter, std::move(requester));
        }

        template<typename F>
        bool useFunctionService(const std::string &functionName, std::function<void(F &function, const celix::Properties&)> use, const std::string &filter = "", std::shared_ptr<const celix::IResourceBundle> requester = {}) const {
            auto svcName = celix::functionServiceName<F>(functionName);
            return useService<F>(svcName, nullptr, use, nullptr, filter, std::move(requester));
        }

        template<typename F>
        bool useFunctionService(const std::string &functionName, std::function<void(F &function, const celix::Properties&, const celix::IResourceBundle&)> use, const std::string &filter = "", std::shared_ptr<const celix::IResourceBundle> requester = {}) const {
            auto svcName = celix::functionServiceName<F>(functionName);
            return useService<F>(svcName, nullptr, nullptr, use, filter, std::move(requester));
        }


        //GENERIC / ANY calls. note these work on void

        int useAnyServices(
                const std::string &svcName,
                const std::function<void(const std::shared_ptr<void> &svc, const celix::Properties &props,const celix::IResourceBundle &bnd)> &use,
                const std::string &filter = {},
                std::shared_ptr<const celix::IResourceBundle> requester = {}) const;

        bool useAnyService(
                const std::string &svcName,
                const std::function<void(const std::shared_ptr<void> &svc, const celix::Properties &props, const celix::IResourceBundle &bnd)> &use,
                const std::string &filter = {},
                std::shared_ptr<const celix::IResourceBundle> requester = {}) const;

        std::vector<long> findAnyServices(const std::string &name, const std::string &filter = {}) const;


        celix::ServiceTracker trackAnyServices(
                std::string svcName,
                ServiceTrackerOptions<void> options,
                std::shared_ptr<const celix::IResourceBundle> requester = {});

        //some aditional registry info
        std::vector<std::string> listAllRegisteredServiceNames() const;
        long nrOfRegisteredServices() const;
        long nrOfServiceTrackers() const;
    private:
        class Impl;
        std::unique_ptr<celix::ServiceRegistry::Impl> pimpl;

        //register services
        celix::ServiceRegistration registerGenericService(
                std::string svcName,
                std::shared_ptr<void> svc,
                celix::Properties props,
                std::shared_ptr<const celix::IResourceBundle> owner);

        template<typename I>
        celix::ServiceRegistration registerServiceFactory(
                std::string svcName,
                std::shared_ptr<celix::IServiceFactory<I>> factory,
                celix::Properties props,
                std::shared_ptr<const celix::IResourceBundle> owner);

        //use Services
        template<typename I>
        int useServices(
                const std::string &svcName,
                std::function<void(I &svc)> use,
                std::function<void(I &svc, const celix::Properties &props)> useWithProps,
                std::function<void(I &svc, const celix::Properties &props, const celix::IResourceBundle &bnd)> useWithOwner,
                const std::string &filter,
                std::shared_ptr<const celix::IResourceBundle> requester) const;

        celix::ServiceRegistration registerServiceFactory(std::string svcName, std::shared_ptr<celix::IServiceFactory<void>> factory, celix::Properties props, std::shared_ptr<const celix::IResourceBundle> owner);

        template<typename I>
        bool useService(
                const std::string &svcName,
                std::function<void(I &svc)> use,
                std::function<void(I &svc, const celix::Properties &props)> useWithProps,
                std::function<void(I &svc, const celix::Properties &props, const celix::IResourceBundle &bnd)> useWithOwner,
                const std::string &filter,
                std::shared_ptr<const celix::IResourceBundle> requester) const;

        //track services
        template<typename I>
        celix::ServiceTracker trackServices(
                std::string svcName,
                celix::ServiceTrackerOptions<I> options,
                std::shared_ptr<const celix::IResourceBundle> requester);

    };
}


std::ostream& operator<<(std::ostream &out, const celix::ServiceRegistration& serviceRegistration);









/**********************************************************************************************************************
  Service Registration Implementation
 **********************************************************************************************************************/


template<typename I>
celix::ServiceRegistration celix::ServiceRegistry::registerService(I svc, celix::Properties props, std::shared_ptr<const celix::IResourceBundle> owner) {
    auto svcName = celix::serviceName<I>();
    auto *servicesStore = new std::vector<I>{};
    servicesStore->emplace_back(std::move(svc));
    auto voidSvc = std::shared_ptr<void>(static_cast<void*>(&(*servicesStore)[0]), [servicesStore](void*){delete servicesStore;}); //transform to std::shared_ptr to minimize the underlining impl needed.
    return registerGenericService(svcName, std::move(voidSvc), std::move(props), std::move(owner));
}

template<typename F>
inline celix::ServiceRegistration celix::ServiceRegistry::registerFunctionService(const std::string &functionName, F&& func, celix::Properties props, std::shared_ptr<const celix::IResourceBundle> owner) {
    class FunctionServiceFactory : public celix::IServiceFactory<void> {
    public:
        FunctionServiceFactory(F&& _function) : function{std::forward<F>(_function)} {}

        void* getService(const celix::IResourceBundle &, const celix::Properties &) noexcept override {
            return static_cast<void*>(&function);
        }
        void ungetService(const celix::IResourceBundle &, const celix::Properties &) noexcept override {
            //nop;
        }
    private:
        F function;
    };
    auto factory = std::shared_ptr<celix::IServiceFactory<void>>{new FunctionServiceFactory{std::forward<F>(func)}};

    std::string svcName = celix::functionServiceName<typename std::remove_reference<F>::type>(functionName);

    return registerServiceFactory(std::move(svcName), factory, std::move(props), std::move(owner));
}

template<typename I>
inline celix::ServiceRegistration celix::ServiceRegistry::registerServiceFactory(std::string svcName, std::shared_ptr<celix::IServiceFactory<I>> factory, celix::Properties props, std::shared_ptr<const celix::IResourceBundle> owner) {
    class VoidServiceFactory : public celix::IServiceFactory<void> {
    public:
        VoidServiceFactory(std::shared_ptr<celix::IServiceFactory<I>> _factory) : factory{std::move(_factory)} {}

        void* getService(const celix::IResourceBundle &bnd, const celix::Properties &props) noexcept override {
            I* service = factory->getService(bnd, props);
            return static_cast<void*>(service);
        }
        void ungetService(const celix::IResourceBundle &bnd, const celix::Properties &props) noexcept override {
            factory->ungetService(bnd, props);
        }
    private:
        std::shared_ptr<celix::IServiceFactory<I>> factory;
    };

    auto voidFactory = std::shared_ptr<celix::IServiceFactory<void>>{new VoidServiceFactory{std::move(factory)}};
    return registerServiceFactory(std::move(svcName), std::move(voidFactory), std::move(props), std::move(owner));
}

template<typename I>
inline int celix::ServiceRegistry::useServices(
        const std::string &svcName,
        std::function<void(I &svc)> use,
        std::function<void(I &svc, const celix::Properties &props)> useWithProps,
        std::function<void(I &svc, const celix::Properties &props, const celix::IResourceBundle &bnd)> useWithOwner,
        const std::string &filter,
        std::shared_ptr<const celix::IResourceBundle> requester) const {

    std::function<void(std::shared_ptr<void>, const celix::Properties&, const celix::IResourceBundle&)> voidUse = [&](std::shared_ptr<void> svc, const celix::Properties &props, const celix::IResourceBundle &bnd) {
        std::shared_ptr<I> typedSvc = std::static_pointer_cast<I>(svc);
        if (use) {
            use(*typedSvc);
        }
        if (useWithProps) {
            useWithProps(*typedSvc, props);
        }
        if (useWithOwner) {
            useWithOwner(*typedSvc, props, bnd);
        }
    };
    return useAnyServices(svcName, std::move(voidUse), filter, std::move(requester));
}

template<typename I>
inline bool celix::ServiceRegistry::useService(
        const std::string &svcName,
        std::function<void(I &svc)> use,
        std::function<void(I &svc, const celix::Properties &props)> useWithProps,
        std::function<void(I &svc, const celix::Properties &props, const celix::IResourceBundle &bnd)> useWithOwner,
        const std::string &filter,
        std::shared_ptr<const celix::IResourceBundle> requester) const {

    std::function<void(std::shared_ptr<void>,const celix::Properties&, const celix::IResourceBundle&)> voidUse = [&](std::shared_ptr<void> svc, const celix::Properties &props, const celix::IResourceBundle &bnd) -> void {
        std::shared_ptr<I> typedSvc = std::static_pointer_cast<I>(svc);
        if (use) {
            use(*typedSvc);
        }
        if (useWithProps) {
            useWithProps(*typedSvc, props);
        }
        if (useWithOwner) {
            useWithOwner(*typedSvc, props, bnd);
        }
    };
    return useAnyService(svcName, std::move(voidUse), filter, std::move(requester));
}

template<typename I>
inline celix::ServiceTracker celix::ServiceRegistry::trackServices(std::string svcName,
                                                                      ServiceTrackerOptions<I> options,
                                                                      std::shared_ptr<const celix::IResourceBundle> requester) {
    ServiceTrackerOptions<void> opts{};
    opts.filter = std::move(options.filter);

    if (options.setWithOwner != nullptr) {
        auto set = std::move(options.setWithOwner);
        opts.setWithOwner = [set](std::shared_ptr<void> svc, const celix::Properties &props, const celix::IResourceBundle &owner){
            auto typedSvc = std::static_pointer_cast<I>(svc);
            set(typedSvc, props, owner);
        };
    }
    if (options.setWithProperties != nullptr) {
        auto set = std::move(options.setWithProperties);
        opts.setWithProperties = [set](std::shared_ptr<void> svc, const celix::Properties &props){
            auto typedSvc = std::static_pointer_cast<I>(svc);
            set(typedSvc, props);
        };
    }
    if (options.set != nullptr) {
        auto set = std::move(options.set);
        opts.set = [set](std::shared_ptr<void> svc){
            auto typedSvc = std::static_pointer_cast<I>(svc);
            set(typedSvc);
        };
    }

    if (options.addWithOwner != nullptr) {
        auto add = std::move(options.addWithOwner);
        opts.addWithOwner = [add](std::shared_ptr<void> svc, const celix::Properties &props, const celix::IResourceBundle &bnd) {
            auto typedSvc = std::static_pointer_cast<I>(svc);
            add(typedSvc, props, bnd);
        };
    }
    if (options.addWithProperties != nullptr) {
        auto add = std::move(options.addWithProperties);
        opts.addWithProperties = [add](std::shared_ptr<void> svc, const celix::Properties &props) {
            auto typedSvc = std::static_pointer_cast<I>(svc);
            add(typedSvc, props);
        };
    }
    if (options.add != nullptr) {
        auto add = std::move(options.add);
        opts.add = [add](std::shared_ptr<void> svc) {
            auto typedSvc = std::static_pointer_cast<I>(svc);
            add(typedSvc);
        };
    }

    if (options.removeWithOwner != nullptr) {
        auto rem = std::move(options.removeWithOwner);
        opts.removeWithOwner = [rem](std::shared_ptr<void> svc, const celix::Properties &props, const celix::IResourceBundle &bnd) {
            auto typedSvc = std::static_pointer_cast<I>(svc);
            rem(typedSvc, props, bnd);
        };
    }
    if (options.removeWithProperties != nullptr) {
        auto rem = std::move(options.removeWithProperties);
        opts.removeWithProperties = [rem](std::shared_ptr<void> svc, const celix::Properties &props) {
            auto typedSvc = std::static_pointer_cast<I>(svc);
            rem(typedSvc, props);
        };
    }
    if (options.remove != nullptr) {
        auto rem = std::move(options.remove);
        opts.remove = [rem](std::shared_ptr<void> svc) {
            auto typedSvc = std::static_pointer_cast<I>(svc);
            rem(typedSvc);
        };
    }

    if (options.updateWithOwner != nullptr) {
        auto update = std::move(options.updateWithOwner);
        opts.updateWithOwner = [update](std::vector<std::tuple<std::shared_ptr<void>, const celix::Properties *, const celix::IResourceBundle*>> rankedServices) {
            std::vector<std::tuple<std::shared_ptr<I>, const celix::Properties*, const celix::IResourceBundle*>> typedServices{};
            typedServices.reserve(rankedServices.size());
            for (auto &tuple : rankedServices) {
                auto typedSvc = std::static_pointer_cast<I>(std::get<0>(tuple));
                typedServices.push_back(std::make_tuple(typedSvc, std::get<1>(tuple), std::get<2>(tuple)));
            }
            update(std::move(typedServices));
        };
    }
    if (options.updateWithProperties != nullptr) {
        auto update = std::move(options.updateWithProperties);
        opts.updateWithProperties = [update](std::vector<std::tuple<std::shared_ptr<void>, const celix::Properties *>> rankedServices) {
            std::vector<std::tuple<std::shared_ptr<I>, const celix::Properties*>> typedServices{};
            typedServices.reserve(rankedServices.size());
            for (auto &tuple : rankedServices) {
                auto typedSvc = std::static_pointer_cast<I>(std::get<0>(tuple));
                typedServices.push_back(std::make_tuple(typedSvc, std::get<1>(tuple)));
            }
            update(std::move(typedServices));
        };
    }
    if (options.update != nullptr) {
        auto update = std::move(options.update);
        opts.update = [update](std::vector<std::shared_ptr<void>> rankedServices) {
            std::vector<std::shared_ptr<I>> typedServices{};
            typedServices.reserve(rankedServices.size());
            for (auto &svc : rankedServices) {
                auto typedSvc = std::static_pointer_cast<I>(svc);
                typedServices.push_back(typedSvc);
            }
            update(std::move(typedServices));
        };
    }

    if (options.preServiceUpdateHook) {
        opts.preServiceUpdateHook = std::move(options.preServiceUpdateHook);
    }
    if (options.postServiceUpdateHook) {
        opts.postServiceUpdateHook = std::move(options.postServiceUpdateHook);
    }

    return trackAnyServices(std::move(svcName), std::move(opts), std::move(requester));
}

#endif //CXX_CELIX_SERVICEREGISTRY_H
