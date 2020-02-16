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

#ifndef CELIX_SERVICEREGISTRATIONBUILDERS_H
#define CELIX_SERVICEREGISTRATIONBUILDERS_H

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
        bool isValid() const;
    private:
        ServiceRegistrationBuilder(const ServiceRegistrationBuilder<I>&);
        ServiceRegistrationBuilder<I>& operator=(const ServiceRegistrationBuilder<I>&);


        const std::shared_ptr<celix::IResourceBundle> owner;
        const std::shared_ptr<celix::ServiceRegistry> registry;

        std::shared_ptr<I> service{};
        std::shared_ptr<celix::IServiceFactory<I>> factoryService{};
        celix::Properties properties{};
    };

    template<typename F>
    class FunctionServiceRegistrationBuilder {
    public:
        FunctionServiceRegistrationBuilder(std::shared_ptr<celix::IResourceBundle> owner, std::shared_ptr<celix::ServiceRegistry> registry, const std::string& name);
        FunctionServiceRegistrationBuilder(FunctionServiceRegistrationBuilder<F> &&);
        FunctionServiceRegistrationBuilder<F>& operator=(FunctionServiceRegistrationBuilder<F>&&);

        FunctionServiceRegistrationBuilder<F>& setFunctionService(F&& function);
        FunctionServiceRegistrationBuilder<F>& setFunctionServiceFactory(std::shared_ptr<celix::IServiceFactory<F>> factory);

        FunctionServiceRegistrationBuilder<F>& setProperties(celix::Properties);
        FunctionServiceRegistrationBuilder<F>& addProperty(const std::string &name, const std::string &value);
        FunctionServiceRegistrationBuilder<F>& addProperties(celix::Properties);

        ServiceRegistration build();
        FunctionServiceRegistrationBuilder<F> copy() const;
        bool isValid() const;
    private:
        FunctionServiceRegistrationBuilder(const FunctionServiceRegistrationBuilder<F>&);
        FunctionServiceRegistrationBuilder<F>& operator=(const FunctionServiceRegistrationBuilder<F>&);


        const std::shared_ptr<celix::IResourceBundle> owner;
        const std::shared_ptr<celix::ServiceRegistry> registry;
        const std::string functionName;

        F function{};
        std::shared_ptr<celix::IServiceFactory<F>> functionFactory{};
        celix::Properties properties{};
    };

    //TODO (Function)ServiceTrackerBuilder
    //TODO (Function)UseServices
}














/**********************************************************************************************************************
  ServiceRegistrationBuilder Implementation
 **********************************************************************************************************************/


template<typename I>
celix::ServiceRegistrationBuilder<I>::ServiceRegistrationBuilder(
        std::shared_ptr<celix::IResourceBundle> _owner,
        std::shared_ptr<ServiceRegistry> _registry) : owner{std::move(_owner)}, registry{std::move(_registry)} {}


template<typename I>
bool celix::ServiceRegistrationBuilder<I>::isValid() const {
    return (service || factoryService);
}

template<typename I>
celix::ServiceRegistrationBuilder<I>& celix::ServiceRegistrationBuilder<I>::setProperties(celix::Properties props) {
    properties = std::move(props);
    return *this;
}

template<typename I>
celix::ServiceRegistrationBuilder<I> &celix::ServiceRegistrationBuilder<I>::addProperties(celix::Properties props) {
    //TODO insert with move
    properties.insert(props.begin(), props.end());
    return *this;
}


template<typename I>
celix::ServiceRegistrationBuilder<I>&
celix::ServiceRegistrationBuilder<I>::addProperty(const std::string &name, const std::string &value) {
    properties[name] = value;
    return *this;
}

template<typename I>
celix::ServiceRegistration celix::ServiceRegistrationBuilder<I>::build() {
    celix::Properties props{};
    std::swap(props, properties);
    if (factoryService) {
        return registry->registerServiceFactory<I>(factoryService, std::move(props), owner);
    } else if (service) {
        return registry->registerService<I>(service, std::move(props), owner);
    } else {
        LOG(ERROR) << "Invalid builder. Builder should have a service, function service or factory service instance";
        abort();
        //TODO return invalid svc reg
    }
}

template<typename I>
celix::ServiceRegistrationBuilder<I> celix::ServiceRegistrationBuilder<I>::copy() const {
    return *this;
}

template<typename I>
celix::ServiceRegistrationBuilder<I> &celix::ServiceRegistrationBuilder<I>::setService(std::shared_ptr<I> svc) {
    service = std::move(svc);
    return *this;
}

template<typename I>
celix::ServiceRegistrationBuilder<I> &celix::ServiceRegistrationBuilder<I>::setService(I *svc) {
    service = std::shared_ptr<I>{svc};
    return *this;
}

