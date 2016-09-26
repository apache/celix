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

/**
 * TODO add a dependency for a whiteboard pattern or marker service. e.g. a service where the type is irrelevant.
 */

namespace celix { namespace dm {

    class DependencyManager; //forward declaration

    enum class DependencyUpdateStrategy {
        suspend,
        locking
    };

    class BaseServiceDependency {
    protected:
        const bool valid;
        dm_service_dependency_pt cServiceDep {nullptr};

        void setDepStrategy(DependencyUpdateStrategy strategy);
    public:
        BaseServiceDependency(bool valid);

        virtual ~BaseServiceDependency() = default;

        /**
         * Wether the service dependency is valid.
         */
        bool isValid() const { return valid; }

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
        TypedServiceDependency(bool valid) : BaseServiceDependency(valid) {}
        virtual ~TypedServiceDependency() = default;

        /**
         * Set the component instance with a pointer
         */
        void setComponentInstance(T* cmp) { componentInstance = cmp;}
    };

    template<class T, typename I>
    class CServiceDependency : public TypedServiceDependency<T> {
    private:
        std::string name {};
        std::string filter {};
        std::string versionRange {};

        void (T::*setFp)(const I* service) {nullptr};
        void (T::*setFpWithProperties)(const I* service, Properties&& properties) {nullptr};

        void (T::*addFp)(const I* service) {nullptr};
        void (T::*addFpWithProperties)(const I* service, Properties&& properties) {nullptr};

        void (T::*removeFp)(const I* service) {nullptr};
        void (T::*removeFpWithProperties)(const I* service, Properties&& properties) {nullptr};

        void setupCallbacks();
        int invokeCallback(void(T::*fp)(const I*), const void* service);
        int invokeCallbackWithProperties(void(T::*fp)(const I*, Properties&&), service_reference_pt  ref, const void* service);

        void setupService();
    public:
        CServiceDependency(bool valid = true) : TypedServiceDependency<T>(valid) {};
        virtual ~CServiceDependency() = default;

        /**
         * Sets the service name, version and filter for the C service dependency.
         *
         * @param serviceName The service name. Must have a value
         * @param serviceVersionRange The service version range, can be an empty string
         * @param filter The (additional) filter to use (e.g. "(location=front)")
         * @return the C service dependency reference for chaining (fluent API)
         */
        CServiceDependency<T,I>& setCService(const std::string serviceName, const std::string serviceVersionRange = std::string{}, const std::string filter = std::string{});

        /**
         * Specify if the service dependency is required. Default is false
         *
         * @return the C service dependency reference for chaining (fluent API)
         */
        CServiceDependency<T,I>& setRequired(bool req);

        /**
         * Specify if the update strategy to use
         *
         * @return the C service dependency reference for chaining (fluent API)
         */
        CServiceDependency<T,I>& setStrategy(DependencyUpdateStrategy strategy);

        /**
         * Set the set callback for when the service dependency becomes available
         *
         * @return the C service dependency reference for chaining (fluent API)
         */
        CServiceDependency<T,I>& setCallbacks(void (T::*set)(const I* service));

        /**
         * Set the set callback for when the service dependency becomes available
         *
         * @return the C service dependency reference for chaining (fluent API)
         */
        CServiceDependency<T,I>& setCallbacks(void (T::*set)(const I* service, Properties&& properties));

        /**
         * Set the add and remove callback for when the services of service dependency are added or removed.
         *
         * @return the C service dependency reference for chaining (fluent API)
         */
        CServiceDependency<T,I>& setCallbacks( void (T::*add)(const I* service),  void (T::*remove)(const I* service));

        /**
         * Set the add and remove callback for when the services of service dependency are added or removed.
         *
         * @return the C service dependency reference for chaining (fluent API)
         */
        CServiceDependency<T,I>& setCallbacks(
                void (T::*add)(const I* service, Properties&& properties),
                void (T::*remove)(const I* service, Properties&& properties)
        );

        /**
         * Specify if the service dependency should add a service.lang filter part if it is not already present
         * For C service dependencies 'service.lang=C' will be added.
         */
        CServiceDependency<T,I>& setAddLanguageFilter(bool addLang);
    };

    template<class T, class I>
    class ServiceDependency : public TypedServiceDependency<T> {
    private:
        bool addCxxLanguageFilter {true};
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

        void setupService();
        void setupCallbacks();
        int invokeCallback(void(T::*fp)(I*), const void* service);
        int invokeCallbackWithProperties(void(T::*fp)(I*, Properties&&), service_reference_pt  ref, const void* service);
    public:
        ServiceDependency(bool valid = true);
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

        /**
         * Specify if the service dependency should add a service.lang filter part if it is not already present
         * For C++ service dependencies 'service.lang=C++' will be added.
         * Should be called before
         */
        ServiceDependency<T,I>& setAddLanguageFilter(bool addLang);
    };
}}

#include "celix/dm/ServiceDependency_Impl.h"


#endif //CELIX_DM_SERVICEDEPENDENCY_H
