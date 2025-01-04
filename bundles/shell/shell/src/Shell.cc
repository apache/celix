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

#include <memory>
#include <unordered_map>

#include "celix/BundleActivator.h"
#include "celix/IShellCommand.h"
#include "std_commands.h"
#include "celix_shell_command.h"
#include "celix_log_service.h"
#include "celix_shell_command.h"
#include "celix_shell.h"

namespace celix {

    class Shell {
    public:

        /**
         * Try to execute a command using the provided command line.
         */
        celix_status_t executeCommand(const char *commandLine, FILE *out, FILE *err) {
            std::vector<std::string> tokens = tokenize(commandLine);
            if (tokens.empty()) {
                fprintf(err, "Invalid commandLine '%s'\n", commandLine);
                return CELIX_ILLEGAL_ARGUMENT;
            }

            Entry entry = findCommand(tokens[0], err);
            if (entry.cCommand) {
                 bool successful = entry.cCommand->executeCommand(entry.cCommand->handle, commandLine, out, err);
                 return successful ? CELIX_SUCCESS : CELIX_SERVICE_EXCEPTION;
            } else if (entry.cxxCommand) {
                std::vector<std::string> arguments{};
                auto start = ++tokens.begin();
                arguments.insert(arguments.begin(), start, tokens.end());
                entry.cxxCommand->executeCommand(commandLine, arguments, out, err);
                return CELIX_SUCCESS;
            } else {
                fprintf(err, "No command '%s'. Provided command line: %s\n", tokens[0].c_str(), commandLine);
                return CELIX_ILLEGAL_ARGUMENT;
            }
        }

        /**
         * List the registered command names. Caller is owner of the commands.
         * @return A celix array list with char*.
         */
        celix_status_t getCommands(celix_array_list_t **commands) {
            auto* result = celix_arrayList_createPointerArray();
            std::lock_guard<std::mutex> lck{mutex};
            for (const auto& pair : entries) {
                celix_arrayList_add(result, celix_utils_strdup(pair.first.c_str()));
            }
            *commands = result;
            return CELIX_SUCCESS;
        }

        /**
         * Gets the usage info for the provided command str. Caller is owner.
         */
        celix_status_t getCommandUsage(const char *commandName, char **usageStr) {
            std::lock_guard<std::mutex> lck{mutex};
            auto it = entries.find(commandName);
            if (it != entries.end()) {
                std::string str = it->second.cCommand ?
                        it->second.properties->get(CELIX_SHELL_COMMAND_USAGE, "") :
                        it->second.properties->get(celix::IShellCommand::COMMAND_USAGE, "");
                *usageStr = celix_utils_strdup(str.c_str());
            } else {
                *usageStr = nullptr;
            }
            return CELIX_SUCCESS;
        }

        /**
         * Gets the usage info for the provided command str. Caller is owner.
         */
        celix_status_t getCommandDescription(const char *commandName, char **commandDescription) {
            std::lock_guard<std::mutex> lck{mutex};
            auto it = entries.find(commandName);
            if (it != entries.end()) {
                std::string str = it->second.cCommand ?
                                  it->second.properties->get(CELIX_SHELL_COMMAND_DESCRIPTION, "") :
                                  it->second.properties->get(celix::IShellCommand::COMMAND_DESCRIPTION, "");
                *commandDescription = celix_utils_strdup(str.c_str());
            } else {
                *commandDescription = nullptr;
            }
            return CELIX_SUCCESS;
        }

        void addCCommand(const std::shared_ptr<celix_shell_command>& command, const std::shared_ptr<const celix::Properties>& properties) {
            addEntry(command, {}, properties);
        }

        void remCCommand(const std::shared_ptr<celix_shell_command>& command, const std::shared_ptr<const celix::Properties>& properties) {
            remEntry(command, {}, properties);
        }

        void addCxxCommand(const std::shared_ptr<celix::IShellCommand>& command, const std::shared_ptr<const celix::Properties>& properties) {
            addEntry({}, command, properties);
        }

        void remCxxCommand(const std::shared_ptr<celix::IShellCommand>& command, const std::shared_ptr<const celix::Properties>& properties) {
            remEntry({}, command, properties);
        }

        void setLogService(const std::shared_ptr<celix_log_service> ls) {
            std::lock_guard<std::mutex> lck{mutex};
            logService = ls;
        }
    private:
        struct Entry {
            long svcId;
            std::string name; //original name (fully qualified, e.g. "celix::lb")
            std::string localName; //only the local part of a name (e.g. "lb")
            std::shared_ptr<celix_shell_command> cCommand;
            std::shared_ptr<celix::IShellCommand> cxxCommand;
            std::shared_ptr<const celix::Properties> properties;
        };

        void addEntry(
                const std::shared_ptr<celix_shell_command>& cCommand,
                const std::shared_ptr<celix::IShellCommand>& cxxCommand,
                const std::shared_ptr<const celix::Properties>& properties) {
            std::lock_guard<std::mutex> lck{mutex};
            std::string name = cCommand ?
                    properties->get(CELIX_SHELL_COMMAND_NAME, "") :
                    properties->get(celix::IShellCommand::COMMAND_NAME, "");
            long svcId = properties->getAsLong(celix::SERVICE_ID, -1L);
            auto it = entries.find(name);
            if (it == entries.end()) {
                auto* nsBegin = strstr(name.c_str(), "::");
                std::string localName = nsBegin == nullptr ? name : std::string{nsBegin + 2};
                entries.emplace(name, Entry{.svcId=svcId, .name=name, .localName=localName, .cCommand=cCommand, .cxxCommand=cxxCommand, .properties=properties});
            } else if (logService){
                logService->warning(logService->handle, "Cannot add command with name %s. A command with this name already exists", name.c_str());
            }
        }

