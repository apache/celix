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

#include <glog/logging.h>

#include "celix/ServiceRegistry.h"

namespace celix {

    template<typename I>
    class ServiceRegistrationBuilder {
    public:
        ServiceRegistrationBuilder(std::shared_ptr<celix::IResourceBundle> owner, std::shared_ptr<celix::ServiceRegistry> registry);
        ServiceRegistrationBuilder(ServiceRegistrationBuilder<I> &&);
        ServiceRegistrationBuilder<I>& operator=(ServiceRegistrationBuilder<I>&&);

        ServiceRegistrationBuilder<I>& setService(std::shared_ptr<I> svc);
        ServiceRegistrationBuilder<I>& setService(I *svc);
        ServiceRegistrationBuilder<I>& setServiceFactory(std::shared_ptr<celix::IServiceFactory<I>> factory);
        ServiceRegistrationBuilder<I>& setServiceFactory(celix::IServiceFactory<I>* factory);

        ServiceRegistrationBuilder<I>& setProperties(celix::Properties);
        ServiceRegistrationBuilder<I>& addProperty(const std::string &name, const std::string &value);
        ServiceRegistrationBuilder<I>& addProperties(celix::Properties);

        ServiceRegistration build();
        ServiceRegistrationBuilder<I> copy() const;
        bool valid() const;
    private:
        ServiceRegistrationBuilder(const ServiceRegistrationBuilder<I>&);
        ServiceRegistrationBuilder<I>& operator=(const ServiceRegistrationBuilder<I>&);


        const std::shared_ptr<celix::IResourceBundle> owner;
        const std::shared_ptr<celix::ServiceRegistry> registry;
        std::shared_ptr<I> service{};
        std::shared_ptr<celix::IServiceFactory<I>> serviceFactory{};
        celix::Properties properties{};
    };

    template<typename F>
    class FunctionServiceRegistrationBuilder {
    public:
        FunctionServiceRegistrationBuilder(std::shared_ptr<celix::IResourceBundle> owner, std::shared_ptr<celix::ServiceRegistry> registry, std::string functionName);
        FunctionServiceRegistrationBuilder(FunctionServiceRegistrationBuilder<F> &&); /*todo make noexcept without default in header ...*/
        FunctionServiceRegistrationBuilder<F>& operator=(FunctionServiceRegistrationBuilder<F>&&);

        FunctionServiceRegistrationBuilder<F>& setFunctionService(F&& function);
        FunctionServiceRegistrationBuilder<F>& setFunctionService(F function);
        //FunctionServiceRegistrationBuilder<F>& setFunctionServiceFactory(std::shared_ptr<celix::IServiceFactory<F>> factory);

        FunctionServiceRegistrationBuilder<F>& setProperties(celix::Properties);
        FunctionServiceRegistrationBuilder<F>& addProperty(const std::string &name, const std::string &value);
        FunctionServiceRegistrationBuilder<F>& addProperties(celix::Properties);

        ServiceRegistration build() const;
        FunctionServiceRegistrationBuilder<F> copy() const;
        bool valid() const;
    private:
        FunctionServiceRegistrationBuilder(const FunctionServiceRegistrationBuilder<F>&);
        FunctionServiceRegistrationBuilder<F>& operator=(const FunctionServiceRegistrationBuilder<F>&);


        const std::shared_ptr<celix::IResourceBundle> owner;
        const std::shared_ptr<celix::ServiceRegistry> registry;
        const std::string functionName;
        std::function<F> function{};
        celix::Properties properties{};
    };

    //TODO (Function)ServiceTrackerBuilder
    //TODO (Function)UseServices
}









/**********************************************************************************************************************
  ServiceRegistrationBuilder Implementation
 **********************************************************************************************************************/


template<typename I>
inline celix::ServiceRegistrationBuilder<I>::ServiceRegistrationBuilder(
        std::shared_ptr<celix::IResourceBundle> _owner,
        std::shared_ptr<ServiceRegistry> _registry) : owner{std::move(_owner)}, registry{std::move(_registry)} {}


