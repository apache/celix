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

#ifndef CELIX_DM_COMPONENT_H
#define CELIX_DM_COMPONENT_H

#include "celix/dm/types.h"
#include "dm_component.h"

#include <map>
#include <string>
#include <vector>

namespace celix { namespace dm {

    class BaseComponent {
    private:
        bundle_context_pt context {nullptr};
        dm_component_pt cCmp {nullptr};
    public:
        BaseComponent(const bundle_context_pt con, std::string name) : context{con}, cCmp{nullptr} {
            component_create(this->context, name.c_str(), &this->cCmp);
            component_setImplementation(this->cCmp, this);
        }
        virtual ~BaseComponent() {}

        BaseComponent(const BaseComponent&) = delete;
        BaseComponent& operator=(const BaseComponent&) = delete;

        /**
         * Returns the C DM Component
         */
        dm_component_pt cComponent() const { return this->cCmp; }

        /**
         * Returns the C bundle context
         */
        bundle_context_pt bundleContext() const { return this->context; }
    };
        

    template<class T>
    class Component : public BaseComponent {
        using type = T;
    private:
        std::unique_ptr<T> instance {nullptr};
        std::shared_ptr<T> sharedInstance {nullptr};
        std::vector<T> valInstance {};
        std::vector<std::shared_ptr<BaseServiceDependency>> dependencies {};

        void (T::*initFp)() = {};
        void (T::*startFp)() = {};
        void (T::*stopFp)() = {};
        void (T::*deinitFp)() = {};

        int (T::*initFpNoExc)() = {};
        int (T::*startFpNoExc)() = {};
        int (T::*stopFpNoExc)() = {};
        int (T::*deinitFpNoExc)() = {};
    public:
        Component(const bundle_context_pt context, std::string name);
        virtual ~Component();

        /**
         * Creates a Component using the provided bundle context
         * and component name.
         * Will use new(nothrow) if exceptions are disabled.
         * @return newly created DM Component or nullptr
         */
        static Component<T>* create(bundle_context_pt, std::string name);

        /**
         * Creates a Component using the provided bundle context.
         * Will use new(nothrow) if exceptions are disabled.
         * @return newly created DM Component or nullptr
         */
        static Component<T>* create(bundle_context_pt);

        /**
         * Wether the component is valid. Invalid component can occurs when no new components can be created and
         * exceptions are not allowed.
         * @return
         */
        bool isValid() const;

        /**
         * Get the component instance. If no instance is explicitly set with setInstance than a instance will be create
         * using a default constructor.
         *
         * @return A reference to the component instance.
         */
        T& getInstance();

        /**
         * Set the component instance using a shared pointer.
         *
         * @return the DM Component reference for chaining (fluent API)
         */
        Component<T>& setInstance(std::shared_ptr<T> inst);

        /**
         * Set the component instance using a unique pointer.
         *
         * @return the DM Component reference for chaining (fluent API)
         */
        Component<T>& setInstance(std::unique_ptr<T>&& inst);

        /**
         * Set the component instance using a value or rval reference
         * The DM Component will contain the instance.
         *
         * @return the DM Component reference for chaining (fluent API)
         */
        Component<T>& setInstance(T inst);

        /**
         * Adds a C++ interface to provide as service to the Celix framework.
         *
         * @param serviceName The service name to use
         * @param version The version of the interface (e.g. "1.0.0"), can be an empty string
         * @param properties To (meta) properties to provide with the service
         * @return the DM Component reference for chaining (fluent API)
         */
        template<class I> Component<T>& addInterfaceWithName(const std::string serviceName, const std::string version = std::string{}, const Properties properties = Properties{});

        /**
         * Adds a C++ interface to provide as service to the Celix framework.
         *
         * @param serviceName The service name to use
         * @param version The version of the interface (e.g. "1.0.0"), can be an empty string
         * @param properties To (meta) properties to provide with the service
         * @return the DM Component reference for chaining (fluent API)
         */
        template<class I> Component<T>& addInterface(const std::string version = std::string{}, const Properties properties = Properties{});

        /**
         * Adds a C interface to provide as service to the Celix framework.
         *
         * @param svc The service struct
         * @param serviceName The service name to use
         * @param version The version of the interface (e.g. "1.0.0"), can be an empty string
         * @param properties To (meta) properties to provide with the service
         */
        template<class I> Component<T>& addCInterface(const I* svc, const std::string serviceName, const std::string version = std::string{}, const Properties properties = Properties{});


        /**
         * Creates and adds a C++ service dependency to the component
         *
         * @return the Service Dependency reference for chaining (fluent API)
         */
        template<class I>
        ServiceDependency<T,I>& createServiceDependency(const std::string name = std::string{});

        /**
         Removes a C++ service dependency from the component
         *
         * @return the DM Component reference for chaining (fluent API)
         */
        template<class I>
        Component<T>& remove(ServiceDependency<T,I>& dep);

        /**
         * Adds a C service dependency to the component
         *
         * @return the DM Component reference for chaining (fluent API)
         */
        template<typename I>
        CServiceDependency<T,I>& createCServiceDependency(const std::string name);

        /**
         * Removes a C service dependency to the component
         *
         * @return the DM Component reference for chaining (fluent API)
         */
        template<typename I>
        Component<T>& remove(CServiceDependency<T,I>& dep);

        /**
         * Set the callback for the component life cycle control
         *
         * @param init The init callback.
         * @param start The start callback.
         * @param stop The stop callback.
         * @param deinit The deinit callback.
         *
         * @return the DM Component reference for chaining (fluent API)
         */
        Component<T>& setCallbacks(
            void (T::*init)(),
            void (T::*start)(),
            void (T::*stop)(),
            void (T::*deinit)()
        );

        /**
         * Set the callback for the component life cycle control
         * with a int return to indicate an error.
         *
         * @param init The init callback.
         * @param start The start callback.
         * @param stop The stop callback.
         * @param deinit The deinit callback.
         *
         * @return the DM Component reference for chaining (fluent API)
         */
        Component<T>& setCallbacks(
            int (T::*init)(),
            int (T::*start)(),
            int (T::*stop)(),
            int (T::*deinit)()
        );
    };
}}

#include "celix/dm/Component_Impl.h"

#endif //CELIX_DM_COMPONENT_H
