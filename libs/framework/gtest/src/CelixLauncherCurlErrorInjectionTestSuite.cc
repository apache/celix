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
#include "curl_ei.h"

#include <future>

#define LAUNCH_WAIT_TIMEOUT_IN_MS 100


class CelixLauncherCurlErrorInjectionTestSuite : public ::testing::Test {
  public:
    CelixLauncherCurlErrorInjectionTestSuite() {
        celix_ei_expect_curl_global_init(nullptr, 0, CURLE_OK);
    }

    static void launchCelixWithCurlInitError() {
        //Given an error injection for curl_global_init from celix_launcher_launchAndWait
        celix_ei_expect_curl_global_init((void*)celix_launcher_launchAndWait, 1, CURLE_FAILED_INIT);

        //When calling celix_launcher_launchAndWait
        auto rc = celix_launcher_launchAndWait(0, nullptr, nullptr);

        //Then the expected error code should be returned
        EXPECT_EQ(rc, 1);    
    }
};

TEST_F(CelixLauncherCurlErrorInjectionTestSuite, LaunchCelixWithCurlInitError) {
    launchCelixWithCurlInitError();

    //When launcher several times, the result should be the same
    for (int i = 0; i < 5; ++i) {
        launchCelixWithCurlInitError();
    }

    //When launching without an error injection
    celix_ei_expect_curl_global_init((void*)nullptr, 0, CURLE_OK);
    std::future<int> futureRc = std::async(std::launch::async, [] {
        return celix_launcher_launchAndWait(0, nullptr, nullptr);
    });

    //And framework is given time to launch
    futureRc.wait_for(std::chrono::milliseconds(LAUNCH_WAIT_TIMEOUT_IN_MS));

    //Then the framework should be launched successfully, and return a 0 code after exiting
    celix_launcher_triggerStop();
    EXPECT_EQ(futureRc.get(), 0);
}