template<typename I>
inline bool celix::ServiceRegistrationBuilder<I>::valid() const {
    return (service || serviceFactory) && ! (service  && serviceFactory); //only one is set.
}

template<typename I>
inline celix::ServiceRegistrationBuilder<I>& celix::ServiceRegistrationBuilder<I>::setProperties(celix::Properties props) {
    properties = std::move(props);
    return *this;
}

template<typename I>
inline celix::ServiceRegistrationBuilder<I> &celix::ServiceRegistrationBuilder<I>::addProperties(celix::Properties props) {
    //TODO insert with move
    properties.insert(props.begin(), props.end());
    return *this;
}


template<typename I>
inline celix::ServiceRegistrationBuilder<I>&
celix::ServiceRegistrationBuilder<I>::addProperty(const std::string &name, const std::string &value) {
    properties[name] = value;
    return *this;
}

template<typename I>
inline celix::ServiceRegistration celix::ServiceRegistrationBuilder<I>::build() {
    if (valid()) {
//        celix::Properties nProps{};
//        std::shared_ptr<I> nSvc{};
//        std::shared_ptr<celix::IServiceFactory<I>> nFac{};
//        std::swap(properties, nProps);
//        std::swap(service, nSvc);
//        std::swap(serviceFactory, nFac);
//        if (nSvc) {
//            return registry->registerService<I>(std::move(nSvc), std::move(nProps), owner);
//        } else {
//            return registry->registerServiceFactory<I>(std::move(nFac), std::move(nProps), owner);
//        }

        //TODO note using copy, make also make a buildAndClear? or make build() also clear and add a buildAndKeep()?
        if (service) {
            return registry->registerService<I>(service, properties, owner);
        } else {
            return registry->registerServiceFactory<I>(serviceFactory, properties, owner);
        }
    } else {
        LOG(ERROR) << "Invalid builder. Builder should have a service, function service or factory service instance";
        abort();
        //TODO create and throw exception
    }
}

template<typename I>
inline celix::ServiceRegistrationBuilder<I> celix::ServiceRegistrationBuilder<I>::copy() const {
    return *this;
}

template<typename I>
inline celix::ServiceRegistrationBuilder<I> &celix::ServiceRegistrationBuilder<I>::setService(std::shared_ptr<I> svc) {
    service = std::move(svc);
    serviceFactory = {};
    return *this;
}

template<typename I>
inline celix::ServiceRegistrationBuilder<I> &celix::ServiceRegistrationBuilder<I>::setService(I *svc) {
    service = std::shared_ptr<I>{svc, [](I*){/*nop*/}};
    serviceFactory = {};
    return *this;
}

template<typename I>
inline celix::ServiceRegistrationBuilder<I> &
celix::ServiceRegistrationBuilder<I>::setServiceFactory(std::shared_ptr<celix::IServiceFactory<I>> factory) {
    serviceFactory = std::move(factory);
    service = {};
    return *this;
}

template<typename I>
inline celix::ServiceRegistrationBuilder<I> &celix::ServiceRegistrationBuilder<I>::setServiceFactory(celix::IServiceFactory<I> *factory) {
    serviceFactory = std::shared_ptr<celix::IServiceFactory<I>>{factory, [](celix::IServiceFactory<I>*){/*nop*/}};
    service = {};
    return *this;
}

template<typename I>
inline celix::ServiceRegistrationBuilder<I> &
celix::ServiceRegistrationBuilder<I>::operator=(celix::ServiceRegistrationBuilder<I> &&) = default;

template<typename I>
inline celix::ServiceRegistrationBuilder<I>::ServiceRegistrationBuilder(const celix::ServiceRegistrationBuilder<I> &) = default;

template<typename I>
inline celix::ServiceRegistrationBuilder<I> &
celix::ServiceRegistrationBuilder<I>::operator=(const celix::ServiceRegistrationBuilder<I> &) = default;

template<typename I>
inline celix::ServiceRegistrationBuilder<I>::ServiceRegistrationBuilder(celix::ServiceRegistrationBuilder<I> &&) = default;


