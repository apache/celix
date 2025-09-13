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

#include <csignal>
#include <thread>
#include <future>
#include <vector>
#include <string>
#include <pthread.h>

#include "celix_constants.h"
#include "celix_launcher.h"
#include "celix_launcher_private.h"
#include "celix_stdlib_cleanup.h"
#include "celix_utils.h"

#define LAUNCH_WAIT_TIMEOUT 200

class CelixLauncherTestSuite : public ::testing::Test {
  public:
    static std::future<void> launchInThread(const std::vector<std::string>& args, celix_properties_t* props, int expectedRc) {
        std::string propsStr{};
        if (props) {
            celix_autofree char* str;
            EXPECT_EQ(CELIX_SUCCESS, celix_properties_saveToString(props, 0, &str));
            propsStr = str;
            celix_properties_destroy(props);
        }
        return std::async(std::launch::async, [args, propsStr, expectedRc] {
            char** argv = new char*[args.size()];
            for (size_t i = 0; i < args.size(); ++i) {
                argv[i] = celix_utils_strdup(args[i].c_str());
            }
            const char* str = propsStr.empty() ? nullptr : propsStr.c_str();
            auto rc = celix_launcher_launchAndWait((int)args.size(), argv, str);
            EXPECT_EQ(rc, expectedRc);
            for (size_t i = 0; i < args.size(); ++i) {
                free(argv[i]);
            }
            delete [] argv;
        });
    }
};

TEST_F(CelixLauncherTestSuite, PrintPropertiesTest) {
    //launch framework with bundle configured to start
    //Given a properties set with 1 bundle configured for start and 1 bundle configured for install
    auto* props = celix_properties_create();
    ASSERT_TRUE(props != nullptr);
    celix_properties_set(props, CELIX_AUTO_START_0, SIMPLE_TEST_BUNDLE1_LOCATION);
    celix_properties_set(props, CELIX_AUTO_INSTALL, SIMPLE_TEST_BUNDLE2_LOCATION);

    //When I run the celixLauncher with "-p" argument and expect a 0 return code
    auto future = launchInThread({"programName", "-p"}, props, 0);

    //Then the launch will print the properties and exit
    auto status = future.wait_for(std::chrono::milliseconds{LAUNCH_WAIT_TIMEOUT});
    EXPECT_EQ(status, std::future_status::ready);
}

TEST_F(CelixLauncherTestSuite, PrintEmptyPropertiesTest) {
    //launch framework with bundle configured to start
    //Given an empty properties
    auto* props = celix_properties_create();
    ASSERT_TRUE(props != nullptr);

    //When I run the celixLauncher with "-p" argument and expect a 0 return code
    auto future = launchInThread({"programName", "-p", "empty.properties"}, props, 0);

    //Then the launch will print the properties and exit
    auto status = future.wait_for(std::chrono::milliseconds{LAUNCH_WAIT_TIMEOUT});
    EXPECT_EQ(status, std::future_status::ready);
}

TEST_F(CelixLauncherTestSuite, PrintHelpTest) {
    // props should be released by launcher
    auto* props = celix_properties_create();
    ASSERT_TRUE(props != nullptr);
    celix_properties_set(props, CELIX_AUTO_START_0, SIMPLE_TEST_BUNDLE1_LOCATION);
    celix_properties_set(props, CELIX_AUTO_INSTALL, SIMPLE_TEST_BUNDLE2_LOCATION);

    //When I run the celixLauncher with "-h" argument and expect a 0 return code
    auto future = launchInThread({"programName", "-h"}, props, 0);

    //Then the launch will print the help info and exit
    auto status = future.wait_for(std::chrono::milliseconds{LAUNCH_WAIT_TIMEOUT});
    EXPECT_EQ(status, std::future_status::ready);
}

