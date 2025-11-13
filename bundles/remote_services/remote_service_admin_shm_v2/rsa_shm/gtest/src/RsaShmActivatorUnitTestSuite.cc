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

#include "rsa_shm_impl.h"
#include "rsa_shm_constants.h"
#include "remote_service_admin.h"
#include "celix_api.h"
#include "celix_properties_ei.h"
#include "celix_bundle_context_ei.h"
#include "malloc_ei.h"
#include "celix_dm_component_ei.h"
#include "asprintf_ei.h"
#include "celix_utils_ei.h"
#include <gtest/gtest.h>

class RsaShmActivatorUnitTestSuite : public ::testing::Test {
public:
    RsaShmActivatorUnitTestSuite() {
        auto* props = celix_properties_create();
        celix_properties_setBool(props, CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, true);
        celix_properties_set(props, CELIX_FRAMEWORK_CACHE_DIR, ".rsa_shm_cache");
        auto* fwPtr = celix_frameworkFactory_createFramework(props);
        auto* ctxPtr = celix_framework_getFrameworkContext(fwPtr);
        fw = std::shared_ptr<celix_framework_t>{fwPtr, [](auto* f) {celix_frameworkFactory_destroyFramework(f);}};
        ctx = std::shared_ptr<celix_bundle_context_t>{ctxPtr, [](auto*){/*nop*/}};
    }

    ~RsaShmActivatorUnitTestSuite() {
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_ei_expect_celix_properties_create(nullptr, 0, nullptr);
        celix_ei_expect_celix_properties_set(nullptr, 0, 0);
        celix_ei_expect_celix_dmComponent_addInterface(nullptr, 0, 0);
        celix_ei_expect_celix_dmServiceDependency_create(nullptr, 0, nullptr);
        celix_ei_expect_asprintf(nullptr, 0, 0);
        celix_ei_expect_celix_dmServiceDependency_setService(nullptr, 0, 0);
        celix_ei_expect_celix_dmComponent_addServiceDependency(nullptr, 0, 0);
        celix_ei_expect_celix_utils_strdup(nullptr, 0, nullptr);
        celix_ei_expect_celix_bundleContext_getDependencyManager(nullptr, 0, nullptr);
        celix_ei_expect_celix_dependencyManager_addAsync(nullptr, 0, 0);
    }


    std::shared_ptr<celix_framework_t> fw{};
    std::shared_ptr<celix_bundle_context_t> ctx{};
};

TEST_F(RsaShmActivatorUnitTestSuite, RsaShmActivatorStart) {
    void *userData = nullptr;
    auto status = celix_bundleActivator_create(ctx.get(), &userData);
    EXPECT_EQ(status, CELIX_SUCCESS);
    status = celix_bundleActivator_start(userData, ctx.get());
    EXPECT_EQ(status, CELIX_SUCCESS);

    bool found = celix_bundleContext_findService(ctx.get(), CELIX_RSA_REMOTE_SERVICE_ADMIN);
    EXPECT_TRUE(found);

    status = celix_bundleActivator_stop(userData, ctx.get());
    EXPECT_EQ(status, CELIX_SUCCESS);
    status = celix_bundleActivator_destroy(userData, ctx.get());
    EXPECT_EQ(status, CELIX_SUCCESS);
}

TEST_F(RsaShmActivatorUnitTestSuite, FailedToCreateRSAComponent) {
    void *userData = nullptr;
    auto status = celix_bundleActivator_create(ctx.get(), &userData);
    EXPECT_EQ(status, CELIX_SUCCESS);
    celix_ei_expect_celix_dmComponent_create((void*)&celix_bundleActivator_start, 1, nullptr);
    status = celix_bundleActivator_start(userData, ctx.get());
    EXPECT_EQ(status, CELIX_BUNDLE_EXCEPTION);

    status = celix_bundleActivator_destroy(userData, ctx.get());
    EXPECT_EQ(status, CELIX_SUCCESS);
}

TEST_F(RsaShmActivatorUnitTestSuite, RsaShmActivatorStartWithCreatingRsaError) {
    void *userData = nullptr;
    auto status = celix_bundleActivator_create(ctx.get(), &userData);
    EXPECT_EQ(status, CELIX_SUCCESS);
    celix_ei_expect_calloc((void*)&rsaShm_create, 0, nullptr);
    status = celix_bundleActivator_start(userData, ctx.get());
    EXPECT_EQ(status, CELIX_ENOMEM);

    status = celix_bundleActivator_destroy(userData, ctx.get());
    EXPECT_EQ(status, CELIX_SUCCESS);
}

