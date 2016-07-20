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

#ifndef CELIX_DM_ACTIVATOR_H
#define CELIX_DM_ACTIVATOR_H

#include "celix/dm/DependencyManager.h"

namespace celix { namespace dm {

    class DmActivator {
    protected:
        DependencyManager& manager;
        DmActivator(DependencyManager& mng) : manager(mng) {}
    public:
        ~DmActivator() = default;
        virtual void init(DependencyManager& manager) {};
        virtual void deinit(DependencyManager& manager) {};

        template< class T>
        Component<T>& createComponent() { return manager.createComponent<T>(); }

        template<class T>
        Component<T>& add(Component<T>& cmp) { return manager.add(cmp); }

        template<class T>
        Component<T>& remove(Component<T>& cmp) { return manager.remove(cmp); }

        template<class T, class I>
        ServiceDependency<T,I>& createServiceDependency() { return manager.createServiceDependency<T,I>(); }

        template<class T>
        CServiceDependency<T>& createCServiceDependency() { return manager.createCServiceDependency<T>(); }

        /* NOTE the following static method is intentionally not implemented
         * This should be done by the bundle activator.
         */
        static DmActivator* create(DependencyManager& mng);
    };
}}

#endif //CELIX_DM_ACTIVATOR_H
