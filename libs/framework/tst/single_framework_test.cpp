/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */
#include <CppUTest/TestHarness.h>
#include <CppUTest/CommandLineTestRunner.h>

extern "C" {

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "celix_launcher.h"
#include "celix_framework_factory.h"


    static framework_pt framework = NULL;
    static bundle_context_pt context = NULL;

    static void setupFm(void) {
        int rc = 0;

        rc = celixLauncher_launch("config.properties", &framework);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        bundle_pt bundle = NULL;
        rc = framework_getFrameworkBundle(framework, &bundle);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        rc = bundle_getContext(bundle, &context);
        CHECK_EQUAL(CELIX_SUCCESS, rc);
    }

    static void teardownFm(void) {

        celixLauncher_stop(framework);
        celixLauncher_waitForShutdown(framework);
        celixLauncher_destroy(framework);

        context = NULL;
        framework = NULL;
    }

    static void testFramework(void) {
        //intentional empty. start/shutdown test
        printf("testing startup/shutdown single framework\n");
    }

    //TODO test register / use service

}


TEST_GROUP(CelixFramework) {
    void setup() {
        setupFm();
    }

    void teardown() {
        teardownFm();
    }
};

TEST(CelixFramework, testFramework) {
    testFramework();
}

TEST_GROUP(FrameworkFactory) {
};


TEST(FrameworkFactory, testFactoryCreate) {
    framework_t* fw = frameworkFactory_newFramework(NULL);
    CHECK(fw != NULL);
    framework_stop(fw);
    framework_waitForStop(fw);
    framework_destroy(fw); //note stop, wait and then destroy is needed .. combine ?
}

TEST(FrameworkFactory, testFactoryCreateAndToManyStartAndStops) {
    framework_t* fw = frameworkFactory_newFramework(NULL);
    CHECK(fw != NULL);

    framework_start(fw); //should already be done by frameworkFactory_newFramework();
    framework_start(fw);
    framework_start(fw);
    framework_start(fw);

    framework_stop(fw); //will also be implicitly done by framework_destroy
    framework_stop(fw);
    framework_stop(fw);
    framework_stop(fw);

    framework_stop(fw);
    framework_waitForStop(fw);
    framework_destroy(fw); //note stop, wait and then destroy is needed .. combine ?
}

TEST(FrameworkFactory, restartFramework) {
    framework_t* fw = frameworkFactory_newFramework(NULL);
    CHECK(fw != NULL);


    /* TODO fix mem leak in restarting framwork
    framework_stop(fw);
    framework_start(fw);
    framework_stop(fw);
    framework_start(fw);
     */

    framework_stop(fw);
    framework_waitForStop(fw);
    framework_destroy(fw);
}

