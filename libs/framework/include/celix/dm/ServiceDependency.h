/*
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

#pragma once

#include "dm_service_dependency.h"
#include "celix/dm/types.h"

#include <map>
#include <string>
#include <list>
#include <tuple>
#include <memory>
#include <iostream>
#include <functional>
#include <atomic>

namespace celix { namespace dm {

    class DependencyManager; //forward declaration

    enum class DependencyUpdateStrategy {
        suspend,
        locking
    };

    class BaseServiceDependency {
    private:
        celix_dm_component_t* cCmp;
        bool valid;
        std::atomic<bool> depAddedToCmp{false};
    protected:
        celix_dm_service_dependency_t *cServiceDep {nullptr};

        void setDepStrategy(DependencyUpdateStrategy strategy) {
            if (!valid) {
                return;
            }
            if (strategy == DependencyUpdateStrategy::locking) {
                celix_dmServiceDependency_setStrategy(this->cServiceDependency(), DM_SERVICE_DEPENDENCY_STRATEGY_LOCKING);
            } else if (strategy == DependencyUpdateStrategy::suspend) {
                celix_dmServiceDependency_setStrategy(this->cServiceDependency(), DM_SERVICE_DEPENDENCY_STRATEGY_SUSPEND);
            } else {
                std::cerr << "Unexpected dependency update strategy. Cannot convert for dm_dependency\n";
            }
        }
    public:
        BaseServiceDependency(celix_dm_component_t* c, bool v)  : cCmp{c}, valid{v} {
            if (this->valid) {
                this->cServiceDep = celix_dmServiceDependency_create();
                //NOTE using suspend as default strategy
                celix_dmServiceDependency_setStrategy(this->cServiceDep,  DM_SERVICE_DEPENDENCY_STRATEGY_SUSPEND);
            }
        }

        virtual ~BaseServiceDependency() noexcept;

        BaseServiceDependency(const BaseServiceDependency&) = delete;
        BaseServiceDependency& operator=(const BaseServiceDependency&) = delete;
        BaseServiceDependency(BaseServiceDependency&&) noexcept = delete;
        BaseServiceDependency& operator=(BaseServiceDependency&&) noexcept = delete;

        /**
         * Whether the service dependency is valid.
         */
        bool isValid() const { return valid; }

        /**
         * Returns the C DM service dependency
         */
        celix_dm_service_dependency_t *cServiceDependency() const { return cServiceDep; }

        void runBuild();
    };

    template<class T>
    class TypedServiceDependency :  public BaseServiceDependency {
        using cmpType = T;
    protected:
        T* componentInstance {nullptr};
    public:
        TypedServiceDependency(celix_dm_component_t* cCmp, bool valid) : BaseServiceDependency(cCmp, valid) {}
        ~TypedServiceDependency() override = default;

        TypedServiceDependency(const TypedServiceDependency&) = delete;
        TypedServiceDependency& operator=(const TypedServiceDependency&) = delete;
        TypedServiceDependency(TypedServiceDependency&&) noexcept = delete;
        TypedServiceDependency& operator=(TypedServiceDependency&&) noexcept = delete;

        /**
         * Set the component instance with a pointer
         */
        void setComponentInstance(T* cmp) { componentInstance = cmp;}
    };

    template<class T, typename I>
    class CServiceDependency : public TypedServiceDependency<T> {
        using type = I;
    public:
        CServiceDependency(celix_dm_component_t* cCmp, const std::string &name, bool valid = true);
        ~CServiceDependency() override = default;

        CServiceDependency(const CServiceDependency&) = delete;
        CServiceDependency& operator=(const CServiceDependency&) = delete;
        CServiceDependency(CServiceDependency&&) noexcept = delete;
        CServiceDependency& operator=(CServiceDependency&&) noexcept = delete;

        /**
         * Sets the service version range for the C service dependency.
         *
         * @param serviceVersionRange The service version range, can be an empty string
         * @return the C service dependency reference for chaining (fluent API)
         */
        CServiceDependency<T,I>& setVersionRange(const std::string &serviceVersionRange);

        /**
         * Sets the service filter for the C service dependency.
         *
         * @param filter The (additional) filter to use (e.g. "(location=front)")
         * @return the C service dependency reference for chaining (fluent API)
         */
        CServiceDependency<T,I>& setFilter(const std::string &filter);

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
         * Set the set callback for when the service dependency becomes available
         *
         * @return the C service dependency reference for chaining (fluent API)
         */
        CServiceDependency<T,I>& setCallbacks(std::function<void(const I* service, Properties&& properties)> set);

        /**
         * Set the add and remove callback for when the services of service dependency are added or removed.
         *
         * @return the C service dependency reference for chaining (fluent API)
         */
        CServiceDependency<T,I>& setCallbacks(void (T::*add)(const I* service),  void (T::*remove)(const I* service));

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
         * Set the add and remove callback for when the services of service dependency are added or removed.
         *
         * @return the C service dependency reference for chaining (fluent API)
         */
        CServiceDependency<T,I>& setCallbacks(
                std::function<void(const I* service, Properties&& properties)> add,
                std::function<void(const I* service, Properties&& properties)> remove
        );

        /**
         * Specify if the service dependency should add a service.lang filter part if it is not already present
         * For C service dependencies 'service.lang=C' will be added.
         */
        CServiceDependency<T,I>& setAddLanguageFilter(bool addLang);

        /**
         * "Build" the service dependency.
         * A service dependency added to an active component will only become active if the build is called
         * @return
         */
        CServiceDependency<T,I>& build();
    private:
        std::string name {};
        std::string filter {};
        std::string versionRange {};

        std::function<void(const I* service, Properties&& properties)> setFp{nullptr};
        std::function<void(const I* service, Properties&& properties)> addFp{nullptr};
        std::function<void(const I* service, Properties&& properties)> removeFp{nullptr};

        void setupCallbacks();
        int invokeCallback(std::function<void(const I*, Properties&&)> fp, const celix_properties_t *props, const void* service);

        void setupService();
    };

    template<class T, class I>
    class ServiceDependency : public TypedServiceDependency<T> {
        using type = I;
    public:
        ServiceDependency(celix_dm_component_t* cCmp, const std::string &name, bool valid = true);
        ~ServiceDependency() override = default;

        ServiceDependency(const ServiceDependency&) = delete;
        ServiceDependency& operator=(const ServiceDependency&) = delete;
        ServiceDependency(ServiceDependency&&) noexcept = delete;
        ServiceDependency& operator=(ServiceDependency&&) noexcept = delete;

        /**
         * Set the service name of the service dependency.
         *
         * @return the C++ service dependency reference for chaining (fluent API)
         */
        ServiceDependency<T,I>& setName(const std::string &_name);

        /**
         * Set the service filter of the service dependency.
         *
         * @return the C++ service dependency reference for chaining (fluent API)
         */
        ServiceDependency<T,I>& setFilter(const std::string &filter);

        /**
         * Set the service version range of the service dependency.
         *
         * @return the C++ service dependency reference for chaining (fluent API)
         */
        ServiceDependency<T,I>& setVersionRange(const std::string &versionRange);

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
         * Set the set callback for when the service dependency becomes available
         *
         * @return the C service dependency reference for chaining (fluent API)
         */
        ServiceDependency<T,I>& setCallbacks(std::function<void(I* service, Properties&& properties)> set);

        /**
         * Set the add and remove callback for when the services of service dependency are added or removed.
         *
         * @return the C++ service dependency reference for chaining (fluent API)
         */
        ServiceDependency<T,I>& setCallbacks(void (T::*add)(I* service),  void (T::*remove)(I* service));

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
         * Set the add and remove callback for when the services of service dependency are added or removed.
         *
         * @return the C service dependency reference for chaining (fluent API)
         */
        ServiceDependency<T,I>& setCallbacks(
                std::function<void(I* service, Properties&& properties)> add,
                std::function<void(I* service, Properties&& properties)> remove
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


        /**
         * Build the service dependency.
         *
         * When building the service dependency will make will enabled service dependency.
         * If this is done on a already build component, this will result in an additional service dependency for the
         * component.
         */
        ServiceDependency<T,I>& build();
    private:
        bool addCxxLanguageFilter {true};
        std::string name {};
        std::string filter {};
        std::string versionRange {};
        std::string modifiedFilter {};

        std::function<void(I* service, Properties&& properties)> setFp{nullptr};
        std::function<void(I* service, Properties&& properties)> addFp{nullptr};
        std::function<void(I* service, Properties&& properties)> removeFp{nullptr};

        void setupService();
        void setupCallbacks();
        int invokeCallback(std::function<void(I*, Properties&&)> fp, const celix_properties_t *props, const void* service);
    };
}}

#include "celix/dm/ServiceDependency_Impl.h"
