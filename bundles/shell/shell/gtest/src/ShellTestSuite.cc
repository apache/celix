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

#include <future>
#include <gtest/gtest.h>
#include <thread>

#include "celix_bundle_context.h"
#include "celix_constants.h"
#include "celix_framework_factory.h"
#include "celix_framework_utils.h"
#include "celix_shell.h"
#include "celix_shell_command.h"
#include "celix_stdlib_cleanup.h"

class ShellTestSuite : public ::testing::Test {
public:
    ShellTestSuite() : ctx{createFrameworkContext()} {
        auto* fw = celix_bundleContext_getFramework(ctx.get());
        size_t nr = celix_framework_utils_installBundleSet(fw, TEST_BUNDLES, true);
        EXPECT_EQ(nr, 2); //shell and  celix_shell_empty_resource_test_bundle bundle
    }

    static std::shared_ptr<celix_bundle_context_t> createFrameworkContext() {
        auto properties = celix_properties_create();
        celix_properties_set(properties, "LOGHELPER_ENABLE_STDOUT_FALLBACK", "true");
        celix_properties_set(properties, CELIX_FRAMEWORK_CACHE_DIR, ".cacheShellTestSuite");
        celix_properties_setBool(properties, CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, true);
        celix_properties_set(properties, "CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "trace");

        //to ensure "query 0" is still a test case for am empty result.
        celix_properties_setBool(properties, CELIX_FRAMEWORK_CONDITION_SERVICES_ENABLED, false);

        auto* cFw = celix_frameworkFactory_createFramework(properties);
        auto cCtx = celix_framework_getFrameworkContext(cFw);

        return std::shared_ptr<celix_bundle_context_t>{cCtx, [](celix_bundle_context_t* context) {
            auto *fw = celix_bundleContext_getFramework(context);
            celix_frameworkFactory_destroyFramework(fw);
        }};
    }

    long getBundleIdForResourceBundle() const {
        celix_autoptr(celix_array_list_t) bundles = celix_bundleContext_listBundles(ctx.get());
        for (auto i = 0 ; i < celix_arrayList_size(bundles); ++i) {
            auto bndId = celix_arrayList_getLong(bundles, i);
            celix_autofree char* name = celix_bundleContext_getBundleSymbolicName(ctx.get(), bndId);
            if (strstr(name, "resource") != nullptr) {
                return bndId;
            }

        }
        return -1;
    }

    std::shared_ptr<celix_bundle_context_t> ctx;
};

TEST_F(ShellTestSuite, shellBundleInstalledTest) {
    auto *bndIds = celix_bundleContext_listBundles(ctx.get());
    EXPECT_EQ(2, celix_arrayList_size(bndIds));
    celix_arrayList_destroy(bndIds);
}

static void callCommand(std::shared_ptr<celix_bundle_context_t>& ctx, const char *cmdLine, bool cmdShouldSucceed) {
    struct callback_data {
        const char *cmdLine{};
        bool cmdShouldSucceed{};
        std::promise<void> barrier{};
        celix_bundle_context_t* context{};
        long tracker{-1};
    };
    struct callback_data data{};
    data.cmdLine = cmdLine;
    data.cmdShouldSucceed = cmdShouldSucceed;
    data.context = ctx.get();
    celix_service_tracking_options_t opts{};
    opts.filter.serviceName = CELIX_SHELL_SERVICE_NAME;
    opts.callbackHandle = &data;
    opts.set = [](void * handle, void * svc) {
        if (svc == nullptr) {
            return;
        }
        auto *shell = static_cast<celix_shell_t *>(svc);
        auto *d = static_cast<struct callback_data*>(handle);
        EXPECT_TRUE(shell != nullptr);
        celix_status_t status = shell->executeCommand(shell->handle, d->cmdLine, stdout, stderr);
        if (d->cmdShouldSucceed) {
            EXPECT_EQ(CELIX_SUCCESS, status) << "Command '" << d->cmdLine << "' should succeed";
        } else {
            EXPECT_NE(CELIX_SUCCESS, status) << "Command '" << d->cmdLine << "' should not succeed";
        }
        celix_bundleContext_stopTracker(d->context, d->tracker);
        d->barrier.set_value();
    };
    data.tracker = celix_bundleContext_trackServicesWithOptions(ctx.get(), &opts);
    data.barrier.get_future().wait();
}

TEST_F(ShellTestSuite, testAllCommandsAreCallable) {
    callCommand(ctx, "non-existing", false);
    callCommand(ctx, "install a-bundle-loc.zip", true);
    callCommand(ctx, "help", true);
    callCommand(ctx, "help lb", true);
    callCommand(ctx, "help celix::lb", true);
    callCommand(ctx, "help non-existing-command", false);
    callCommand(ctx, "help celix::non-existing-command-with-namespace", false);
    callCommand(ctx, "lb", true);
    callCommand(ctx, "lb -l", true);
    callCommand(ctx, "query", true);
    callCommand(ctx, "q -v", true);
    callCommand(ctx, "stop not-a-number", false);
    callCommand(ctx, "stop", false); // incorrect number of arguments
    callCommand(ctx, "start not-a-number", false);
    callCommand(ctx, "start", false); // incorrect number of arguments
    callCommand(ctx, "uninstall not-a-number", false);
    callCommand(ctx, "uninstall", false); // incorrect number of arguments
    callCommand(ctx, "unload not-a-number", false);
    callCommand(ctx, "unload", false); // incorrect number of arguments
    callCommand(ctx, "update not-a-number", false);
    callCommand(ctx, "update", false); // incorrect number of arguments
    callCommand(ctx, "stop 15", false); //non existing bundle id
    callCommand(ctx, "start 15", false); //non existing bundle id
    callCommand(ctx, "uninstall 15", false); //non existing bundle id
    callCommand(ctx, "unload 15", false); //non existing bundle id
    callCommand(ctx, "update 15", false); //non existing bundle id
}

TEST_F(ShellTestSuite, quitTest) {
    callCommand(ctx, "quit", true);

    //ensure that the command can be executed, before framework stop
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
}

TEST_F(ShellTestSuite, stopFrameworkTest) {
    callCommand(ctx, "stop 0", true);

    //ensure that the command can be executed, before framework stop
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
}

TEST_F(ShellTestSuite, stopSelfTest) {
    auto* list = celix_bundleContext_listBundles(ctx.get());
    ASSERT_GE(celix_arrayList_size(list), 1);
    auto nrOfBundles = celix_arrayList_size(list);
    long shellBundleId = celix_arrayList_getLong(list, 0);
    celix_arrayList_destroy(list);

    //rule it should be possible to stop the Shell bundle from the stop command (which is part of the Shell bundle)
    std::string cmd = std::string{"stop "} + std::to_string(shellBundleId);
    callCommand(ctx, cmd.c_str(), true);

    //ensure that the command can be executed
    std::this_thread::sleep_for(std::chrono::milliseconds{100});

    list = celix_bundleContext_listBundles(ctx.get());
    EXPECT_EQ(nrOfBundles-1, celix_arrayList_size(list));
    celix_arrayList_destroy(list);
}

TEST_F(ShellTestSuite, queryTest) {
    struct data {
        long resourceBundleId;
    };
    data data{};
    data.resourceBundleId = getBundleIdForResourceBundle();

    celix_service_use_options_t opts{};
    opts.filter.serviceName = CELIX_SHELL_COMMAND_SERVICE_NAME;
    opts.filter.filter = "(command.name=celix::query)";
    opts.waitTimeoutInSeconds = 1.0;
    opts.callbackHandle = (void*)&data;
    opts.use = [](void* handle, void *svc) {
        auto *d = static_cast<struct data*>(handle);
        auto *command = static_cast<celix_shell_command_t*>(svc);
        ASSERT_TRUE(command != nullptr);
        ASSERT_TRUE(d != nullptr);

        {
            char *buf = nullptr;
            size_t len;
            FILE *sout = open_memstream(&buf, &len);
            command->executeCommand(command->handle, "query", sout, sout);
            fclose(sout);
            char* found = strstr(buf, "Provided services found 1"); //note could be 11, 12, etc
            EXPECT_TRUE(found != nullptr);
            free(buf);
        }
        {
            char *buf = nullptr;
            size_t len;
            FILE *sout = open_memstream(&buf, &len);
            command->executeCommand(command->handle, "query 1", sout, sout);
            fclose(sout);
            char* found = strstr(buf, "Provided services found 1"); //note could be 11, 12, etc
            EXPECT_TRUE(found != nullptr);
            free(buf);
        }
        {
            char *buf = nullptr;
            size_t len;
            FILE *sout = open_memstream(&buf, &len);
            auto cmd = std::string{"query "} + std::to_string(d->resourceBundleId);
            command->executeCommand(command->handle, cmd.c_str(), sout, sout); //note query test resource bundle -> no results
            fclose(sout);
            char* found = strstr(buf, "No results");
            EXPECT_TRUE(found != nullptr);
            free(buf);
        }
        {
            char *buf = nullptr;
            size_t len;
            FILE *sout = open_memstream(&buf, &len);
            command->executeCommand(command->handle, "query celix_shell_command", sout, sout); //note query test resource bundle -> no results
            fclose(sout);
            char* found = strstr(buf, "Provided services found 1"); //note could be 11, 12, etc
            EXPECT_TRUE(found != nullptr);
            free(buf);
        }
    };
    bool called = celix_bundleContext_useServiceWithOptions(ctx.get(), &opts);
    EXPECT_TRUE(called);
}

TEST_F(ShellTestSuite, localNameClashTest) {
    callCommand(ctx, "lb", true);

    celix_shell_command_t cmdService;
    cmdService.handle = nullptr;
    cmdService.executeCommand = [](void *, const char* cmdLine, FILE *, FILE *) -> bool {
        EXPECT_TRUE(cmdLine != nullptr);
        return true;
    };

    celix_properties_t *props = celix_properties_create();
    celix_properties_set(props, CELIX_SHELL_COMMAND_NAME, "3rdparty::lb");
    long svcId = celix_bundleContext_registerService(ctx.get(), &cmdService, CELIX_SHELL_COMMAND_SERVICE_NAME, props);

    //two lb commands, need namespace
    callCommand(ctx, "lb", false);
    callCommand(ctx, "celix::lb", true);
    callCommand(ctx, "3rdparty::lb", true);

    celix_bundleContext_unregisterService(ctx.get(), svcId);

}

#ifdef CXX_SHELL
#include "celix/BundleContext.h"
#include "celix/IShellCommand.h"

class ShellCommandImpl : public celix::IShellCommand {
public:
    ~ShellCommandImpl() noexcept override = default;
    void executeCommand(const std::string& commandLine, const std::vector<std::string>& commandArgs, FILE* outStream, FILE* errorStream) override {
        fprintf(outStream, "called cxx command with cmd line %s\n", commandLine.c_str());
        fprintf(errorStream, "Arguments size is %i\n", (int)commandArgs.size());
    }
};

TEST_F(ShellTestSuite, CxxShellTest) {
    auto cxxCtx = celix::BundleContext{ctx.get()};
    std::atomic<std::size_t> commandCount{0};
    auto countCb = [&commandCount](celix_shell& shell) {
        celix_array_list_t* result = nullptr;
        shell.getCommands(shell.handle, &result);
        commandCount = celix_arrayList_size(result);
        for (int i = 0; i < celix_arrayList_size(result); ++i) {
            free(celix_arrayList_get(result, i));
        }
        celix_arrayList_destroy(result);
    };
    auto callCount = cxxCtx.useService<celix_shell>(CELIX_SHELL_SERVICE_NAME)
            .addUseCallback(countCb)
            .build();
    EXPECT_EQ(1, callCount);
    std::size_t initialCount = commandCount.load();
    EXPECT_GT(initialCount, 0);

    callCommand(ctx, "example", false);

    auto reg = cxxCtx.registerService<celix::IShellCommand>(std::make_shared<ShellCommandImpl>())
            .addProperty(celix::IShellCommand::COMMAND_NAME, "cxx::example")
            .addProperty(celix::IShellCommand::COMMAND_USAGE, "usage")
            .addProperty(celix::IShellCommand::COMMAND_DESCRIPTION, "desc")
            .build();
    reg->wait();

    callCount = cxxCtx.useService<celix_shell>(CELIX_SHELL_SERVICE_NAME)
            .addUseCallback(countCb)
            .build();
    EXPECT_EQ(1, callCount);
    EXPECT_EQ(commandCount.load(), initialCount + 1);

    callCommand(ctx, "example", true);
    callCommand(ctx, "cxx::example bla boe", true);
}
#endif