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
#include <thread>

#include "celix_launcher.h"
#include "celix_constants.h"
#include "celix_utils.h"


class CelixLauncherTestSuite : public ::testing::Test {};

TEST_F(CelixLauncherTestSuite, PrintPropertiesTest) {
    //launch framework with bundle configured to start
    //Given a properties set with 1 bundle configured for start and 1 bundle configured for install
    auto* props = celix_properties_create();
    celix_properties_set(props, CELIX_AUTO_START_0, SIMPLE_TEST_BUNDLE1_LOCATION);
    celix_properties_set(props, CELIX_AUTO_INSTALL, SIMPLE_TEST_BUNDLE2_LOCATION);

    //When I run the celixLauncher with "-p" argument
    char* arg1 = celix_utils_strdup("programName");
    char* arg2 = celix_utils_strdup("-p");
    char* argv[] = {arg1, arg2};
    int rc = celixLauncher_launchAndWaitForShutdown(2, argv, props);

    //Then it will print the properties and exit with 0
    EXPECT_EQ(rc, 0);
    free(arg1);
    free(arg2);
}

TEST_F(CelixLauncherTestSuite, PrintHelpTest) {
    // props should be released by launcher
    auto* props = celix_properties_create();
    celix_properties_set(props, CELIX_AUTO_START_0, SIMPLE_TEST_BUNDLE1_LOCATION);
    celix_properties_set(props, CELIX_AUTO_INSTALL, SIMPLE_TEST_BUNDLE2_LOCATION);
    //When I run the celixLauncher with "-h" argument
    char* arg1 = celix_utils_strdup("programName");
    char* arg2 = celix_utils_strdup("-h");
    char* argv[] = {arg1, arg2};
    int rc = celixLauncher_launchAndWaitForShutdown(2, argv, props);

    //Then it will print the usage and exit with 0
    EXPECT_EQ(rc, 0);
    free(arg1);
    free(arg2);
}

TEST_F(CelixLauncherTestSuite, ExtractBundlesTest) {
    //launch framework with bundle configured to start
    //Given a properties set with 2 bundles configured for start
    auto* props = celix_properties_create();
    auto start = std::string{} + SIMPLE_TEST_BUNDLE1_LOCATION + " " + SIMPLE_TEST_BUNDLE2_LOCATION;
    celix_properties_set(props, CELIX_AUTO_START_0, start.c_str());

    //When I run the celixLauncher with "-c" argument
    char* arg1 = celix_utils_strdup("programName");
    char* arg2 = celix_utils_strdup("-c");
    char* argv[] = {arg1, arg2};
    int rc = celixLauncher_launchAndWaitForShutdown(2, argv, props);

    //Then it will print the result of the extracted bundles and exit with 0
    EXPECT_EQ(rc, 0);
    free(arg1);
    free(arg2);
}

TEST_F(CelixLauncherTestSuite, ExtractNonexistentBundlesTest) {
    //launch framework with bundle configured to start
    //Given a properties set with 2 bundles configured for start
    auto* props = celix_properties_create();
    auto start = std::string{} + "nonexistent";
    celix_properties_set(props, CELIX_AUTO_START_0, start.c_str());

    //When I run the celixLauncher with "-c" argument
    char* arg1 = celix_utils_strdup("programName");
    char* arg2 = celix_utils_strdup("-c");
    char* argv[] = {arg1, arg2};
    int rc = celixLauncher_launchAndWaitForShutdown(2, argv, props);

    //Then it will print the result of the extracted bundles and exit with 0
    EXPECT_EQ(rc, 1);
    free(arg1);
    free(arg2);
}

TEST_F(CelixLauncherTestSuite, ExtractBundlesIntoTempTest) {
    //launch framework with bundle configured to start
    //Given a properties set with 2 bundles configured for start
    auto* props = celix_properties_create();
    auto start = std::string{} + SIMPLE_TEST_BUNDLE1_LOCATION + " " + SIMPLE_TEST_BUNDLE2_LOCATION;
    celix_properties_set(props, CELIX_AUTO_START_0, start.c_str());
    celix_properties_setBool(props, CELIX_FRAMEWORK_CACHE_USE_TMP_DIR, true);

    //When I run the celixLauncher with "-c" argument
    char* arg1 = celix_utils_strdup("programName");
    char* arg2 = celix_utils_strdup("-c");
    char* argv[] = {arg1, arg2};
    int rc = celixLauncher_launchAndWaitForShutdown(2, argv, props);

    //Then it will print the result of the extracted bundles and exit with 0
    EXPECT_EQ(rc, 1);
    free(arg1);
    free(arg2);
}

TEST_F(CelixLauncherTestSuite, LaunchCelixTest) {
    //launch framework with bundle configured to start
    //Given a properties set with 2 bundles configured for start
    auto* props = celix_properties_create();
    auto start = std::string{} + SIMPLE_TEST_BUNDLE1_LOCATION + " " + SIMPLE_TEST_BUNDLE2_LOCATION;
    celix_properties_set(props, CELIX_AUTO_START_0, start.c_str());

    //When I run the celixLauncher with "config.properties" argument
    char* arg1 = celix_utils_strdup("programName");
    char* arg2 = celix_utils_strdup("config.properties");
    char* argv[] = {arg1, arg2};
    celix_framework_t* fw = nullptr;
    int rc = celixLauncher_launchWithArgv(2, argv, props, &fw);
    ASSERT_EQ(rc, 0);

    celixLauncher_stop(fw);
    celixLauncher_waitForShutdown(fw);
    celixLauncher_destroy(fw);

    free(arg1);
    free(arg2);
}