        void remEntry(
                const std::shared_ptr<celix_shell_command>& cCommand,
                const std::shared_ptr<celix::IShellCommand>& /*cxxCommand*/,
                const std::shared_ptr<const celix::Properties>& properties) {
            std::lock_guard<std::mutex> lck{mutex};
            std::string name = cCommand ?
                               properties->get(CELIX_SHELL_COMMAND_NAME, "") :
                               properties->get(celix::IShellCommand::COMMAND_NAME, "");
            long svcId = properties->getAsLong(celix::SERVICE_ID, -1L);
            auto it = entries.find(name);
            if (it != entries.end() && it->second.svcId == svcId) {
                entries.erase(it);
            }
        }

        std::vector<std::string> tokenize(const char* input) {
            std::vector<std::string> result{};
            char* str = celix_utils_strdup(input);
            char* savePtr = nullptr;
            char* token = strtok_r(str, " ", &savePtr);
            while (token != nullptr) {
                result.emplace_back(token);
                token = strtok_r(nullptr, " ", &savePtr);
            }
            free(str);
            return result;
        }

        Entry findCommand(const std::string& commandName, FILE * err) {
            std::lock_guard<std::mutex> lck{mutex};
            auto it = entries.find(commandName);
            if (it != entries.end()) {
                return it->second;
            }

            //not found, check if entry can be found without namespace
            int nrFound = 0;
            for (const auto& pair : entries) {
                if (pair.second.localName == commandName) {
                    nrFound += 1;
                }
            }
            if (nrFound == 1) {
                for (const auto& pair : entries) {
                    if (pair.second.localName == commandName) {
                        return pair.second;
                    }
                }
            } else if (nrFound > 1) {
                fprintf(err, "Found %i nr commands for command name %s. Please use a fully qualified name\n", nrFound, commandName.c_str());
            }
            return Entry{.svcId = -1, .name = "", .localName = "", .cCommand = {}, .cxxCommand = {}, .properties = {}};;
        }

        std::mutex mutex{}; //protects below
        std::unordered_map<std::string, Entry> entries{};
        std::shared_ptr<celix_log_service> logService{};
    };

    class ShellActivator {
    public:
        explicit ShellActivator(const std::shared_ptr <celix::BundleContext> &ctx) : commands{createCommands(ctx)} {
            auto trk1 = ctx->trackServices<celix_shell_command>(CELIX_SHELL_COMMAND_SERVICE_NAME)
                    .addAddWithPropertiesCallback([this](const std::shared_ptr<celix_shell_command>& cmd, const std::shared_ptr<const celix::Properties>& properties){
                        shell.addCCommand(cmd, properties);
                    })
                    .addRemWithPropertiesCallback([this](const std::shared_ptr<celix_shell_command>& cmd, const std::shared_ptr<const celix::Properties>& properties){
                        shell.remCCommand(cmd, properties);
                    })
                    .build();
            trackers.push_back(std::move(trk1));

            auto trk2 = ctx->trackServices<celix::IShellCommand>()
                    .addAddWithPropertiesCallback([this](const std::shared_ptr<celix::IShellCommand>& cmd, const std::shared_ptr<const celix::Properties>& properties){
                        shell.addCxxCommand(cmd, properties);
                    })
                    .addRemWithPropertiesCallback([this](const std::shared_ptr<celix::IShellCommand>& cmd, const std::shared_ptr<const celix::Properties>& properties){
                        shell.remCxxCommand(cmd, properties);
                    })
                    .build();
            trackers.push_back(std::move(trk2));

            auto trk3 = ctx->trackServices<celix_log_service>()
                    .addSetCallback([this](const std::shared_ptr<celix_log_service>& logService){
                        shell.setLogService(logService);
                    })
                    .build();
            trackers.push_back(std::move(trk3));

            shellSvc.handle = this;
            shellSvc.executeCommand = [](void *handle, const char *commandLine, FILE *out, FILE *err) -> celix_status_t {
                auto* act = static_cast<ShellActivator*>(handle);
                return act->shell.executeCommand(commandLine, out, err);
            };
            shellSvc.getCommands = [](void *handle, celix_array_list_t **commands) -> celix_status_t {
                auto* act = static_cast<ShellActivator*>(handle);
                return act->shell.getCommands(commands);
            };
            shellSvc.getCommandUsage = [](void *handle, const char *commandName, char **UsageStr) -> celix_status_t {
                auto* act = static_cast<ShellActivator*>(handle);
                return act->shell.getCommandUsage(commandName, UsageStr);
            };
            shellSvc.getCommandDescription = [](void *handle, const char *commandName, char **commandDescription) -> celix_status_t  {
                auto* act = static_cast<ShellActivator*>(handle);
                return act->shell.getCommandDescription(commandName, commandDescription);
            };
            shellSvcReg = ctx->registerUnmanagedService<celix_shell>(&shellSvc, CELIX_SHELL_SERVICE_NAME)
                    .build();
        }
    private:
        std::shared_ptr <celix_std_commands> createCommands(const std::shared_ptr <celix::BundleContext> &ctx) {
            return std::shared_ptr<celix_std_commands>
                   {celix_stdCommands_create(ctx->getCBundleContext()), [](celix_std_commands *cmds) {
                       celix_stdCommands_destroy(cmds);
                   }};
        }

        Shell shell{};
        celix_shell shellSvc{};
        std::vector<std::shared_ptr<celix::GenericServiceTracker>> trackers{};
        std::shared_ptr<celix::ServiceRegistration> shellSvcReg{};
        std::shared_ptr<celix_std_commands> commands;
    };
}

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(celix::ShellActivator)