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

#include <string>
#include <vector>
#include <iostream>

#pragma once

namespace celix {

    class IShellCommand {
    public:
        static constexpr const char * const NAME = "[celix::IShellCommand] [version 1]";

        static constexpr const char * const COMMAND_NAME = "command.name";
        static constexpr const char * const COMMAND_USAGE = "command.usage";
        static constexpr const char * const COMMAND_DESCRIPTION = "command.description";

        virtual ~IShellCommand() = default;

        virtual void executeCommand(const std::string &cmdName, const std::vector<std::string> &cmdArgs, std::ostream &out, std::ostream &err) = 0;
    };

    static constexpr const char * const SHELL_COMMAND_FUNCTION_SERVICE_NAME = "[celix::ShellCommandFunction] [version 1]";
    static constexpr const char * const SHELL_COMMAND_FUNCTION_COMMAND_NAME = IShellCommand::COMMAND_NAME;
    static constexpr const char * const SHELL_COMMAND_FUNCTION_COMMAND_USAGE = IShellCommand::COMMAND_USAGE;
    static constexpr const char * const SHELL_COMMAND_FUNCTION_COMMAND_DESCRIPTION = IShellCommand::COMMAND_DESCRIPTION;
    using ShellCommandFunction = std::function<void(const std::string &cmdName, const std::vector<std::string> &cmdArgs, std::ostream &out, std::ostream &err)>;
}
