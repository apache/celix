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

#include <CppUTest/TestHarness.h>
#include <CppUTest/CommandLineTestRunner.h>

#include "celix_shell_command.h"
#include "celix_api.h"
#include "celix_shell.h"

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

static bool callCommand(celix_bundle_context_t *ctx, const char *cmdName, const char *cmdLine, bool cmdShouldSucceed) {
    celix_service_use_options_t opts{};
    char filter[512];
    snprintf(filter, 512, "(command.name=%s)", cmdName);

    struct callback_data {
        const char *cmdLine{};
        bool cmdShouldSucceed{};
    };
    struct callback_data data{};
    data.cmdLine = cmdLine;
    data.cmdShouldSucceed = cmdShouldSucceed;

    opts.filter.serviceName = CELIX_SHELL_COMMAND_SERVICE_NAME;
    opts.filter.filter = filter;
    opts.callbackHandle = static_cast<void*>(&data);
    opts.waitTimeoutInSeconds = 1.0;
    opts.use = [](void *handle, void *svc) {
        auto *cmd = static_cast<celix_shell_command_t*>(svc);
        auto *d = static_cast<struct callback_data*>(handle);
        CHECK_TRUE(cmd != nullptr);
        bool succeeded = cmd->executeCommand(cmd->handle, d->cmdLine, stdout, stderr);
        if (d->cmdShouldSucceed) {
            CHECK_TRUE_TEXT(succeeded, d->cmdLine);
        } else {
            CHECK_FALSE_TEXT(succeeded, d->cmdLine);
        }
    };
    bool called = celix_bundleContext_useServiceWithOptions(ctx, &opts);
    return called;
}

TEST(CelixShellTests, testAllCommandsAreCallable) {
    bool called = callCommand(ctx, "non-existing", "non-existing", false);
    CHECK_FALSE(called);

    called = callCommand(ctx, "install", "install a-bundle-loc.zip", false);
    CHECK_TRUE(called);

    called = callCommand(ctx, "help", "help", true);
    CHECK_TRUE(called);

    called = callCommand(ctx, "help", "help lb", true);
    CHECK_TRUE(called);

    called = callCommand(ctx, "help", "help non-existing-command", false);
    CHECK_TRUE(called);

    called = callCommand(ctx, "lb", "lb -a", true);
    CHECK_TRUE(called);

    called = callCommand(ctx, "lb", "lb -a", true);
    CHECK_TRUE(called);

    called = callCommand(ctx, "lb", "lb", true);
    CHECK_TRUE(called);

    called = callCommand(ctx, "query", "query", true);
    CHECK_TRUE(called);

    called = callCommand(ctx, "q", "q -v", true);
    CHECK_TRUE(called);

    called = callCommand(ctx, "stop", "stop 15", false);
    CHECK_TRUE(called);

    called = callCommand(ctx, "start", "start 15", false);
    CHECK_TRUE(called);

    called = callCommand(ctx, "uninstall", "uninstall 15", false);
    CHECK_TRUE(called);

    called = callCommand(ctx, "update", "update 15", false);
    CHECK_TRUE(called);
}

TEST(CelixShellTests, quitTest) {
    bool called = callCommand(ctx, "quit", "quit", true);
    CHECK_TRUE(called);
}

TEST(CelixShellTests, stopFrameworkTest) {
    bool called = callCommand(ctx, "stop", "stop 0", true);
    CHECK_TRUE(called);
}

TEST(CelixShellTests, queryTest) {
    celix_service_use_options_t opts{};
    opts.filter.serviceName = CELIX_SHELL_COMMAND_SERVICE_NAME;
    opts.filter.filter = "(command.name=query)";
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

#ifdef CELIX_ADD_DEPRECATED_API
#include "command.H"
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

    celix_service_use_options_t opts{};
    opts.filter.serviceName = CELIX_SHELL_SERVICE_NAME;
    opts.waitTimeoutInSeconds = 1.0;
    opts.use = [](void */*handle*/, void *svc) {
        auto *shell = static_cast<celix_shell_t*>(svc);
        celix_array_list_t *commands = nullptr;
        shell->getCommands(shell->handle, &commands);
        bool commandFound = false;
        for (int i = 0; i < celix_arrayList_size(commands); ++i) {
            auto* name = static_cast<char*>(celix_arrayList_get(commands, i));
            if (strncmp(name, "testCommand", 64) == 0) {
                commandFound = true;
            }
            free(name);
        }
        celix_arrayList_destroy(commands);
        CHECK_TRUE(commandFound);

        shell->executeCommand(shell->handle, "testCommand withArg", stdout, stderr);
    };
    bool called = celix_bundleContext_useServiceWithOptions(ctx, &opts);
    CHECK_TRUE(called);

    celix_bundleContext_unregisterService(ctx, svcId);
}
#endif