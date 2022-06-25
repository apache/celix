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

#include <celix/IShellCommand.h>
#include <celix/BundleActivator.h>
#include <celix_shell_command.h>

static void useCxxShellCommand(const std::shared_ptr<celix::BundleContext>& ctx) {
    auto called = ctx->useService<celix::IShellCommand>()
            .setFilter("(name=MyCommand)")
            .addUseCallback([](celix::IShellCommand& cmdSvc) {
                cmdSvc.executeCommand("MyCommand test call from C++", {}, stdout, stderr);
            })
            .build();
    if (!called) {
        std::cerr << __PRETTY_FUNCTION__  << ": Command service not called!" << std::endl;
    }
}

static void useCShellCommand(const std::shared_ptr<celix::BundleContext>& ctx) {
    auto calledCount = ctx->useServices<celix_shell_command>(CELIX_SHELL_COMMAND_SERVICE_NAME)
            //Note the filter should match 2 shell commands
            .setFilter("(|(command.name=MyCCommand)(command.name=my_command))")
            .addUseCallback([](celix_shell_command& cmdSvc) {
                cmdSvc.executeCommand(cmdSvc.handle, "MyCCommand test call from C++", stdout, stderr);
            })
            .build();
    if (calledCount == 0) {
        std::cerr << __PRETTY_FUNCTION__  << ": Command service not called!" << std::endl;
    }
}

class UsingCommandServicesExample {
public:
    explicit UsingCommandServicesExample(const std::shared_ptr<celix::BundleContext>& ctx) {
        useCxxShellCommand(ctx);
        useCShellCommand(ctx);
    }

    ~UsingCommandServicesExample() noexcept = default;
};

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(UsingCommandServicesExample)