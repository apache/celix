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

#include <map>
#include <string>
#include <list>
#include <tuple>
#include <memory>
#include <iostream>
#include <functional>
#include <atomic>
#include <vector>
#include <cstring>
#include <chrono>

#include "celix_dm_service_dependency.h"
#include "celix_constants.h"
#include "celix_properties.h"
#include "celix/Utils.h"
#include "celix/dm/Properties.h"

namespace celix { namespace dm {

    enum class DependencyUpdateStrategy {
        suspend,
        locking
    };

    class BaseServiceDependency {
    private:
        const std::chrono::milliseconds warningTimoutForNonExpiredSvcObject{5000};
        std::atomic<bool> depAddedToCmp{false};
        celix_dm_component_t* cCmp;
    protected:
        celix_dm_service_dependency_t *cServiceDep {nullptr};

        void setDepStrategy(DependencyUpdateStrategy strategy) {
            if (strategy == DependencyUpdateStrategy::locking) {
                celix_dmServiceDependency_setStrategy(this->cServiceDependency(), DM_SERVICE_DEPENDENCY_STRATEGY_LOCKING);
            } else { /*suspend*/
                celix_dmServiceDependency_setStrategy(this->cServiceDependency(), DM_SERVICE_DEPENDENCY_STRATEGY_SUSPEND);
            }
        }

        template<typename U>
        void waitForExpired(std::weak_ptr<U> observe, long svcId, const char* observeType);
    public:
        BaseServiceDependency(celix_dm_component_t* c)  : cCmp{c} {
            this->cServiceDep = celix_dmServiceDependency_create();
            //NOTE using suspend as default strategy
            celix_dmServiceDependency_setStrategy(this->cServiceDep,  DM_SERVICE_DEPENDENCY_STRATEGY_SUSPEND);
        }

        virtual ~BaseServiceDependency() noexcept;

        BaseServiceDependency(const BaseServiceDependency&) = delete;
        BaseServiceDependency& operator=(const BaseServiceDependency&) = delete;
        BaseServiceDependency(BaseServiceDependency&&) noexcept = delete;
        BaseServiceDependency& operator=(BaseServiceDependency&&) noexcept = delete;

        /**
         * Whether the service dependency is valid.
         *
         * Deprecated -> will always return true.
         */
        bool isValid() const __attribute__((deprecated)) { return true; }

        /**
         * Returns the C DM service dependency
         */
        celix_dm_service_dependency_t *cServiceDependency() const { return cServiceDep; }

        /**
         * Wait for an empty Celix event queue.
         * Should not be called on the Celix event queue thread.
         *
         * Can be used to ensure that the service dependency is completely processed (service trackers are created).
         */
        void wait() const;

        /**
         * Run the service dependency build. After this call the service dependency is added to the component and
         * is enabled.
         * The underlining service tracker will be created async.
         */
        void runBuild();
    };

    template<class T>
    class TypedServiceDependency :  public BaseServiceDependency {
        using cmpType = T;
    protected:
        T* componentInstance {nullptr};
    public:
        TypedServiceDependency(celix_dm_component_t* cCmp) : BaseServiceDependency(cCmp) {}
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

    /**
     * @brief A service dependency for a component.
     * @tparam T The component type
     * @tparam I The service interface type
     * @warning CServiceDependency is considered deprecated use celix::dm::ServiceDependency instead.
     */
    template<class T, typename I>
    class CServiceDependency : public TypedServiceDependency<T> {
        using type = I;
    public:
        CServiceDependency(celix_dm_component_t* cCmp, const std::string &name);
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
         * "Build" the service dependency.
         * When build the service dependency is active and the service tracker is created.
         *
         * Should not be called on the Celix event thread.
         */
        CServiceDependency<T,I>& build();

        /**
         * Same a build, but will not wait till the underlining service tracker is created.
         * Can be called on the Celix event thread.
         */
        CServiceDependency<T,I>& buildAsync();



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

