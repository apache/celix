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


#include <utility>

#include "celix/dm/DependencyManager.h"

namespace celix { namespace dm {

    class DmActivator {
    public:
        DmActivator(DependencyManager& m);
        virtual ~DmActivator();

        DependencyManager& manager() const;
        bundle_context_pt context() const;

        /**
         * The init of the DM Activator. Should be overridden by the bundle specific DM activator.
         *
         * @param manager A reference to the  Dependency Manager
         */
        virtual void init() {};

        /**
         * The init of the DM Activator. Can be overridden by the bundle specific DM activator.
         *
         * @param manager A reference to the  Dependency Manager
         */
        virtual void deinit() {};

        /**
         * The static method to create a new DM activator.
         * NOTE that this method in intentionally not implemented in the C++ Dependency Manager library.
         * This should be done by the bundle specific DM activator.
         *
         * @param mng A reference to the Dependency Manager
         * @returns A pointer to a DmActivator. The Dependency Manager is responsible for deleting the pointer when the bundle is stopped.
         */
        static DmActivator* create(DependencyManager& mng);

    protected:
        bundle_context_pt ctx;
        DependencyManager& mng;
    };

}}

#ifndef CELIX_CREATE_BUNDLE_ACTIVATOR_SYMBOLS
#define CELIX_CREATE_BUNDLE_ACTIVATOR_SYMBOLS 1
#endif

#if CELIX_CREATE_BUNDLE_ACTIVATOR_SYMBOLS == 1
extern "C" celix_status_t bundleActivator_create(bundle_context_pt context, void **userData);
extern "C" celix_status_t bundleActivator_start(void *userData, bundle_context_pt context);
extern "C" celix_status_t bundleActivator_stop(void *userData, bundle_context_pt context);
extern "C" celix_status_t bundleActivator_destroy(void *userData, bundle_context_pt context);
#endif


#include "celix/dm/DmActivator_Impl.h"

#endif //CELIX_DM_ACTIVATOR_H
