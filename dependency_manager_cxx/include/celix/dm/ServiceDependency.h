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
        void setDepStrategy(DependencyUpdateStrategy strategy);
    public:
        BaseServiceDependency();
        virtual ~BaseServiceDependency() = default;

        /**
         * Returns the C DM service dependency
         */
        dm_service_dependency_pt cServiceDependency() const { return cServiceDep; }
    };

    template<class T>
    class TypedServiceDependency :  public BaseServiceDependency {
    protected:
        T* componentInstance {nullptr};
    public:
        TypedServiceDependency() : BaseServiceDependency() {}
        virtual ~TypedServiceDependency() = default;

        /**
         * Set the component instance with a pointer
         */
        void setComponentInstance(T* cmp) { componentInstance = cmp;}
    };

    template<class T>
    class CServiceDependency : public TypedServiceDependency<T> {
    public:
        CServiceDependency() : TypedServiceDependency<T>() {};
        virtual ~CServiceDependency() = default;

        /**
         * Sets the service name, version and filter for the C service dependency.
         *
         * @param serviceName The service name. Must have a value
         * @param serviceVersionRange The service version range, can be an empty string
         * @param filter The (additional) filter to use (e.g. "(location=front)")
         * @return the C service dependency reference for chaining (fluent API)
         */
        CServiceDependency<T>& setCService(const std::string serviceName, const std::string serviceVersionRange, const std::string filter);

        /**
         * Specify if the service dependency is required. Default is false
         *
         * @return the C service dependency reference for chaining (fluent API)
         */
        CServiceDependency<T>& setRequired(bool req);

        /**
         * Specify if the update strategy to use
         *
         * @return the C service dependency reference for chaining (fluent API)
         */
        CServiceDependency<T>& setStrategy(DependencyUpdateStrategy strategy);
    };

    template<class T, class I>
    class ServiceDependency : public TypedServiceDependency<T> {
    protected:
        std::string name {};
        std::string filter {};
        std::string versionRange {};
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

        /**
         * Set the service name of the service dependency.
         *
         * @return the C++ service dependency reference for chaining (fluent API)
         */
        ServiceDependency<T,I>& setName(std::string name);

        /**
         * Set the service filter of the service dependency.
         *
         * @return the C++ service dependency reference for chaining (fluent API)
         */
        ServiceDependency<T,I>& setFilter(std::string filter);

        /**
         * Set the service version range of the service dependency.
         *
         * @return the C++ service dependency reference for chaining (fluent API)
         */
        ServiceDependency<T,I>& setVersion(std::string versionRange);

        /**
         * Set the set callback for when the service dependency becomes available
         *
         * @return the C++ service dependency reference for chaining (fluent API)
         */
        ServiceDependency<T,I>& setCallbacks(void (T::*set)(I* service));

        /**
         * Set the set callback for when the service dependency becomes available
         *
         * @return the C++ service dependency reference for chaining (fluent API)
         */
        ServiceDependency<T,I>& setCallbacks(void (T::*set)(I* service, Properties&& properties));

        /**
         * Set the add and remove callback for when the services of service dependency are added or removed.
         *
         * @return the C++ service dependency reference for chaining (fluent API)
         */
        ServiceDependency<T,I>& setCallbacks( void (T::*add)(I* service),  void (T::*remove)(I* service));

        /**
         * Set the add and remove callback for when the services of service dependency are added or removed.
         *
         * @return the C++ service dependency reference for chaining (fluent API)
         */
        ServiceDependency<T,I>& setCallbacks(
                void (T::*add)(I* service, Properties&& properties),
                void (T::*remove)(I* service, Properties&& properties)
        );

        /**
         * Specify if the service dependency is required. Default is false
         *
         * @return the C service dependency reference for chaining (fluent API)
         */
        ServiceDependency<T,I>& setRequired(bool req);

        /**
         * Specify if the update strategy to use
         *
         * @return the C service dependency reference for chaining (fluent API)
         */
        ServiceDependency<T,I>& setStrategy(DependencyUpdateStrategy strategy);
    };
}}

#include "celix/dm/ServiceDependency_Impl.h"


#endif //CELIX_DM_SERVICEDEPENDENCY_H
