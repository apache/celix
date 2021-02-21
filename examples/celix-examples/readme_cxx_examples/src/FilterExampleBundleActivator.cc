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

#include <iostream>
#include "celix/BundleActivator.h"
#include "celix/IShellCommand.h"

class HelloWorldShellCommand : public celix::IShellCommand {
public:
    void executeCommand(const std::string& /*commandLine*/, const std::vector<std::string>& /*commandArgs*/, FILE* outStream, FILE* /*errorStream*/) {
        fprintf(outStream, "Hello World\n");
    }
};

class FilterExampleBundleActivator {
public:
    explicit FilterExampleBundleActivator(const std::shared_ptr<celix::BundleContext>& ctx) {
        auto reg1 = ctx->registerService<celix::IShellCommand>(std::make_shared<HelloWorldShellCommand>())
                .addProperty(celix::IShellCommand::COMMAND_NAME, "command1")
                .build();
        auto reg2 = ctx->registerService<celix::IShellCommand>(std::make_shared<HelloWorldShellCommand>())
                .addProperty(celix::IShellCommand::COMMAND_NAME, "command2")
                .build();
        regs.push_back(reg1);
        regs.push_back(reg2);

        auto serviceIdsNoFilter  = ctx->findServices<celix::IShellCommand>();
        auto serviceIdsWithFilter = ctx->findServices<celix::IShellCommand>(std::string{"("} + celix::IShellCommand::COMMAND_NAME + "=" + "command1)");
        std::cout << "Found " << std::to_string(serviceIdsNoFilter.size()) << " IShelLCommand services and found ";
        std::cout << std::to_string(serviceIdsWithFilter.size()) << " IShellCommand service with name command1" << std::endl;
    }
private:
    std::vector<std::shared_ptr<celix::ServiceRegistration>> regs{};
};

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(FilterExampleBundleActivator)