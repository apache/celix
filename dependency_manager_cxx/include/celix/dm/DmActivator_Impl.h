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

using namespace celix::dm;

DmActivator::DmActivator(DependencyManager& m) : ctx{m.bundleContext()}, mng{m} {}

DmActivator::~DmActivator() = default;

DependencyManager& DmActivator::manager() const {
    return this->mng;
}

bundle_context_pt DmActivator::context() const {
    return this->ctx;
}

namespace celix { namespace dm { namespace /*anon*/ {
    class DmActivatorImpl {
    private:
        DependencyManager mng;
        bundle_context_pt ctx;
        DmActivator *act;
        service_registration_pt reg{nullptr};
        dm_info_service_t info{nullptr, nullptr, nullptr};

    public:
        DmActivatorImpl(bundle_context_pt c) : mng{c}, ctx{c}, act{DmActivator::create(mng)} { }
        ~DmActivatorImpl() { delete act; }

        DmActivatorImpl(DmActivatorImpl&&) = delete;
        DmActivatorImpl& operator=(DmActivatorImpl&&) = delete;

        DmActivatorImpl(const DmActivatorImpl&) = delete;
        DmActivator& operator=(const DmActivatorImpl&) = delete;

        celix_status_t start() {
            celix_status_t status = CELIX_SUCCESS;

            this->act->init();
            this->mng.start();

            //Create and register the dm info service
            this->info.handle = this->mng.cDependencyManager();
            this->info.getInfo = (celix_status_t (*)(void *,
                                                     dm_dependency_manager_info_pt *)) dependencyManager_getInfo;
            this->info.destroyInfo = (void (*)(void *, dm_dependency_manager_info_pt)) dependencyManager_destroyInfo;
            status = bundleContext_registerService(this->ctx, (char *) DM_INFO_SERVICE_NAME, &this->info, NULL,
                                                   &(this->reg));

            return status;
        }

        celix_status_t stop() {
            celix_status_t status = CELIX_SUCCESS;

            // Remove the service
            if (this->reg != nullptr) {
                status = serviceRegistration_unregister(this->reg);
                this->reg = nullptr;
            }
            // Remove all components
            dependencyManager_removeAllComponents(this->mng.cDependencyManager());

            return status;
        }
    };
}}}


celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
    int status = CELIX_SUCCESS;
    DmActivatorImpl* impl = nullptr;
#ifdef __EXCEPTIONS
    impl = new DmActivatorImpl(context);
#else
    impl = new(std::nothrow) DmActivatorImpl(context);
#endif

    if (impl == nullptr) {
        status = CELIX_ENOMEM;
    } else {
        *userData = impl;
    }
    return status;
}

#if CELIX_CREATE_BUNDLE_ACTIVATOR_SYMBOLS == 1
celix_status_t  bundleActivator_start(void *userData, bundle_context_pt context __attribute__((unused))) {
    int status = CELIX_SUCCESS;
    DmActivatorImpl* impl = static_cast<DmActivatorImpl*>(userData);
    if (impl != nullptr) {
        status = impl->start();
    }
    return status;
}

celix_status_t bundleActivator_stop(void *userData, bundle_context_pt context __attribute__((unused))) {
    int status = CELIX_SUCCESS;
    DmActivatorImpl* impl = static_cast<DmActivatorImpl*>(userData);
    if (impl != nullptr) {
        status = impl->stop();
    }
    return status;
}

celix_status_t bundleActivator_destroy(void *userData, bundle_context_pt context __attribute__((unused))) {
    DmActivatorImpl* impl = static_cast<DmActivatorImpl*>(userData);
    if (impl != nullptr) {
        delete impl;
    }
    return CELIX_SUCCESS;
}
#endif