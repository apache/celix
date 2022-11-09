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

//src/ComponentWithProvidedServiceActivator.cc
#include <celix/BundleActivator.h>
#include <celix/IShellCommand.h>
#include <celix_shell_command.h>

class ComponentWithProvidedService : public celix::IShellCommand { // <----------------------------------------------<1>
public:
    ~ComponentWithProvidedService() noexcept override = default;

    void executeCommand(
            const std::string& commandLine,
            const std::vector<std::string>& /*commandArgs*/,
            FILE* outStream,
            FILE* /*errorStream*/) override {
        fprintf(outStream, "Hello from cmp. C++ command called %i times. commandLine is %s\n",
                cxxCallCount++, commandLine.c_str());
    } // <-----------------------------------------------------------------------------------------------------------<2>

    void executeCCommand(const char* commandLine, FILE* outStream) {
        fprintf(outStream, "Hello from cmp. C command called %i times. commandLine is %s\n", cCallCount++, commandLine);
    } // <-----------------------------------------------------------------------------------------------------------<3>
private:
    std::atomic<int> cxxCallCount{1};
    std::atomic<int> cCallCount{1};
};

class ComponentWithProvidedServiceActivator {
public:
    explicit ComponentWithProvidedServiceActivator(const std::shared_ptr<celix::BundleContext>& ctx) {
        auto& cmp = ctx->getDependencyManager()->createComponent<ComponentWithProvidedService>(); // <---------------<4>

        cmp.createProvidedService<celix::IShellCommand>()
                .addProperty(celix::IShellCommand::COMMAND_NAME, "HelloComponent"); // <-----------------------------<5>

        auto shellCmd = std::make_shared<celix_shell_command_t>();
        shellCmd->handle = static_cast<void*>(&cmp.getInstance());
        shellCmd->executeCommand = [](void* handle, const char* commandLine, FILE* outStream, FILE*) -> bool {
            auto* impl = static_cast<ComponentWithProvidedService*>(handle);
            impl->executeCCommand(commandLine, outStream);
            return true;
        }; // <------------------------------------------------------------------------------------------------------<6>

        cmp.createUnassociatedProvidedService(std::move(shellCmd), CELIX_SHELL_COMMAND_SERVICE_NAME)
                .addProperty(CELIX_SHELL_COMMAND_NAME, "hello_component"); // < -------------------------------------<7>

        cmp.build(); // <--------------------------------------------------------------------------------------------<8>
    }
private:
};

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(ComponentWithProvidedServiceActivator)