TEST_F(CelixLauncherTestSuite, ExtractBundlesTest) {
    //launch framework with bundle configured to start
    //Given a properties set with 2 bundles configured for start
    auto* props = celix_properties_create();
    ASSERT_TRUE(props != nullptr);
    celix_properties_set(props, CELIX_AUTO_START_0, SIMPLE_TEST_BUNDLE1_LOCATION);
    celix_properties_set(props, CELIX_AUTO_INSTALL, SIMPLE_TEST_BUNDLE2_LOCATION);
    celix_autofree char* propsStr;
    ASSERT_EQ(CELIX_SUCCESS, celix_properties_saveToString(props, 0, &propsStr));

    //When I run the celixLauncher with "-c" argument and expect a 0 return code
    auto future = launchInThread({"programName", "-c"}, props, 0);

    //Then the launch print the result of the extracted bundles and exit
    auto status = future.wait_for(std::chrono::milliseconds{LAUNCH_WAIT_TIMEOUT});
    EXPECT_EQ(status, std::future_status::ready);
}

TEST_F(CelixLauncherTestSuite, ExtractNonexistentBundlesTest) {
    //launch framework with bundle configured to start
    //Given a properties set with a nonexistent bundle configured for start
    auto* props = celix_properties_create();
    ASSERT_TRUE(props != nullptr);
    auto start = std::string{} + "nonexistent";
    celix_properties_set(props, CELIX_AUTO_START_0, start.c_str());
    celix_autofree char* propsStr;
    ASSERT_EQ(CELIX_SUCCESS, celix_properties_saveToString(props, 0, &propsStr));

    //When I run the celixLauncher with "-c" argument and expect a 1 return code
    auto future = launchInThread({"programName", "-c"}, props, 1);

    //Then the launch will exit
    auto status = future.wait_for(std::chrono::milliseconds{LAUNCH_WAIT_TIMEOUT});
    EXPECT_EQ(status, std::future_status::ready);
}

TEST_F(CelixLauncherTestSuite, ExtractBundlesIntoTempTest) {
    //launch framework with bundle configured to start
    //Given a properties set with 2 bundles configured for start
    //And the CELIX_FRAMEWORK_CACHE_USE_TMP_DIR is set to true
    auto* props = celix_properties_create();
    ASSERT_TRUE(props != nullptr);
    auto start = std::string{} + SIMPLE_TEST_BUNDLE1_LOCATION + "," + SIMPLE_TEST_BUNDLE2_LOCATION;
    celix_properties_set(props, CELIX_AUTO_START_0, start.c_str());
    celix_properties_setBool(props, CELIX_FRAMEWORK_CACHE_USE_TMP_DIR, true);

    //When I run the celixLauncher with "-c" argument and expect a 1 return code, because
    //extracting bundles to a temp dir is not supported.
    auto future = launchInThread({"programName", "-c"}, props, 1);

    //Then the launch print the result of the extracted bundles and exit
    auto status = future.wait_for(std::chrono::milliseconds{LAUNCH_WAIT_TIMEOUT});
    EXPECT_EQ(status, std::future_status::ready);
}

TEST_F(CelixLauncherTestSuite, LaunchCelixTest) {
    //launch framework with bundle configured to start
    //Given a properties set with 2 bundles configured for start
    auto* props = celix_properties_create();
    ASSERT_TRUE(props != nullptr);
    auto start = std::string{} + SIMPLE_TEST_BUNDLE1_LOCATION + " " + SIMPLE_TEST_BUNDLE2_LOCATION;
    celix_properties_set(props, CELIX_AUTO_START_0, start.c_str());
    celix_autofree char* propsStr;
    ASSERT_EQ(CELIX_SUCCESS, celix_properties_saveToString(props, 0, &propsStr));

    //When I run the celixLauncher with "-c" argument
    auto future = launchInThread({"programName", "config.properties"}, props, 0);

    //Then the wait for the launch to finish will time out (framework is running)
    auto status = future.wait_for(std::chrono::milliseconds{LAUNCH_WAIT_TIMEOUT});
    EXPECT_EQ(status, std::future_status::timeout);

    //When I stop the framework
    celix_launcher_triggerStop();

    //Then the launch will exit
    status = future.wait_for(std::chrono::milliseconds{LAUNCH_WAIT_TIMEOUT});
    EXPECT_EQ(status, std::future_status::ready);
}

