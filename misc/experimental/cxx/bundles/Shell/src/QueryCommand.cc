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

#include "celix/Constants.h"
#include "celix/IShellCommand.h"

namespace {

    void query(std::shared_ptr<celix::BundleContext> ctx, const std::string &, const std::vector<std::string> &cmdArgs, std::ostream &out, std::ostream &) {
        if (cmdArgs.empty()) {
            auto names = ctx->registry().listAllRegisteredServiceNames();
            out << "Available Service Names:" << std::endl;
            for (auto &name : names) {
                out << "|- " << name << std::endl;
            }
        } else if (cmdArgs.size() == 1) {
            //need no of minimal 2 args
            out << "Provide a service name and filter" << std::endl;
        } else {
            std::string lang{};
            std::string svcName{};
            std::string filter{};
            if (cmdArgs.size() >= 3) {
                lang = cmdArgs[0];
                svcName = cmdArgs[1];
                filter = cmdArgs[2];
            } else {
                svcName = cmdArgs[0];
                filter = cmdArgs[1];
            }


            auto reg = ctx->bundle()->framework().registry();
            out << "Result '" << svcName << " " << filter << "':" << std::endl;
            const auto fun = [&](const std::shared_ptr<void>&, const celix::Properties& props, const celix::IResourceBundle&) {
                out << "|- Service: " << celix::getProperty(props, celix::SERVICE_NAME, "!Error") << std::endl;
                for (auto &pair : props) {
                    out << "   |- " << pair.first << " = " << pair.second << std::endl;
                }
            };
            reg->useAnyServices(svcName, fun, filter, ctx->bundle());
        }
    }
}


celix::ServiceRegistration impl::registerQuery(std::shared_ptr<celix::BundleContext> ctx) {
    using namespace std::placeholders;
    celix::ShellCommandFunction cmd = std::bind(&query, ctx, _1, _2, _3, _4);

    celix::Properties props{};
    props[celix::SHELL_COMMAND_FUNCTION_COMMAND_NAME] = "query";
    props[celix::SHELL_COMMAND_FUNCTION_COMMAND_USAGE] = "query [lang] [serviceName serviceFilter]";
    props[celix::SHELL_COMMAND_FUNCTION_COMMAND_DESCRIPTION] = "Query the service registry. If no arguments are provided list the available services names.";
    return ctx->registerFunctionService(celix::SHELL_COMMAND_FUNCTION_SERVICE_FQN, std::move(cmd), std::move(props));
}