/**********************************************************************************************************************
  FunctionServiceRegistrationBuilder Implementation
 **********************************************************************************************************************/

template<typename F>
inline celix::FunctionServiceRegistrationBuilder<F>::FunctionServiceRegistrationBuilder(
        std::shared_ptr<celix::IResourceBundle> _owner,
        std::shared_ptr<celix::ServiceRegistry> _registry,
        std::string _functionName) : owner{std::move(_owner)}, registry{std::move(_registry)}, functionName{std::move(_functionName)} {
    //TODO static assert is function / callable}
}

template<typename F>
inline celix::FunctionServiceRegistrationBuilder<F>::FunctionServiceRegistrationBuilder(celix::FunctionServiceRegistrationBuilder<F> &&) = default;

template<typename F>
inline celix::FunctionServiceRegistrationBuilder<F> &
celix::FunctionServiceRegistrationBuilder<F>::operator=(celix::FunctionServiceRegistrationBuilder<F> &&) = default;

template<typename F>
inline celix::FunctionServiceRegistrationBuilder<F> &
celix::FunctionServiceRegistrationBuilder<F>::setFunctionService(F&& func) {
    function = std::forward<F>(func);
    return *this;
}

template<typename F>
inline celix::FunctionServiceRegistrationBuilder<F>& celix::FunctionServiceRegistrationBuilder<F>::setFunctionService(F func) {
   function = func;
   return *this;
}

//template<typename F>
//celix::FunctionServiceRegistrationBuilder<F> &
//celix::FunctionServiceRegistrationBuilder<F>::setFunctionServiceFactory(std::shared_ptr<celix::IServiceFactory<F>> factory) {
//    function = nullptr;
//    functionFactory = std::move(factory);
//    return *this;
//}

template<typename F>
inline celix::FunctionServiceRegistrationBuilder<F> &
celix::FunctionServiceRegistrationBuilder<F>::setProperties(celix::Properties props) {
    properties = props;
    return *this;
}

template<typename F>
inline celix::FunctionServiceRegistrationBuilder<F> &
celix::FunctionServiceRegistrationBuilder<F>::addProperty(const std::string &name, const std::string &value) {
    properties[name] = value;
    return *this;
}

template<typename F>
inline celix::FunctionServiceRegistrationBuilder<F> &
celix::FunctionServiceRegistrationBuilder<F>::addProperties(celix::Properties props) {
    //TODO insert with move
    properties.insert(props.begin(), props.end());
    return *this;
}

template<typename F>
inline celix::ServiceRegistration celix::FunctionServiceRegistrationBuilder<F>::build() const {
    if (valid()) {
        //TODO note using copy, make also make a buildAndClear? or make build() also clear and add a buildAndKeep()?
//        std::function<F> nFunc{};
//        celix::Properties nProps{};
//        std::swap(function, nFunc);
//        std::swap(properties, nProps);
//        return registry->registerFunctionService(functionName, std::move(nFunc), std::move(nProps), owner);
        return registry->registerFunctionService(functionName, function, properties, owner);
    } else {
        LOG(ERROR) << "Invalid builder. Builder should have a service, function service or factory service instance";
        abort();
        //TODO return invalid svc reg
    }
}

template<typename F>
inline celix::FunctionServiceRegistrationBuilder<F> celix::FunctionServiceRegistrationBuilder<F>::copy() const {
    return *this;
}

template<typename F>
inline bool celix::FunctionServiceRegistrationBuilder<F>::valid() const {
    return (!functionName.empty() && function);
}

template<typename F>
inline celix::FunctionServiceRegistrationBuilder<F>::FunctionServiceRegistrationBuilder(const celix::FunctionServiceRegistrationBuilder<F> &) = default;

template<typename F>
inline celix::FunctionServiceRegistrationBuilder<F> &
celix::FunctionServiceRegistrationBuilder<F>::operator=(const celix::FunctionServiceRegistrationBuilder<F> &) = default;
