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

extern "C" {

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "celix_launcher.h"
#include "framework.h"

    static framework_pt serverFramework = nullptr;
    static bundle_context_pt serverContext = nullptr;

    static framework_pt clientFramework = nullptr;
    static bundle_context_pt clientContext = nullptr;

    static void setupFm(void) {
        int rc = 0;
        bundle_pt bundle = nullptr;

        //server
        rc = celixLauncher_launch("framework1.properties", &serverFramework);
        ASSERT_EQ(CELIX_SUCCESS, rc);

        bundle = nullptr;
        rc = framework_getFrameworkBundle(serverFramework, &bundle);
        ASSERT_EQ(CELIX_SUCCESS, rc);

        rc = bundle_getContext(bundle, &serverContext);
        ASSERT_EQ(CELIX_SUCCESS, rc);


        //client
        rc = celixLauncher_launch("framework2.properties", &clientFramework);
        ASSERT_EQ(CELIX_SUCCESS, rc);

        bundle = nullptr;
        rc = framework_getFrameworkBundle(clientFramework, &bundle);
        ASSERT_EQ(CELIX_SUCCESS, rc);

        rc = bundle_getContext(bundle, &clientContext);
        ASSERT_EQ(CELIX_SUCCESS, rc);
    }

    static void teardownFm(void) {

        celixLauncher_stop(serverFramework);
        celixLauncher_waitForShutdown(serverFramework);
        celixLauncher_destroy(serverFramework);

        celixLauncher_stop(clientFramework);
        celixLauncher_waitForShutdown(clientFramework);
        celixLauncher_destroy(clientFramework);

        serverContext = nullptr;
        serverFramework = nullptr;
        clientContext = nullptr;
        clientFramework = nullptr;
    }

    static void testFrameworks(void) {
        printf("testing startup/shutdown multiple frameworks\n");
    }

}

class CelixMultipleFrameworks : public ::testing::Test {
public:
    CelixMultipleFrameworks() {
        setupFm();
    }

    ~CelixMultipleFrameworks() override {
        teardownFm();
    }
};

TEST_F(CelixMultipleFrameworks, testFrameworks) {
    testFrameworks();
}
