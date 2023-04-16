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

#include "celix_launcher.h"


class MultipleFrameworkTestSuite : public ::testing::Test {
public:
    MultipleFrameworkTestSuite() = default;
    ~MultipleFrameworkTestSuite() noexcept override  = default;
};

TEST_F(MultipleFrameworkTestSuite, MultipleFrameworkTest) {
    //framework 1
    celix_framework_t *framework1 = nullptr;
    int rc = celixLauncher_launch("framework1.properties", &framework1);
    ASSERT_EQ(CELIX_SUCCESS, rc);
    celix_bundle_t* fwBundle1 = celix_framework_getFrameworkBundle(framework1);
    EXPECT_NE(nullptr, fwBundle1);
    celix_bundle_context_t* fwBundleContext1 = celix_framework_getFrameworkContext(framework1);
    EXPECT_NE(nullptr, fwBundleContext1);

    //framework 2
    celix_framework_t *framework2 = nullptr;
    rc = celixLauncher_launch("framework2.properties", &framework2);
    celix_bundle_t* fwBundle2 = celix_framework_getFrameworkBundle(framework2);
    EXPECT_NE(nullptr, fwBundle2);
    celix_bundle_context_t* fwBundleContext2 = celix_framework_getFrameworkContext(framework2);
    EXPECT_NE(nullptr, fwBundleContext2);

    celixLauncher_stop(framework1);
    celixLauncher_waitForShutdown(framework1);
    celixLauncher_destroy(framework1);

    celixLauncher_stop(framework2);
    celixLauncher_waitForShutdown(framework2);
    celixLauncher_destroy(framework2);
}
