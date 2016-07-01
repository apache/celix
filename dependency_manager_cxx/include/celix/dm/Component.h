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
        std::string name {};
        bundle_context_pt context {nullptr};
        dm_component_pt cCmp {nullptr};
    public:
        BaseComponent(const bundle_context_pt context, std::string name);
        virtual ~BaseComponent();

        const dm_component_pt cComponent() const;
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

        T& getInstance();

        Component<T>& setInstance(std::shared_ptr<T> inst);

        Component<T>& setInstance(T&& inst);

        //provided interfaces
        template<class I> Component<T>& addInterface(const std::string version);
        template<class I> Component<T>& addInterface(const std::string version, const Properties properties);
        Component<T>& addInterface(const std::string intfName, const std::string version, const Properties properties);
        Component<T>& addCInterface(const void* svc, const std::string serviceName, const std::string version, const Properties properties);

        //dependencies
        template<class I>
        Component<T>& add(ServiceDependency<T,I>& dep);
        template<class I>
        Component<T>& remove(ServiceDependency<T,I>& dep);

        Component<T>& add(CServiceDependency<T>& dep);
        Component<T>& remove(CServiceDependency<T>& dep);

        //callbacks
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
