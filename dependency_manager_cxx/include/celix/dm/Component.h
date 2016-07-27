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
#include <list>
#include <tuple>

namespace celix { namespace dm {

    class BaseComponent {
        bundle_context_pt context {nullptr};
        std::string name {};
        dm_component_pt cCmp {nullptr};
    public:
        BaseComponent(const bundle_context_pt context, std::string name);
        virtual ~BaseComponent();

        /**
         * Returns the C DM Component
         */
        const dm_component_pt cComponent() const;

        /**
         * Returns the C bundle context
         */
        const bundle_context_pt bundleContext() const;
    };
        

    template<class T>
    class Component : public BaseComponent {
        std::shared_ptr<T> instance = {nullptr};
        std::list<T> refInstance {};

        void (T::*initFp)() = {};
        void (T::*startFp)() = {};
        void (T::*stopFp)() = {};
        void (T::*deinitFp)() = {};
    public:
        Component(const bundle_context_pt context, std::string name);
        virtual ~Component();

        /**
         * Get the component instance. If no instance is explicitly set with setInstance than a instance will be create
         * using a default constructor.
         *
         * @return A reference to the component instance.
         */
        T& getInstance();

        /**
         * Set the component instance using a (shared) pointer.
         *
         * @return the DM Component reference for chaining (fluent API)
         */
        Component<T>& setInstance(std::shared_ptr<T> inst);

        /**
         * Set the component instance using rvalue reference
         * The DM Component will contain the instance.
         *
         * @return the DM Component reference for chaining (fluent API)
         */
        Component<T>& setInstance(T&& inst);


        /**
         * Adds a C++ interface to provide as service to the Celix framework.
         * The interface name will be inferred using the I template.
         *
         * @param version The version of the interface (e.g. "1.0.0"), can be an empty string
         * @return the DM Component reference for chaining (fluent API)
         */
        template<class I> Component<T>& addInterface(const std::string version);

        /**
         * Adds a C++ interface to provide as service to the Celix framework.
         * The interface name will be inferred using the I template.
         *
         * @param version The version of the interface (e.g. "1.0.0"), can be an empty string
         * @param properties To (meta) properties to provide with the service
         * @return the DM Component reference for chaining (fluent API)
         */
        template<class I> Component<T>& addInterface(const std::string version, const Properties properties);

        /**
         * Adds a C++ interface to provide as service to the Celix framework.
         *
         * @param serviceName The service name to use
         * @param version The version of the interface (e.g. "1.0.0"), can be an empty string
         * @param properties To (meta) properties to provide with the service
         * @return the DM Component reference for chaining (fluent API)
         */
        Component<T>& addInterface(const std::string serviceName, const std::string version, const Properties properties);

        /**
         * Adds a C interface to provide as service to the Celix framework.
         *
         * @param svc The service struct
         * @param serviceName The service name to use
         * @param version The version of the interface (e.g. "1.0.0"), can be an empty string
         * @param properties To (meta) properties to provide with the service
         */
        Component<T>& addCInterface(const void* svc, const std::string serviceName, const std::string version);

        /**
         * Adds a C interface to provide as service to the Celix framework.
         *
         * @param svc The service struct
         * @param serviceName The service name to use
         * @param version The version of the interface (e.g. "1.0.0"), can be an empty string
         * @param properties To (meta) properties to provide with the service
         */
        Component<T>& addCInterface(const void* svc, const std::string serviceName, const std::string version, const Properties properties);

        /**
         * Adds a C++ service dependency to the component
         *
         * @return the DM Component reference for chaining (fluent API)
         */
        template<class I>
        Component<T>& add(ServiceDependency<T,I>& dep);

        /**
         * Removes a C++ service dependency to the component
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
        Component<T>& add(CServiceDependency<T>& dep);

        /**
         * Removes a C service dependency to the component
         *
         * @return the DM Component reference for chaining (fluent API)
         */
        Component<T>& remove(CServiceDependency<T>& dep);

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
    };

}}

#include "celix/dm/Component_Impl.h"

#endif //CELIX_DM_COMPONENT_H
