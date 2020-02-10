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


#include "celix_shell_command.h"
#include "celix_api.h"
#include "celix_shell.h"

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

static void callCommand(celix_bundle_context_t *ctx, const char *cmdLine, bool cmdShouldSucceed) {
    celix_service_use_options_t opts{};

    struct callback_data {
        const char *cmdLine{};
        bool cmdShouldSucceed{};
    };
    struct callback_data data{};
    data.cmdLine = cmdLine;
    data.cmdShouldSucceed = cmdShouldSucceed;

    opts.filter.serviceName = CELIX_SHELL_SERVICE_NAME;
    opts.callbackHandle = static_cast<void*>(&data);
    opts.waitTimeoutInSeconds = 1.0;
    opts.use = [](void *handle, void *svc) {
        auto *shell = static_cast<celix_shell_t *>(svc);
        auto *d = static_cast<struct callback_data*>(handle);
        CHECK_TRUE(shell != nullptr);
        celix_status_t status = shell->executeCommand(shell->handle, d->cmdLine, stdout, stderr);
        if (d->cmdShouldSucceed) {
            CHECK_EQUAL_TEXT(CELIX_SUCCESS, status, d->cmdLine);
        } else {
            CHECK_TRUE_TEXT(status != CELIX_SUCCESS, d->cmdLine);
        }
    };
    bool called = celix_bundleContext_useServiceWithOptions(ctx, &opts);
    CHECK_TRUE(called);
}

TEST(CelixShellTests, testAllCommandsAreCallable) {
    callCommand(ctx, "non-existing", false);
    callCommand(ctx, "install a-bundle-loc.zip", false);
    callCommand(ctx, "help lb", false); //note need namespace
    callCommand(ctx, "help celix::lb", true);
    callCommand(ctx, "help non-existing-command", false);
    callCommand(ctx, "lb -a", true);
    callCommand(ctx, "lb", true);
    callCommand(ctx, "query", true);
    callCommand(ctx, "q -v", true);
    callCommand(ctx, "stop 15", false);
    callCommand(ctx, "start 15", false);
    callCommand(ctx, "uninstall 15", false);
    callCommand(ctx, "update 15", false);
}

TEST(CelixShellTests, quitTest) {
    callCommand(ctx, "quit", true);
}

TEST(CelixShellTests, stopFrameworkTest) {
    callCommand(ctx, "stop 0", true);
}

TEST(CelixShellTests, queryTest) {
    celix_service_use_options_t opts{};
    opts.filter.serviceName = CELIX_SHELL_COMMAND_SERVICE_NAME;
    opts.filter.filter = "(command.name=celix::query)";
    opts.waitTimeoutInSeconds = 1.0;
    opts.use = [](void */*handle*/, void *svc) {
        auto *command = static_cast<celix_shell_command_t*>(svc);
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

TEST(CelixShellTests, localNameClashTest) {
    callCommand(ctx, "lb", true);

    celix_shell_command_t cmdService;
    cmdService.handle = nullptr;
    cmdService.executeCommand = [](void *, const char* cmdLine, FILE *, FILE *) -> bool {
        CHECK_TRUE(cmdLine != NULL);
        return true;
    };

    celix_properties_t *props = celix_properties_create();
    celix_properties_set(props, CELIX_SHELL_COMMAND_NAME, "3rdparty::lb");
    long svcId = celix_bundleContext_registerService(ctx, &cmdService, CELIX_SHELL_COMMAND_SERVICE_NAME, props);

    //two lb commands, need namespace
    callCommand(ctx, "lb", false);
    callCommand(ctx, "celix::lb", true);
    callCommand(ctx, "3rdparty::lb", true);

    celix_bundleContext_unregisterService(ctx, svcId);

}

#ifdef CELIX_ADD_DEPRECATED_API
#include "command.h"
TEST(CelixShellTests, legacyCommandTest) {
    command_service_t cmdService;
    cmdService.handle = nullptr;
    cmdService.executeCommand = [](void *, char* cmdLine, FILE *, FILE *) -> celix_status_t {
        CHECK_TRUE(cmdLine != NULL);
        return CELIX_SUCCESS;
    };

    celix_properties_t *props = celix_properties_create();
    celix_properties_set(props, OSGI_SHELL_COMMAND_NAME, "testCommand");
    long svcId = celix_bundleContext_registerService(ctx, &cmdService, OSGI_SHELL_COMMAND_SERVICE_NAME, props);

    callCommand(ctx, "testCommand", true);

    celix_bundleContext_unregisterService(ctx, svcId);
}
#endif