TEST_F(CelixLauncherTestSuite, LaunchWithInvalidConfigPropertiesTest) {
    //When launching the framework with an invalid config properties file
    auto future = launchInThread({"programName", "invalid.properties"}, nullptr, 1);

    //Then the launch will exit
    auto status = future.wait_for(std::chrono::milliseconds{LAUNCH_WAIT_TIMEOUT});
    EXPECT_EQ(status, std::future_status::ready);
}

TEST_F(CelixLauncherTestSuite, LaunchWithInvalidConfigProperties2Test) {

    //When launching the framework with a properties set with a negative shutdown period
    auto* props = celix_properties_create();
    ASSERT_TRUE(props != nullptr);
    celix_properties_set(props, CELIX_AUTO_START_0, IMMEDIATE_STOP_BUNDLE_LOCATION);
    celix_properties_setDouble(props, CELIX_LAUNCHER_SHUTDOWN_PERIOD_IN_SECONDS, -1.0);
    auto future = launchInThread({"programName"}, props, 0);
    //Then launch will exit
    auto status = future.wait_for(std::chrono::milliseconds{LAUNCH_WAIT_TIMEOUT});
    EXPECT_EQ(status, std::future_status::ready);
}


TEST_F(CelixLauncherTestSuite, StopLauncherWithSignalTest) {
    auto* props = celix_properties_create();
    // When launching the framework
    celix_properties_setDouble(props, CELIX_LAUNCHER_SHUTDOWN_PERIOD_IN_SECONDS, 0.01);
    auto future = launchInThread({"programName"}, props, 0);

    // Then the launch will not exit, because the framework is running
    auto status = future.wait_for(std::chrono::milliseconds{LAUNCH_WAIT_TIMEOUT});
    EXPECT_EQ(status, std::future_status::timeout);

    // When I stop the framework by sending a SIGINT signal
    int signal = SIGINT;
    EXPECT_EQ(0, pthread_kill(pthread_self(), signal));

    // Then the launch will exit
    status = future.wait_for(std::chrono::milliseconds{LAUNCH_WAIT_TIMEOUT});
    EXPECT_EQ(status, std::future_status::ready);
}

TEST_F(CelixLauncherTestSuite, DoubleStartAndStopLauncherTest) {
        // When launching the framework
        auto future = launchInThread({"programName"}, nullptr, 0);

        // Then the launch will not exit, because the framework is running
        auto status = future.wait_for(std::chrono::milliseconds{LAUNCH_WAIT_TIMEOUT});
        EXPECT_EQ(status, std::future_status::timeout);

        //When I try start the framework again and expect a 1 return code (framework already running)
        auto future2 = launchInThread({"programName"}, nullptr, 1);

        // The launch will exit
        status = future2.wait_for(std::chrono::milliseconds{LAUNCH_WAIT_TIMEOUT});
        EXPECT_EQ(status, std::future_status::ready);

        // When I stop the framework
        celix_launcher_triggerStop();

        // Then the launch will exit
        status = future.wait_for(std::chrono::milliseconds{LAUNCH_WAIT_TIMEOUT});
        EXPECT_EQ(status, std::future_status::ready);

        // When I try to stop the framework again
        celix_launcher_triggerStop();

        // Then nothing happens
}

TEST_F(CelixLauncherTestSuite, StartWithInvalidEmbeddedPropertiesTest) {
    //When launching the framework with an invalid embedded properties
    auto rc = celix_launcher_launchAndWait(0, nullptr, "invalid props");

    //Then the launch will exit with a return code of 1
    EXPECT_EQ(1, rc);
}

TEST_F(CelixLauncherTestSuite, StartWithInvalidArgumentsTest) {
    //When launching the framework with invalid arguments and expect a 1 return code
    auto future = launchInThread({"programName", "config1.properties", "config2.properties"}, nullptr, 1);

    // The launch will exit
    auto status = future.wait_for(std::chrono::milliseconds{LAUNCH_WAIT_TIMEOUT});
    EXPECT_EQ(status, std::future_status::ready);

    //When launching the framework with invalid arguments and expect a 1 return code
    future = launchInThread({"programName", "-c", "-p"}, nullptr, 1);

    // The launch will exit
    status = future.wait_for(std::chrono::milliseconds{LAUNCH_WAIT_TIMEOUT});
    EXPECT_EQ(status, std::future_status::ready);
}
