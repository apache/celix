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

#ifndef CELIX_DM_SERVICEDEPENDENCY_H
#define CELIX_DM_SERVICEDEPENDENCY_H

#include "dm_service_dependency.h"
#include "celix/dm/types.h"

#include <map>
#include <string>
#include <list>
#include <tuple>
#include <memory>
#include <iostream>

namespace celix { namespace dm {

    class DependencyManager; //forward declaration


    enum class DependencyUpdateStrategy {
        suspend,
        locking
    };

    class BaseServiceDependency {
    protected:
        dm_service_dependency_pt cServiceDep {nullptr};
    public:
        BaseServiceDependency() {
            serviceDependency_create(&this->cServiceDep);
            serviceDependency_setStrategy(this->cServiceDep, DM_SERVICE_DEPENDENCY_STRATEGY_SUSPEND); //NOTE using suspend as default strategy
        }
        virtual ~BaseServiceDependency() = default;

        dm_service_dependency_pt cServiceDependency() const { return cServiceDep; }
    };

    template<class T>
    class TypedServiceDependency :  public BaseServiceDependency {
    protected:
        T* componentInstance {nullptr};
    public:
        TypedServiceDependency() : BaseServiceDependency() {}
        virtual ~TypedServiceDependency() = default;

        void setComponentInstance(T* cmp) { componentInstance = cmp;}
    };

    template<class T>
    class CServiceDependency : public TypedServiceDependency<T> {
    public:
        CServiceDependency() : TypedServiceDependency<T>() {};
        virtual ~CServiceDependency() = default;

        CServiceDependency<T>& setCService(const std::string serviceName, const std::string serviceVersion, const std::string filter);

        CServiceDependency<T>& setRequired(bool req);
        CServiceDependency<T>& setStrategy(DependencyUpdateStrategy strategy);
    };

    template<class T, class I>
    class ServiceDependency : public TypedServiceDependency<T> {
    protected:
        std::string name {};
        std::string filter {};
        std::string version {};
        std::string modifiedFilter {};

        void (T::*setFp)(I* service) {nullptr};
        void (T::*setFpWithProperties)(I* service, Properties&& properties) {nullptr};

        void (T::*addFp)(I* service) {nullptr};
        void (T::*addFpWithProperties)(I* service, Properties&& properties) {nullptr};

        void (T::*removeFp)(I* service) {nullptr};
        void (T::*removeFpWithProperties)(I* service, Properties&& properties) {nullptr};
    private:
        void setupService();
        void setupCallbacks();
        int invokeCallback(void(T::*fp)(I*), const void* service);
        int invokeCallbackWithProperties(void(T::*fp)(I*, Properties&&), service_reference_pt  ref, const void* service);
    public:
        ServiceDependency();
        virtual ~ServiceDependency() = default;

        ServiceDependency<T,I>& setName(std::string name);

        ServiceDependency<T,I>& setFilter(std::string filter);

        ServiceDependency<T,I>& setVersion(std::string version);

        //set callbacks
        ServiceDependency<T,I>& setCallbacks(void (T::*set)(I* service));

        ServiceDependency<T,I>& setCallbacks(void (T::*set)(I* service, Properties&& properties));

        //add remove callbacks
        ServiceDependency<T,I>& setCallbacks( void (T::*add)(I* service),  void (T::*remove)(I* service));
        ServiceDependency<T,I>& setCallbacks(
                void (T::*add)(I* service, Properties&& properties),
                void (T::*remove)(I* service, Properties&& properties)
        );

        ServiceDependency<T,I>& setRequired(bool req);

        ServiceDependency<T,I>& setStrategy(DependencyUpdateStrategy strategy);
    };
}}

#include "celix/dm/ServiceDependency_Impl.h"


#endif //CELIX_DM_SERVICEDEPENDENCY_H