TEST_F(RsaShmActivatorUnitTestSuite, RsaShmActivatorStartWithCreatingServicePropertiesError) {
    void *userData = nullptr;
    auto status = celix_bundleActivator_create(ctx.get(), &userData);
    EXPECT_EQ(status, CELIX_SUCCESS);
    celix_ei_expect_celix_properties_create((void*)&celix_bundleActivator_start, 1, nullptr);
    status = celix_bundleActivator_start(userData, ctx.get());
    EXPECT_EQ(status, CELIX_ENOMEM);

    status = celix_bundleActivator_destroy(userData, ctx.get());
    EXPECT_EQ(status, CELIX_SUCCESS);
}

TEST_F(RsaShmActivatorUnitTestSuite, RsaShmActivatorStartWithSettingServicePropertiesError) {
    void *userData = nullptr;
    auto status = celix_bundleActivator_create(ctx.get(), &userData);
    EXPECT_EQ(status, CELIX_SUCCESS);
    celix_ei_expect_celix_properties_set((void*)&celix_bundleActivator_start, 1, CELIX_ENOMEM);
    status = celix_bundleActivator_start(userData, ctx.get());
    EXPECT_EQ(status, CELIX_ENOMEM);

    status = celix_bundleActivator_destroy(userData, ctx.get());
    EXPECT_EQ(status, CELIX_SUCCESS);
}

TEST_F(RsaShmActivatorUnitTestSuite, FailedToAddRSAInterface) {
    void *userData = nullptr;
    auto status = celix_bundleActivator_create(ctx.get(), &userData);
    EXPECT_EQ(status, CELIX_SUCCESS);
    celix_ei_expect_celix_dmComponent_addInterface((void*)&celix_bundleActivator_start, 1, CELIX_BUNDLE_EXCEPTION);
    status = celix_bundleActivator_start(userData, ctx.get());
    EXPECT_EQ(status, CELIX_BUNDLE_EXCEPTION);

    status = celix_bundleActivator_destroy(userData, ctx.get());
    EXPECT_EQ(status, CELIX_SUCCESS);
}

TEST_F(RsaShmActivatorUnitTestSuite, FailedToCreateRpcFactoryDependency) {
    void *userData = nullptr;
    auto status = celix_bundleActivator_create(ctx.get(), &userData);
    EXPECT_EQ(status, CELIX_SUCCESS);
    celix_ei_expect_celix_dmServiceDependency_create((void*)&celix_bundleActivator_start, 2, nullptr);
    status = celix_bundleActivator_start(userData, ctx.get());
    EXPECT_EQ(status, ENOMEM);

    status = celix_bundleActivator_destroy(userData, ctx.get());
    EXPECT_EQ(status, CELIX_SUCCESS);
}

TEST_F(RsaShmActivatorUnitTestSuite, FailedToGenerateRpcFactoryFilterString) {
    void *userData = nullptr;
    auto status = celix_bundleActivator_create(ctx.get(), &userData);
    EXPECT_EQ(status, CELIX_SUCCESS);
    celix_ei_expect_asprintf((void*)&celix_bundleActivator_start, 2, -1, 2);//first:rsaShm_create
    status = celix_bundleActivator_start(userData, ctx.get());
    EXPECT_EQ(status, ENOMEM);

    status = celix_bundleActivator_destroy(userData, ctx.get());
    EXPECT_EQ(status, CELIX_SUCCESS);
}

TEST_F(RsaShmActivatorUnitTestSuite, FailedToSetServiceToRpcFactoryDependency) {
    void *userData = nullptr;
    auto status = celix_bundleActivator_create(ctx.get(), &userData);
    EXPECT_EQ(status, CELIX_SUCCESS);
    celix_ei_expect_celix_dmServiceDependency_setService((void*)&celix_bundleActivator_start, 2, ENOMEM);
    status = celix_bundleActivator_start(userData, ctx.get());
    EXPECT_EQ(status, ENOMEM);

    status = celix_bundleActivator_destroy(userData, ctx.get());
    EXPECT_EQ(status, CELIX_SUCCESS);
}

TEST_F(RsaShmActivatorUnitTestSuite, FailedToAddRpcFactoryDependency) {
    void *userData = nullptr;
    auto status = celix_bundleActivator_create(ctx.get(), &userData);
    EXPECT_EQ(status, CELIX_SUCCESS);
    celix_ei_expect_celix_dmComponent_addServiceDependency((void*)&celix_bundleActivator_start, 2, ENOMEM);
    status = celix_bundleActivator_start(userData, ctx.get());
    EXPECT_EQ(status, ENOMEM);

    status = celix_bundleActivator_destroy(userData, ctx.get());
    EXPECT_EQ(status, CELIX_SUCCESS);
}

