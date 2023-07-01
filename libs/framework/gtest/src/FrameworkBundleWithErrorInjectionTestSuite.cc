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
#include "celix_condition.h"
#include "celix_framework_bundle.h"

#include "celix_properties_ei.h"
#include "celix_threads_ei.h"
#include "malloc_ei.h"

class FrameworkBundleWithErrorInjectionTestSuite : public ::testing::Test {
  public:
    const int USE_SERVICE_TIMEOUT_IN_MS = 500;
    const std::string readyFilter =
        std::string{"("} + CELIX_CONDITION_ID + "=" + CELIX_CONDITION_ID_FRAMEWORK_READY + ")";

    FrameworkBundleWithErrorInjectionTestSuite() = default;

    ~FrameworkBundleWithErrorInjectionTestSuite() noexcept override {
        // reset error injections
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_ei_expect_celixThreadMutex_create(nullptr, 0, CELIX_SUCCESS);
        celix_ei_expect_celix_properties_create(nullptr, 0, nullptr);
    }
};

TEST_F(FrameworkBundleWithErrorInjectionTestSuite, ErroCreatingFrameworkBundleTest) {
    // Wen an error injection for calloc is primed when called from celix_frameworkBundle_create
    celix_ei_expect_calloc((void*)celix_frameworkBundle_create, 0, nullptr);
    ;

    // Then an exception is expected when creating a framework instance
    EXPECT_ANY_THROW(celix::createFramework());

    // When an error injection for celixThreadMutex_create is primed when called from celix_frameworkBundle_create
    celix_ei_expect_celixThreadMutex_create((void*)celix_frameworkBundle_create, 0, ENOMEM);

    // Then an exception is expected when creating a framework instance
    EXPECT_ANY_THROW(celix::createFramework());
}

TEST_F(FrameworkBundleWithErrorInjectionTestSuite, ErrorStartingFrameworkBundleTest) {
    // Wen an error injection for celix_properties_create is primed when called (indirectly) from
    //  celix_frameworkBundle_start
    celix_ei_expect_celix_properties_create((void*)celix_frameworkBundle_start, 1, nullptr);

    // Then an exception is expected when creating a framework instance
    EXPECT_ANY_THROW(celix::createFramework());
}

TEST_F(FrameworkBundleWithErrorInjectionTestSuite, ErrorRegisteringFrameworkReadyConditionTest) {
    // When an error injection for celix_properties_create is primed when called from celix_frameworkBundle_readyCheck
    celix_ei_expect_celix_properties_create((void*)celix_frameworkBundle_readyCheck, 0, nullptr);

    // And a framework instance is created
    auto fw = celix::createFramework();
    auto ctx = fw->getFrameworkBundleContext();

    // Then the framework.ready condition will not become available, due to an error creating properties for the service
    auto count = ctx->useService<celix_condition>(CELIX_CONDITION_SERVICE_NAME)
                     .setFilter(readyFilter)
                     .setTimeout(std::chrono::milliseconds {USE_SERVICE_TIMEOUT_IN_MS})
                     .build();
    EXPECT_EQ(count, 0);
}
