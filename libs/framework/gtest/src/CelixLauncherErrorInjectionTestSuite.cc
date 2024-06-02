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
#include "celix_stdlib_cleanup.h"
#include "celix_utils.h"
#include "framework.h"
#include "celix_launcher_private.h"

#include "celix_properties_ei.h"
#include "malloc_ei.h"

#define LAUNCH_WAIT_TIMEOUT 100

class CelixLauncherErrorInjectionTestSuite : public ::testing::Test {
  public:
    CelixLauncherErrorInjectionTestSuite() {
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_ei_expect_celix_properties_setEntry(nullptr, 0, CELIX_SUCCESS);
    }

    static int launch(const std::vector<std::string>& args, celix_properties_t* props) {
        celix_autofree char* str = nullptr;
        if (props) {
            EXPECT_EQ(CELIX_SUCCESS, celix_properties_saveToString(props, 0, &str));
        }

        char** argv = new char*[args.size()];
        for (size_t i = 0; i < args.size(); ++i) {
            argv[i] = celix_utils_strdup(args[i].c_str());
        }

        auto rc = celix_launcher_launchAndWait((int)args.size(), argv, str);

        for (size_t i = 0; i < args.size(); ++i) {
            free(argv[i]);
        }
        delete [] argv;
        return rc;
    }
};

TEST_F(CelixLauncherErrorInjectionTestSuite, CreateBundleCacheErrorTest) {
    //Given an error injection for calloc is primed when called from framework_create
    celix_ei_expect_calloc((void*)framework_create, 0, nullptr);

    //When calling celix_launcher_launchAndWait
    auto rc = launch({"programName", "-c"}, nullptr);

    //Then an exception is expected
    EXPECT_EQ(1, rc);
}

TEST_F(CelixLauncherErrorInjectionTestSuite, CombinePropertiesErrorTest) {
    //Given an error injection for celix_properties_setEntry is primed when called from celix_launcher_combineProperties
    celix_ei_expect_celix_properties_setEntry((void*)celix_launcher_combineProperties, 0, ENOMEM);

    //When calling celix_launcher_launchAndWait with configuration properties
    auto rc = launch({"programName"}, nullptr);

    //Then an exception is expected
    EXPECT_EQ(1, rc);
}
