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

#pragma once

#include <string>
#include <vector>

namespace celix {

    /**
     * The shell command interface can be used to register additional Celix shell commands.
     * This service should be registered with the following properties:
     *  - name: mandatory, name of the command e.g. 'celix::lb'
     *  - usage: optional, string describing how tu use the command e.g. 'celix::lb [-l | -s | -u]'
     *  - description: optional, string describing the command e.g. 'list bundles.'
     */
    class IShellCommand {
    public:
        /**
         * The required name of the shell command service (service property)
         */
        static constexpr const char * const COMMAND_NAME = "name";

        /**
         * The optional usage text of the shell command service (service property)
         */
        static constexpr const char * const COMMAND_USAGE = "usage";

        /**
         * The optional description text of the shell command service (service property)
         */
        static constexpr const char * const COMMAND_DESCRIPTION = "description";

        virtual ~IShellCommand() noexcept = default;

        /**
         * Calls the shell command.
         * @param commandLine   The complete provided command line (e.g. for a 'stop' command -> 'stop 42'). Only valid during the call.
         * @param commandArgs   A list of the arguments for the command (e.g. for a "stop 42 43" commandLine -> {"42", "43"}). Only valid during the call.
         * @param outStream     The C output stream, to use for printing normal flow info.
         * @param errorStream   The C error stream, to use for printing error flow info.
         * @return              Whether the command has been executed correctly.
         */
        virtual void executeCommand(const std::string& commandLine, const std::vector<std::string>& commandArgs, FILE* outStream, FILE* errorStream) = 0;
    };
}