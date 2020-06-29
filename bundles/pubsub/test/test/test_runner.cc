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

#include "celix_api.h"
#include <unistd.h>
#include "receive_count_service.h"

#include <CppUTest/TestHarness.h>
#include <CppUTest/CommandLineTestRunner.h>

int main(int argc, char **argv) {
    MemoryLeakWarningPlugin::turnOffNewDeleteOverloads();
    int rc = RUN_ALL_TESTS(argc, argv);
    return rc;
}

TEST_GROUP(PUBSUB_INT_GROUP) {
    celix_framework_t *fw = NULL;
    celix_bundle_context_t *ctx = NULL;
    void setup() override {
        celixLauncher_launch("config.properties", &fw);
        ctx = celix_framework_getFrameworkContext(fw);
    }

    void teardown() override {
        celixLauncher_stop(fw);
        celixLauncher_waitForShutdown(fw);
        celixLauncher_destroy(fw);
        ctx = NULL;
        fw = NULL;
    }
};

TEST(PUBSUB_INT_GROUP, recvTest) {
    constexpr int TRIES = 40;
    constexpr int TIMEOUT = 250000;
    constexpr int MSG_COUNT = 100;

    int count = 0;

    for (int i = 0; i < TRIES; ++i) {
        count = 0;
        celix_bundleContext_useService(ctx, CELIX_RECEIVE_COUNT_SERVICE_NAME, &count, [](void *handle, void *svc) {
            auto* count_ptr = static_cast<int*>(handle);
            auto* count = static_cast<celix_receive_count_service_t*>(svc);
            *count_ptr = count->receiveCount(count->handle);
        });
        printf("Current msg count is %i, waiting for at least %i\n", count, MSG_COUNT);
        if (count >= MSG_COUNT) {
            break;
        }
        usleep(TIMEOUT);
    }
    CHECK(count >= MSG_COUNT);
}