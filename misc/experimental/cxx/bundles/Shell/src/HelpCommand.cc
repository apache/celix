/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */

#include "commands.h"

#include "celix/api.h"
#include "celix/IShellCommand.h"


namespace {

    void help(std::shared_ptr<celix::BundleContext> ctx, const std::string &, const std::vector<std::string> &commandArguments, std::ostream &out, std::ostream &) {

        if (commandArguments.empty()) { //only command -> overview

            std::string hasCommandNameFilter = std::string{"("} + celix::IShellCommand::COMMAND_NAME + "=*)";
            std::vector<std::string> commands{};
            ctx->useServices<celix::IShellCommand>([&](celix::IShellCommand &, const celix::Properties &props) {
                commands.push_back(celix::getProperty(props, celix::IShellCommand::COMMAND_NAME, "!Error!"));
            }, hasCommandNameFilter);

            hasCommandNameFilter = std::string{"("} + celix::SHELL_COMMAND_FUNCTION_COMMAND_NAME + "=*)";

            std::function<void(celix::ShellCommandFunction &, const celix::Properties &)> use = [&](
                    celix::ShellCommandFunction &, const celix::Properties &props) {
                commands.push_back(celix::getProperty(props, celix::SHELL_COMMAND_FUNCTION_COMMAND_NAME, "!Error!"));
            };
            ctx->useFunctionServices(celix::SHELL_COMMAND_FUNCTION_SERVICE_FQN, use, hasCommandNameFilter);

            //TODO useCService with a shell command service struct

            out << "Available commands: " << std::endl;
            for (auto &name : commands) {
                out << "|- " << name << std::endl;
            }
        } else { //details
            for (auto &cmd : commandArguments) {
                std::string commandNameFilter = std::string{"("} + celix::IShellCommand::COMMAND_NAME + "=" + cmd + ")";
                bool found = ctx->useService<celix::IShellCommand>([&](celix::IShellCommand &, const celix::Properties &props) {
                    out << "Command Name       : " << celix::getProperty(props, celix::IShellCommand::COMMAND_NAME, "!Error!") << std::endl;
                    out << "Command Usage      : " << celix::getProperty(props, celix::IShellCommand::COMMAND_USAGE, "!Error!") << std::endl;
                    out << "Command Description: " << celix::getProperty(props, celix::IShellCommand::COMMAND_DESCRIPTION, "!Error!") << std::endl;

                }, commandNameFilter);
                if (!found) {
                    commandNameFilter = std::string{"("} + celix::SHELL_COMMAND_FUNCTION_COMMAND_NAME + "=" + cmd + ")";
                    std::function<void(celix::ShellCommandFunction &, const celix::Properties &)> use = [&](celix::ShellCommandFunction &, const celix::Properties &props) {
                        out << "Command Name       : " << celix::getProperty(props, celix::SHELL_COMMAND_FUNCTION_COMMAND_NAME, "!Error!") << std::endl;
                        out << "Command Usage      : " << celix::getProperty(props, celix::SHELL_COMMAND_FUNCTION_COMMAND_USAGE, "!Error!") << std::endl;
                        out << "Command Description: " << celix::getProperty(props, celix::SHELL_COMMAND_FUNCTION_COMMAND_DESCRIPTION, "!Error!") << std::endl;
                    };
                    found = ctx->useFunctionService(celix::SHELL_COMMAND_FUNCTION_SERVICE_FQN, use, commandNameFilter);
                }
                if (!found) {
                    //TODO use C cmd service
                }
                if (!found) {
                    out << "Command '" << cmd << "' not available" << std::endl;
                }
                out << std::endl;
            }
        }
    }
}

celix::ServiceRegistration impl::registerHelp(std::shared_ptr<celix::BundleContext> ctx) {
    using namespace std::placeholders;
    celix::ShellCommandFunction cmd = std::bind(&help, ctx, _1, _2, _3, _4);

    celix::Properties props{};
    props[celix::SHELL_COMMAND_FUNCTION_COMMAND_NAME] = "help";
    props[celix::SHELL_COMMAND_FUNCTION_COMMAND_USAGE] = "help [command name]";
    props[celix::SHELL_COMMAND_FUNCTION_COMMAND_DESCRIPTION] = "display available commands and description.";
    return ctx->registerFunctionService(celix::SHELL_COMMAND_FUNCTION_SERVICE_FQN, std::move(cmd), std::move(props));
}