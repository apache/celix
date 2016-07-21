/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <vector>
#include <iostream>

using namespace celix::dm;

template<class T>
CServiceDependency<T>& CServiceDependency<T>::setCService(const std::string serviceName, const std::string serviceVersion, const std::string filter) {
    const char* cversion = serviceVersion.empty() ? nullptr : serviceVersion.c_str();
    const char* cfilter = filter.empty() ? nullptr : filter.c_str();
    serviceDependency_setService(this->cServiceDependency(), serviceName.c_str(), cversion, cfilter);
    return *this;
}

template<class T>
CServiceDependency<T>& CServiceDependency<T>::setRequired(bool req) {
    serviceDependency_setRequired(this->cServiceDependency(), req);
    return *this;
}

template<class T>
CServiceDependency<T>& CServiceDependency<T>::setStrategy(DependencyUpdateStrategy strategy) {
    this->setDepStrategy(strategy);
    return *this;
}

template<class T, class I>
ServiceDependency<T,I>::ServiceDependency() : TypedServiceDependency<T>() {
    setupService();
};

template<class T, class I>
void ServiceDependency<T,I>::setupService() {
    std::string n = name.empty() ? typeName<I>() : name;
    const char* v =  version.empty() ? nullptr : version.c_str();

    //setting modified filter. which is in a filter including a lang=c++
    modifiedFilter = std::string("(lang=c++)");
    if (!filter.empty()) {
        if (strncmp(filter.c_str(), "(&", 2) == 0 && filter[filter.size()-1] == ')') {
            modifiedFilter = filter.substr(0, filter.size()-1);
            modifiedFilter = modifiedFilter.append("(lang=c++))");
        } else if (filter[0] == '(' && filter[filter.size()-1] == ')') {
            modifiedFilter = "(&";
            modifiedFilter = modifiedFilter.append(filter);
            modifiedFilter = modifiedFilter.append("(lang=c++))");
        } else {
            std::cerr << "Unexpected filter layout: '" << filter << "'\n";
        }
    }


    serviceDependency_setService(this->cServiceDependency(), n.c_str(), v, this->modifiedFilter.c_str());
}

template<class T, class I>
ServiceDependency<T,I>& ServiceDependency<T,I>::setName(std::string name) {
    this->name = name;
    setupService();
    return *this;
};

template<class T, class I>
ServiceDependency<T,I>& ServiceDependency<T,I>::setFilter(std::string filter) {
    this->filter = filter;
    setupService();
    return *this;
};

template<class T, class I>
ServiceDependency<T,I>& ServiceDependency<T,I>::setVersion(std::string version) {
    this->version = version;
    setupService();
    return *this;
};

//set callbacks
template<class T, class I>
ServiceDependency<T,I>& ServiceDependency<T,I>::setCallbacks(void (T::*set)(I* service)) {
    this->setFp = set;
    this->setupCallbacks();
    return *this;
}

template<class T, class I>
ServiceDependency<T,I>& ServiceDependency<T,I>::setCallbacks(void (T::*set)(I* service, Properties&& properties)) {
    this->setFpWithProperties = set;
    this->setupCallbacks();
    return *this;
}

//add remove callbacks
template<class T, class I>
ServiceDependency<T,I>& ServiceDependency<T,I>::setCallbacks(
        void (T::*add)(I* service),
        void (T::*remove)(I* service)) {
    this->addFp = add;
    this->removeFp = remove;
    this->setupCallbacks();
    return *this;
}

template<class T, class I>
ServiceDependency<T,I>& ServiceDependency<T,I>::setCallbacks(
        void (T::*add)(I* service, Properties&& properties),
        void (T::*remove)(I* service, Properties&& properties)
        ) {
    this->addFpWithProperties = add;
    this->removeFpWithProperties = remove;
    this->setupCallbacks();
    return *this;
}

template<class T, class I>
ServiceDependency<T,I>& ServiceDependency<T,I>::setRequired(bool req) {
    serviceDependency_setRequired(this->cServiceDependency(), req);
    return *this;
}