    /**
     * @brief A service dependency for a component.
     * @tparam T The component type
     * @tparam I The service interface type
     */
    template<class T, class I>
    class ServiceDependency : public TypedServiceDependency<T> {
        using type = I;
    public:
        ServiceDependency(celix_dm_component_t* cCmp, const std::string &name);
        ~ServiceDependency() override = default;

        ServiceDependency(const ServiceDependency&) = delete;
        ServiceDependency& operator=(const ServiceDependency&) = delete;
        ServiceDependency(ServiceDependency&&) noexcept = delete;
        ServiceDependency& operator=(ServiceDependency&&) noexcept = delete;

        /**
         * Set the service name of the service dependency.
         *
         * @return the service dependency reference for chaining (fluent API)
         */
        ServiceDependency<T,I>& setName(const std::string &_name);

        /**
         * Set the service filter of the service dependency.
         *
         * @return the service dependency reference for chaining (fluent API)
         */
        ServiceDependency<T,I>& setFilter(const std::string &filter);

        /**
         * Set the service version range of the service dependency.
         *
         * @return the service dependency reference for chaining (fluent API)
         */
        ServiceDependency<T,I>& setVersionRange(const std::string &versionRange);

        /**
         * Set the set callback for when the service dependency becomes available
         *
         * @return the service dependency reference for chaining (fluent API)
         */
        ServiceDependency<T,I>& setCallbacks(void (T::*set)(I* service));

        /**
         * Set the set callback for when the service dependency becomes available
         *
         * @return the service dependency reference for chaining (fluent API)
         */
        ServiceDependency<T,I>& setCallbacks(void (T::*set)(I* service, Properties&& properties));

        /**
         * Set the set callback for when the service dependency becomes available
         *
         * @return the service dependency reference for chaining (fluent API)
         */
        ServiceDependency<T,I>& setCallbacks(std::function<void(I* service, Properties&& properties)> set);

        /**
         * @brief Set the set callback for when the service dependency becomes available using shared pointers
         *
         * @return the service dependency reference for chaining (fluent API)
         */
        ServiceDependency<T,I>& setCallbacks(void (T::*set)(const std::shared_ptr<I>& service, const std::shared_ptr<const celix::Properties>& properties));

        /**
         * @brief Set the set callback for when the service dependency becomes available using shared pointers.
         *
         * @return the service dependency reference for chaining (fluent API)
         */
        ServiceDependency<T,I>& setCallbacks(std::function<void(const std::shared_ptr<I>& service, const std::shared_ptr<const celix::Properties>& properties)> set);

        /**
         * @brief Set the set callback for when the service dependency becomes available using shared pointers
         *
         * @return the service dependency reference for chaining (fluent API)
         */
        ServiceDependency<T,I>& setCallbacks(void (T::*set)(const std::shared_ptr<I>& service));

        /**
         * @brief Set the set callback for when the service dependency becomes available using shared pointers.
         *
         * @return the service dependency reference for chaining (fluent API)
         */
        ServiceDependency<T,I>& setCallbacks(std::function<void(const std::shared_ptr<I>& service)> set);

        /**
         * Set the add and remove callback for when the services of service dependency are added or removed.
         *
         * @return the service dependency reference for chaining (fluent API)
         */
        ServiceDependency<T,I>& setCallbacks(void (T::*add)(I* service),  void (T::*remove)(I* service));

        /**
         * Set the add and remove callback for when the services of service dependency are added or removed.
         *
         * @return the service dependency reference for chaining (fluent API)
         */
        ServiceDependency<T,I>& setCallbacks(
                void (T::*add)(I* service, Properties&& properties),
                void (T::*remove)(I* service, Properties&& properties)
        );

