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

#include <utility>
#include <memory>

#include "celix/dm/DependencyManager.h"
#include "celix/dm/DmActivator.h"
#include "bundle_activator.h"


/**
 * Deprecated. Use the CELIX_GEN_CXX_BUNDLE_ACTIVATOR marco from celix_bundle_activator.h instead
 */
struct BundleActivatorData {
    std::shared_ptr<DependencyManager> mng;
    std::unique_ptr<celix::dm::DmActivator> act;
};

extern "C" celix_status_t bundleActivator_create(celix_bundle_context_t *context, void** userData) {
    int status = CELIX_SUCCESS;

    BundleActivatorData* data = nullptr;
#ifdef __EXCEPTIONS
    data = new BundleActivatorData{
        .mng = std::make_shared<celix::dm::DependencyManager>(context),
        .act = nullptr
    };
#else
    data = new(std::nothrow) BundleActivatorData{
            .mng = celix::dm::DependencyManager{context},
            .act = nullptr
    };
#endif
    if (data != nullptr) {
        data->act = std::unique_ptr<celix::dm::DmActivator>{celix::dm::DmActivator::create(*data->mng)};
    }

    if (data == nullptr || data->act == nullptr) {
        status = CELIX_ENOMEM;
        if (data != nullptr) {
            delete data;
        }
        *userData = nullptr;
    } else {
        *userData = data;
    }
    return status;
}

extern "C" celix_status_t bundleActivator_start(void* userData, celix_bundle_context_t *) {
    int status = CELIX_SUCCESS;
    auto* data = static_cast<BundleActivatorData*>(userData);
    if (data != nullptr) {
        status = data->act->start();
    }
    return status;
}

extern "C" celix_status_t bundleActivator_stop(void* userData, celix_bundle_context_t *) {
    int status = CELIX_SUCCESS;
    auto* data = static_cast<BundleActivatorData*>(userData);
    if (data != nullptr) {
        status = data->act->stop();
        data->act = nullptr;
    }
    return status;
}

extern "C" celix_status_t bundleActivator_destroy(void *userData, celix_bundle_context_t* ) {
    auto* data = static_cast<BundleActivatorData*>(userData);
    if (data != nullptr) {
        delete data;
    }
    return CELIX_SUCCESS;
}