template<class T, class I>
ServiceDependency<T,I>& ServiceDependency<T,I>::setStrategy(DependencyUpdateStrategy strategy) {
    this->setDepStrategy(strategy);
    return *this;
};

template<class T, class I>
int ServiceDependency<T,I>::invokeCallback(void(T::*fp)(I*), const void* service) {
    T *cmp = this->componentInstance;
    I *svc = (I *) service;
    (cmp->*fp)(svc);
    return 0;
};

template<class T, class I>
int ServiceDependency<T,I>::invokeCallbackWithProperties(void(T::*fp)(I*, Properties&&), service_reference_pt  ref, const void* service) {
    service_registration_pt reg {nullptr};
    properties_pt props {nullptr};
    T *cmp = this->componentInstance;
    I *svc = (I *) service;
    serviceReference_getServiceRegistration(ref, &reg);
    serviceRegistration_getProperties(reg, &props);

    Properties properties {};
    const char* key {nullptr};
    const char* value {nullptr};

    hash_map_iterator_t iter = hashMapIterator_construct((hash_map_pt)props);
    while(hashMapIterator_hasNext(&iter)) {
        key = (const char*) hashMapIterator_nextKey(&iter);
        value = properties_get(props, key);
        //std::cout << "got property " << key << "=" << value << "\n";
        properties[key] = value;
    }

    (cmp->*fp)(svc, static_cast<Properties&&>(properties)); //explicit move of lvalue properties.
    return 0;
}

template<class T, class I>
void ServiceDependency<T,I>::setupCallbacks() {

    int(*cset)(void*,const void*) {nullptr};
    int(*cadd)(void*, const void*) {nullptr};
    int(*cremove)(void*, const void*) {nullptr};


    if (setFp != nullptr) {
        cset = [](void* handle, const void*service) -> int {
            auto dep = (ServiceDependency<T, I> *) handle;
            return dep->invokeCallback(dep->setFp, service);
        };
    }
    if (addFp != nullptr) {
        cadd = [](void* handle, const void*service) -> int {
            auto dep = (ServiceDependency<T, I> *) handle;
            return dep->invokeCallback(dep->addFp, service);
        };
    }
    if (removeFp != nullptr) {
        cremove = [](void* handle, const void*service) -> int {
            auto dep = (ServiceDependency<T, I> *) handle;
            return dep->invokeCallback(dep->removeFp, service);
        };
    }

    int(*csetWithRef)(void*, service_reference_pt, const void*) {nullptr};
    int(*caddWithRef)(void*, service_reference_pt, const void*) {nullptr};
    int(*cremoveWithRef)(void*, service_reference_pt, const void*) {nullptr};

    if (setFpWithProperties != nullptr) {
        csetWithRef = [](void* handle, service_reference_pt ref, const void* service) -> int {
            auto dep = (ServiceDependency<T,I>*) handle;
            return dep->invokeCallbackWithProperties(dep->setFpWithProperties, ref, service);
        };
    }
    if (addFpWithProperties != nullptr) {
        caddWithRef = [](void* handle, service_reference_pt ref, const void* service) -> int {
            auto dep = (ServiceDependency<T,I>*) handle;
            return dep->invokeCallbackWithProperties(dep->addFpWithProperties, ref, service);
        };
    }
    if (removeFpWithProperties != nullptr) {
        cremoveWithRef = [](void* handle, service_reference_pt ref, const void* service) -> int {
            auto dep = (ServiceDependency<T,I>*) handle;
            return dep->invokeCallbackWithProperties(dep->removeFpWithProperties, ref, service);
        };
    }

    serviceDependency_setCallbackHandle(this->cServiceDependency(), this);
    serviceDependency_setCallbacks(this->cServiceDependency(), cset, cadd, nullptr, cremove, nullptr);
    serviceDependency_setCallbacksWithServiceReference(this->cServiceDependency(), csetWithRef, caddWithRef, nullptr, cremoveWithRef, nullptr);
};