template<typename I>
celix::ServiceRegistrationBuilder<I> &
celix::ServiceRegistrationBuilder<I>::setServiceFactory(std::shared_ptr<celix::IServiceFactory<I>> factory) {
    factoryService = std::move(factory);
    return *this;
}

template<typename I>
celix::ServiceRegistrationBuilder<I> &celix::ServiceRegistrationBuilder<I>::setServiceFactory(celix::IServiceFactory<I> *factory) {
    factoryService = std::shared_ptr<celix::IServiceFactory<I>>{factory};
    return *this;
}

template<typename I>
celix::ServiceRegistrationBuilder<I> &
celix::ServiceRegistrationBuilder<I>::operator=(celix::ServiceRegistrationBuilder<I> &&) = default;

template<typename I>
celix::ServiceRegistrationBuilder<I>::ServiceRegistrationBuilder(const celix::ServiceRegistrationBuilder<I> &) = default;

template<typename I>
celix::ServiceRegistrationBuilder<I> &
celix::ServiceRegistrationBuilder<I>::operator=(const celix::ServiceRegistrationBuilder<I> &) = default;

template<typename I>
celix::ServiceRegistrationBuilder<I>::ServiceRegistrationBuilder(celix::ServiceRegistrationBuilder<I> &&) = default;


/**********************************************************************************************************************
  FunctionServiceRegistrationBuilder Implementation
 **********************************************************************************************************************/

template<typename F>
celix::FunctionServiceRegistrationBuilder<F>::FunctionServiceRegistrationBuilder(
        std::shared_ptr<celix::IResourceBundle> _owner,
        std::shared_ptr<celix::ServiceRegistry> _registry,
        const std::string& _name) : owner{std::move(_owner)}, registry{std::move(_registry)}, functionName{std::move(_name)} {
    //TODO static assert is function / callable}
}

template<typename F>
celix::FunctionServiceRegistrationBuilder<F>::FunctionServiceRegistrationBuilder(celix::FunctionServiceRegistrationBuilder<F> &&)  = default;

template<typename F>
celix::FunctionServiceRegistrationBuilder<F> &
celix::FunctionServiceRegistrationBuilder<F>::operator=(celix::FunctionServiceRegistrationBuilder<F> &&) = default;

template<typename F>
celix::FunctionServiceRegistrationBuilder<F> &
celix::FunctionServiceRegistrationBuilder<F>::setFunctionService(F&& func) {
    function = std::forward<F>(func);
    functionFactory = nullptr;
    return *this;
}

template<typename F>
celix::FunctionServiceRegistrationBuilder<F> &
celix::FunctionServiceRegistrationBuilder<F>::setFunctionServiceFactory(std::shared_ptr<celix::IServiceFactory<F>> factory) {
    function = nullptr;
    functionFactory = std::move(factory);
    return *this;
}

template<typename F>
celix::FunctionServiceRegistrationBuilder<F> &
celix::FunctionServiceRegistrationBuilder<F>::setProperties(celix::Properties props) {
    properties = props;
    return *this;
}

template<typename F>
celix::FunctionServiceRegistrationBuilder<F> &
celix::FunctionServiceRegistrationBuilder<F>::addProperty(const std::string &name, const std::string &value) {
    properties[name] = value;
    return *this;
}

template<typename F>
celix::FunctionServiceRegistrationBuilder<F> &
celix::FunctionServiceRegistrationBuilder<F>::addProperties(celix::Properties props) {
    //TODO insert with move
    properties.insert(props.begin(), props.end());
    return *this;
}

template<typename F>
celix::ServiceRegistration celix::FunctionServiceRegistrationBuilder<F>::build() {
    celix::Properties props{};
    std::swap(props, properties);
    F func{};
    std::swap(function, func);
    if (functionFactory) {
        return registry->registerFunctionServiceFactory<F>(functionName, functionFactory, std::move(props), owner);
    } else if (func) {
        return registry->registerFunctionService<F>(functionName, std::move(func), std::move(props), owner);
    } else {
        LOG(ERROR) << "Invalid builder. Builder should have a service, function service or factory service instance";
        abort();
        //TODO return invalid svc reg
    }
}

template<typename F>
celix::FunctionServiceRegistrationBuilder<F> celix::FunctionServiceRegistrationBuilder<F>::copy() const {
    return *this;
}

template<typename F>
bool celix::FunctionServiceRegistrationBuilder<F>::isValid() const {
    return !functionName.empty() && (function || functionFactory);
}

template<typename F>
celix::FunctionServiceRegistrationBuilder<F>::FunctionServiceRegistrationBuilder(const celix::FunctionServiceRegistrationBuilder<F> &) = default;

template<typename F>
celix::FunctionServiceRegistrationBuilder<F> &
celix::FunctionServiceRegistrationBuilder<F>::operator=(const celix::FunctionServiceRegistrationBuilder<F> &) = default;


#endif //CELIX_SERVICEREGISTRATIONBUILDERS_H