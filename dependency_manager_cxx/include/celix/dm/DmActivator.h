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
#include "bundle_activator.h"

namespace celix { namespace dm {

    class DmActivator {
    public:
        DmActivator(DependencyManager& m) : mng(m), ctx{m.bundleContext()}  {}
        virtual ~DmActivator() = default;

        DmActivator(const DmActivator&) = delete;
        DmActivator& operator=(const DmActivator&) = delete;

        DependencyManager& manager() const { return this->mng; }

        bundle_context_pt context() const { return this->ctx; }

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
        DependencyManager& mng;
        bundle_context_pt ctx;
    private:
        dm_info_service_t info{};
        service_registration_pt reg{nullptr};

        int start() {
            celix_status_t status = CELIX_SUCCESS;
            this->init();
            this->mng.start();

            //Create and register the dm info service
            this->info.handle = this->mng.cDependencyManager();
            this->info.getInfo = (celix_status_t (*)(void *, dm_dependency_manager_info_pt *)) dependencyManager_getInfo;
            this->info.destroyInfo = (void (*)(void *, dm_dependency_manager_info_pt)) dependencyManager_destroyInfo;
            status = bundleContext_registerService(this->ctx, (char *) DM_INFO_SERVICE_NAME, &this->info, NULL, &(this->reg));

            return status;
        }

        int stop() {
            celix_status_t status = CELIX_SUCCESS;

            this->deinit();

            // Remove the service
            if (this->reg != nullptr) {
                status = serviceRegistration_unregister(this->reg);
                this->reg = nullptr;
            }
            // Remove all components
            dependencyManager_removeAllComponents(this->mng.cDependencyManager());

            return status;
        }

        friend int ::bundleActivator_create(::bundle_context_pt, void**);
        friend int ::bundleActivator_start(void*, ::bundle_context_pt);
        friend int ::bundleActivator_stop(void*, ::bundle_context_pt);
        friend int ::bundleActivator_destroy(void*, ::bundle_context_pt);
    };
}}

#endif //CELIX_DM_ACTIVATOR_H
