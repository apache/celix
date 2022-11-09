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

//src/ComponentWithServiceDependencyActivator.cc
#include <celix/BundleActivator.h>
#include <celix/IShellCommand.h>
#include <celix_shell_command.h>

class ComponentWithServiceDependency {
public:
    void setHighestRankingShellCommand(const std::shared_ptr<celix::IShellCommand>& cmdSvc) {
        std::cout << "New highest ranking service (can be NULL): " << (intptr_t)cmdSvc.get() << std::endl;
        highestRankingShellCmd = cmdSvc;
    }

    void addCShellCmd(
            const std::shared_ptr<celix_shell_command_t>& cmdSvc,
            const std::shared_ptr<const celix::Properties>& props) {
        auto id = props->getAsLong(celix::SERVICE_ID, -1);
        std::cout << "Adding shell command service with service.id: " << id << std::endl;
        std::lock_guard lck{mutex};
        shellCommands.emplace(id, cmdSvc);
    }

    void removeCShellCmd(
            const std::shared_ptr<celix_shell_command_t>& /*cmdSvc*/,
            const std::shared_ptr<const celix::Properties>& props) {
        auto id = props->getAsLong(celix::SERVICE_ID, -1);
        std::cout << "Removing shell command service with service.id: " << id << std::endl;
        std::lock_guard lck{mutex};
        shellCommands.erase(id);
    }
private:
    std::shared_ptr<celix::IShellCommand> highestRankingShellCmd{};
    std::mutex mutex{}; //protect below
    std::unordered_map<long, std::shared_ptr<celix_shell_command_t>> shellCommands{};
};

class ComponentWithServiceDependencyActivator {
public:
    explicit ComponentWithServiceDependencyActivator(const std::shared_ptr<celix::BundleContext>& ctx) {
        using Cmp = ComponentWithServiceDependency;
        auto& cmp = ctx->getDependencyManager()->createComponent<Cmp>(); // <-------------<1>

        cmp.createServiceDependency<celix::IShellCommand>()
                .setCallbacks(&Cmp::setHighestRankingShellCommand)
                .setRequired(true)
                .setStrategy(DependencyUpdateStrategy::suspend);

        cmp.createServiceDependency<celix_shell_command_t>(CELIX_SHELL_COMMAND_SERVICE_NAME)
                .setCallbacks(&Cmp::addCShellCmd, &Cmp::removeCShellCmd)
                .setRequired(false)
                .setStrategy(DependencyUpdateStrategy::locking);

        cmp.build(); // <--------------------------------------------------------------------------------------------<8>
    }
};

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(ComponentWithServiceDependencyActivator)
