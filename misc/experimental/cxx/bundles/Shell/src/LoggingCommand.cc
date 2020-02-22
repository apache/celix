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

#include <functional>
#include <spdlog/spdlog.h>

#include "celix/Api.h"
#include "celix/IShellCommand.h"

namespace {

    constexpr const char * const usage ="logging [[logger name] (trace|debug|info|warning|error|critical|off)]";

    void logger(const std::string &, const std::vector<std::string> &arguments, std::ostream &out, std::ostream &err) {
        if  (arguments.size() == 0) {
            out << "Loggers: " << std::endl;
            spdlog::apply_all([&](std::shared_ptr<spdlog::logger> logger) {
                const auto& name = logger->name().empty() ? "default" : logger->name();
                out << "|- " << name << ": " << spdlog::level::to_string_view(logger->level()).data() << std::endl;
            });
        } else if (arguments.size() == 1 || arguments.size() == 2) {
            auto &target = arguments.size() == 1 ? "" : arguments[0];
            auto &level = arguments.size() == 1 ? arguments[0] : arguments[1];
            spdlog::apply_all([&](std::shared_ptr<spdlog::logger> logger) {
                const auto& name = logger->name().empty() ? "default" : logger->name();
                if (target == "" || target == name) {
                    auto oldLevel = logger->level();
                    logger->set_level(spdlog::level::from_str(level));
                    auto newLevel = logger->level();
                    const auto* oStr = spdlog::level::to_string_view(oldLevel).data();
                    const auto* nStr = spdlog::level::to_string_view(newLevel).data();
                    if (oldLevel == newLevel) {
                        out << "Logger " << name << " not changed from level. level is " << nStr << "." << std::endl;
                    } else {
                        out << "Logger " << name << " changed from level " << oStr << " to " << nStr << "." << std::endl;
                    }
                }
            });
        } else {
            err << "Invalid args. Usage: " << usage << std::endl;
        }
    }
}


celix::ServiceRegistration celix::impl::registerLogging(const std::shared_ptr<celix::BundleContext>& ctx) {
    using namespace std::placeholders;
    celix::ShellCommandFunction cmd = std::bind(&logger, _1, _2, _3, _4);

    celix::Properties props{};
    props[celix::SHELL_COMMAND_FUNCTION_COMMAND_NAME] = "logging";
    props[celix::SHELL_COMMAND_FUNCTION_COMMAND_USAGE] = usage;
    props[celix::SHELL_COMMAND_FUNCTION_COMMAND_DESCRIPTION] = "logging command can be used to list all loggers and to configure the log level of the loggers";
    return ctx->registerFunctionService(celix::SHELL_COMMAND_FUNCTION_SERVICE_NAME, std::move(cmd), std::move(props));
}