TEST_F(RsaShmActivatorUnitTestSuite, FailedToGetDependencyManager) {
    void *userData = nullptr;
    auto status = celix_bundleActivator_create(ctx.get(), &userData);
    EXPECT_EQ(status, CELIX_SUCCESS);
    celix_ei_expect_celix_bundleContext_getDependencyManager((void*)&celix_bundleActivator_start, 1, nullptr);
    status = celix_bundleActivator_start(userData, ctx.get());
    EXPECT_EQ(status, CELIX_BUNDLE_EXCEPTION);

    status = celix_bundleActivator_destroy(userData, ctx.get());
    EXPECT_EQ(status, CELIX_SUCCESS);
}

TEST_F(RsaShmActivatorUnitTestSuite, FailedToAddComponentToDependencyManager) {
    void *userData = nullptr;
    auto status = celix_bundleActivator_create(ctx.get(), &userData);
    EXPECT_EQ(status, CELIX_SUCCESS);
    celix_ei_expect_celix_dependencyManager_addAsync((void*)&celix_bundleActivator_start, 1, CELIX_BUNDLE_EXCEPTION);
    status = celix_bundleActivator_start(userData, ctx.get());
    EXPECT_EQ(status, CELIX_BUNDLE_EXCEPTION);

    status = celix_bundleActivator_destroy(userData, ctx.get());
    EXPECT_EQ(status, CELIX_SUCCESS);
}

TEST_F(RsaShmActivatorUnitTestSuite, StartWithMultiRpcType) {
    setenv(CELIX_RSA_SHM_RPC_TYPES, "rpcType1, rpcType2", 1);
    void *userData = nullptr;
    auto status = celix_bundleActivator_create(ctx.get(), &userData);
    EXPECT_EQ(status, CELIX_SUCCESS);

    status = celix_bundleActivator_start(userData, ctx.get());
    EXPECT_EQ(status, CELIX_SUCCESS);

    status = celix_bundleActivator_stop(userData, ctx.get());
    EXPECT_EQ(status, CELIX_SUCCESS);
    status = celix_bundleActivator_destroy(userData, ctx.get());
    EXPECT_EQ(status, CELIX_SUCCESS);
    unsetenv(CELIX_RSA_SHM_RPC_TYPES);
}

TEST_F(RsaShmActivatorUnitTestSuite, FailedToDupRpcTypes) {
    setenv(CELIX_RSA_SHM_RPC_TYPES, "rpcType1", 1);
    void *userData = nullptr;
    auto status = celix_bundleActivator_create(ctx.get(), &userData);
    EXPECT_EQ(status, CELIX_SUCCESS);

    celix_ei_expect_celix_utils_strdup((void*)&celix_bundleActivator_start, 1, nullptr);
    status = celix_bundleActivator_start(userData, ctx.get());
    EXPECT_EQ(status, ENOMEM);

    status = celix_bundleActivator_destroy(userData, ctx.get());
    EXPECT_EQ(status, CELIX_SUCCESS);
    unsetenv(CELIX_RSA_SHM_RPC_TYPES);
}

TEST_F(RsaShmActivatorUnitTestSuite, StartWithEmptyRpcType) {
    setenv(CELIX_RSA_SHM_RPC_TYPES, "", 1);
    void *userData = nullptr;
    auto status = celix_bundleActivator_create(ctx.get(), &userData);
    EXPECT_EQ(status, CELIX_SUCCESS);

    status = celix_bundleActivator_start(userData, ctx.get());
    EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);

    status = celix_bundleActivator_destroy(userData, ctx.get());
    EXPECT_EQ(status, CELIX_SUCCESS);
    unsetenv(CELIX_RSA_SHM_RPC_TYPES);
}

TEST_F(RsaShmActivatorUnitTestSuite, FailedToCreateRpcFactoryDependencyForCustomRpcType) {
    setenv(CELIX_RSA_SHM_RPC_TYPES, "rpcType1", 1);
    void *userData = nullptr;
    auto status = celix_bundleActivator_create(ctx.get(), &userData);
    EXPECT_EQ(status, CELIX_SUCCESS);
    celix_ei_expect_celix_dmServiceDependency_create((void*)&celix_bundleActivator_start, 2, nullptr);
    status = celix_bundleActivator_start(userData, ctx.get());
    EXPECT_EQ(status, ENOMEM);

    status = celix_bundleActivator_destroy(userData, ctx.get());
    EXPECT_EQ(status, CELIX_SUCCESS);
    unsetenv(CELIX_RSA_SHM_RPC_TYPES);
}

