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

#include <gtest/gtest.h>

#include "celix/FrameworkFactory.h"

class CxxBundleActivatorTestSuite : public ::testing::Test {
public:
    CxxBundleActivatorTestSuite() {
        fw = celix::createFramework();
        ctx = fw->getFrameworkBundleContext();
    }

    std::shared_ptr<celix::Framework> fw{};
    std::shared_ptr<celix::BundleContext> ctx{};
};

class TestInterface {
public:
    virtual ~TestInterface() noexcept = default;
};

class TestImpl : public TestInterface {
public:
    ~TestImpl() noexcept override = default;
};

TEST_F(CxxBundleActivatorTestSuite, InstallUninstallBundleWithCmpAndTracker) {
    //When I install and uninstall a bundle with a component and tracker combination without asan issues (use after free)

    auto reg = ctx->registerService<TestInterface>(std::make_shared<TestImpl>()).build();

    auto bndId = ctx->installBundle(CMP_TEST_BUNDLE_LOC);
    EXPECT_GE(bndId, 0);
    ctx->uninstallBundle(bndId);

}

