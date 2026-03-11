/*
 Licensed to the Apache Software Foundation (ASF) under one
 or more contributor license agreements.  See the NOTICE file
 distributed with this work for additional information
 regarding copyright ownership.  The ASF licenses this file
 to you under the Apache License, Version 2.0 (the
 "License"); you may not use this file except in compliance
 with the License.  You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing,
  software distributed under the License is distributed on an
  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
  KIND, either express or implied.  See the License for the
  specific language governing permissions and limitations
  under the License.
 */

#include <celix/BundleContext.h>
#include <celix/IShellCommand.h>
#include <celix_framework.h>
#include <celix_framework_factory.h>
#include <celix_shell.h>
#include <celix_properties.h>
#include <iostream>
#include <atomic>
#include <assert.h>

class ShellCommandImpl : public celix::IShellCommand {
public:
    ~ShellCommandImpl() noexcept override = default;
    void executeCommand(const std::string& commandLine, const std::vector<std::string>& commandArgs, FILE* outStream, FILE* errorStream) override {
        fprintf(outStream, "called cxx command with cmd line %s\n", commandLine.c_str());
        fprintf(errorStream, "Arguments size is %i\n", (int)commandArgs.size());
    }
};

int main() {
    celix_framework_t* fw = NULL;
    celix_bundle_context_t *ctx = NULL;
    celix_properties_t *properties = NULL;

    properties = celix_properties_create();
    celix_properties_setBool(properties, "LOGHELPER_ENABLE_STDOUT_FALLBACK", true);
    celix_properties_setBool(properties, CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, true);
    celix_properties_set(properties, CELIX_FRAMEWORK_CACHE_DIR, ".cacheBundleContextTestFramework");

    fw = celix_frameworkFactory_createFramework(properties);
    ctx = celix_framework_getFrameworkContext(fw);
    long bndId = celix_bundleContext_installBundle(ctx, CXX_SHELL_BUNDLE_LOCATION, true);
    assert(bndId >= 0);

    auto cxxCtx = celix::BundleContext{ctx};
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
    assert(callCount == 1);
    std::size_t initialCount = commandCount.load();
    std::cout << "initialCount=" << initialCount << std::endl;

    auto reg = cxxCtx.registerService<celix::IShellCommand>(std::make_shared<ShellCommandImpl>())
            .addProperty(celix::IShellCommand::COMMAND_NAME, "cxx::example")
            .addProperty(celix::IShellCommand::COMMAND_USAGE, "usage")
            .addProperty(celix::IShellCommand::COMMAND_DESCRIPTION, "desc")
            .build();
    reg->wait();
    callCount = cxxCtx.useService<celix_shell>(CELIX_SHELL_SERVICE_NAME)
            .addUseCallback(countCb)
            .build();
    assert(callCount == 1);
    std::cout << "command count after registration=" << commandCount.load() << std::endl;
    reg.reset();
    celix_frameworkFactory_destroyFramework(fw);
    return 0;
}
