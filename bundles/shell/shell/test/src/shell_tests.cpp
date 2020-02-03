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

#include <thread>
#include <chrono>
#include <iostream>
#include <mutex>
#include <condition_variable>

#include <zconf.h>

#include "celix_api.h"
#include "command.h"

#include <CppUTest/TestHarness.h>
#include <CppUTest/CommandLineTestRunner.h>

#ifdef SHELL_BUNDLE_LOCATION
const char * const SHELL_BUNDLE = SHELL_BUNDLE_LOCATION;
#endif

#ifdef __APPLE__
#include "memstream/open_memstream.h"
#else
#include <stdio.h>
#endif


TEST_GROUP(CelixShellTests) {
    framework_t* fw = nullptr;
    bundle_context_t *ctx = nullptr;
    celix_properties_t *properties = nullptr;

    void setup() {
        properties = celix_properties_create();
        celix_properties_set(properties, "LOGHELPER_ENABLE_STDOUT_FALLBACK", "true");
        celix_properties_set(properties, "org.osgi.framework.storage.clean", "onFirstInit");
        celix_properties_set(properties, "org.osgi.framework.storage", ".cacheBundleContextTestFramework");

        fw = celix_frameworkFactory_createFramework(properties);
        ctx = framework_getContext(fw);

        long bndId = celix_bundleContext_installBundle(ctx, SHELL_BUNDLE, true);
        CHECK_TRUE(bndId >= 0);
    }

    void teardown() {
        celix_frameworkFactory_destroyFramework(fw);
    }
};

TEST(CelixShellTests, shellBundleInstalledTest) {
    auto *bndIds = celix_bundleContext_listBundles(ctx);
    CHECK_EQUAL(1, celix_arrayList_size(bndIds));
    celix_arrayList_destroy(bndIds);
}

TEST(CelixShellTests, queryTest) {
    celix_service_use_options_t opts{};
    opts.filter.serviceName = OSGI_SHELL_COMMAND_SERVICE_NAME;
    opts.filter.filter = "(command.name=query)";
    opts.waitTimeoutInSeconds = 1.0;
    opts.use = [](void */*handle*/, void *svc) {
        auto *command = static_cast<command_service_t*>(svc);
        CHECK_TRUE(command != nullptr);

        {
            char *buf = nullptr;
            size_t len;
            FILE *sout = open_memstream(&buf, &len);
            command->executeCommand(command->handle, (char *) "query", sout, sout);
            fclose(sout);
            STRCMP_CONTAINS("Provided services found 1", buf); //note could be 11, 12, etc
            //STRCMP_CONTAINS("Requested services found 1", buf); //note very explicit, could be improved
            free(buf);
        }
        {
            char *buf = nullptr;
            size_t len;
            FILE *sout = open_memstream(&buf, &len);
            command->executeCommand(command->handle, (char *) "query 0", sout, sout); //note query framework bundle -> no results
            fclose(sout);
            STRCMP_CONTAINS("No results", buf); //note could be 11, 12, etc
            free(buf);
        }
    };
    bool called = celix_bundleContext_useServiceWithOptions(ctx, &opts);
    CHECK_TRUE(called);
}