        /**
         * Set the add and remove callback for when the services of service dependency are added or removed.
         *
         * @return the service dependency reference for chaining (fluent API)
         */
        ServiceDependency<T,I>& setCallbacks(
                std::function<void(I* service, Properties&& properties)> add,
                std::function<void(I* service, Properties&& properties)> remove
        );

        /**
         * @brief Set the add and remove callback for a service dependency using shared pointers.
         *
         * @return the service dependency reference for chaining (fluent API)
         */
        ServiceDependency<T,I>& setCallbacks(
                void (T::*add)(const std::shared_ptr<I>& service, const std::shared_ptr<const celix::Properties>& properties),
                void (T::*remove)(const std::shared_ptr<I>& service, const std::shared_ptr<const celix::Properties>& properties));

        /**
         * @brief Set the add and remove callback for a service dependency using shared pointers.
         *
         * @return the service dependency reference for chaining (fluent API)
         */
        ServiceDependency<T,I>& setCallbacks(
                std::function<void(const std::shared_ptr<I>& service, const std::shared_ptr<const celix::Properties>& properties)> add,
                std::function<void(const std::shared_ptr<I>& service, const std::shared_ptr<const celix::Properties>& properties)> remove);


        /**
         * @brief Set the add and remove callback for a service dependency using shared pointers.
         *
         * @return the service dependency reference for chaining (fluent API)
         */
        ServiceDependency<T,I>& setCallbacks(
                void (T::*add)(const std::shared_ptr<I>& service),
                void (T::*remove)(const std::shared_ptr<I>& service));

        /**
         * @brief Set the add and remove callback for a service dependency using shared pointers.
         *
         * @return the service dependency reference for chaining (fluent API)
         */
        ServiceDependency<T,I>& setCallbacks(
                std::function<void(const std::shared_ptr<I>& service)> add,
                std::function<void(const std::shared_ptr<I>& service)> remove);

        /**
         * Specify if the service dependency is required. Default is false
         *
         * @return the service dependency reference for chaining (fluent API)
         */
        ServiceDependency<T,I>& setRequired(bool req);

        /**
         * Specify if the update strategy to use
         *
         * @return the service dependency reference for chaining (fluent API)
         */
        ServiceDependency<T,I>& setStrategy(DependencyUpdateStrategy strategy);

        /**
         * "Build" the service dependency.
         * When build the service dependency is active and the service tracker is created.
         *
         * Should not be called on the Celix event thread.
         *
         * @return the service dependency reference for chaining (fluent API)
         */
        ServiceDependency<T,I>& build();

        /**
         * Same a build, but will not wait till the underlining service trackers are opened.
         * Can be called on the Celix event thread.
         *
         * @return the service dependency reference for chaining (fluent API)
         */
        ServiceDependency<T,I>& buildAsync();
    private:
        void setupService();
        void setupCallbacks();
        int invokeCallback(std::function<void(I*, Properties&&)> fp, const celix_properties_t *props, const void* service);

        std::string name {};
        std::string filter {};
        std::string versionRange {};

        std::function<void(I* service, Properties&& properties)> setFp{};
        std::function<void(I* service, Properties&& properties)> addFp{};
        std::function<void(I* service, Properties&& properties)> removeFp{};

        std::function<void(const std::shared_ptr<I>& service, const std::shared_ptr<const celix::Properties>& properties)> setFpUsingSharedPtr{};
        std::function<void(const std::shared_ptr<I>& service, const std::shared_ptr<const celix::Properties>& properties)> addFpUsingSharedPtr{};
        std::function<void(const std::shared_ptr<I>& service, const std::shared_ptr<const celix::Properties>& properties)> removeFpUsingSharedPtr{};

        //note below is only updated in the Celix event thread.
        std::unordered_map<long, std::pair<std::shared_ptr<I>, std::shared_ptr<const celix::Properties>>> addedServices{};
        std::pair<std::shared_ptr<I>, std::shared_ptr<const celix::Properties>> setService{};
    };
}}

#include "celix/dm/ServiceDependency_Impl.h"
