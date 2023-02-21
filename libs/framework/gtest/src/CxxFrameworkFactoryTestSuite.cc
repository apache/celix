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

#include <future>

#include "celix/FrameworkFactory.h"

class CxxFrameworkFactoryTestSuite : public ::testing::Test {
public:
    std::shared_ptr<celix::Framework> framework{};
    std::vector<std::shared_ptr<celix::ServiceRegistration>> registrations{};
};

TEST_F(CxxFrameworkFactoryTestSuite, CreateDestroy) {
    auto fw = celix::createFramework();
    EXPECT_TRUE(fw->getFrameworkBundleContext()->getBundle().isSystemBundle());
}

TEST_F(CxxFrameworkFactoryTestSuite, WaitForStop) {
    auto fw = celix::createFramework();
    std::future<void> sync;
    std::thread wait{[fw] {
        fw->waitForStop();
    }};
    fw->getFrameworkBundleContext()->stopBundle(0);
    wait.join();
}

struct cService{};

TEST_F(CxxFrameworkFactoryTestSuite, DestroyFrameworkAndUnregisterServices) {
    framework = celix::createFramework();
    auto ctx = framework->getFrameworkBundleContext();
    for (int i = 0; i < 10; ++i) {
        auto reg = ctx->registerService<cService>(std::make_shared<cService>())
                .build();
        registrations.emplace_back(std::move(reg));
    }
}

