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

#include "celix/api.h"
#include "celix/IShellCommand.h"

namespace {

    void stopOrStart(std::shared_ptr<celix::BundleContext> ctx, const std::string &cmdName, const std::vector<std::string> &cmdArgs, std::ostream &out, std::ostream &err) {
        if (cmdArgs.empty()) {
            out << "Provide a bundle id name to " << cmdName << std::endl;
        } else {
            for (const auto &bndId : cmdArgs) {
                bool isNum = true;
                for(const char &c : bndId) {
                    isNum = isNum && isdigit(c);
                }

                if (isNum) {
                    long id = atoi(bndId.c_str());
                    if (cmdName == "stop") {
                        ctx->stopBundle(id);
                    } else {
                        ctx->startBundle(id);
                    }
                } else {
                    err << "Cannot parse '" << bndId << "' to bundle id" << std::endl;
                }
            }
        }
    }
}


celix::ServiceRegistration impl::registerStop(std::shared_ptr<celix::BundleContext> ctx) {
    using namespace std::placeholders;
    celix::ShellCommandFunction stop = std::bind(&stopOrStart, ctx, _1, _2, _3, _4);

    celix::Properties props{};
    props[celix::SHELL_COMMAND_FUNCTION_COMMAND_NAME] = "stop";
    props[celix::SHELL_COMMAND_FUNCTION_COMMAND_USAGE] = "stop (bndId)+";
    props[celix::SHELL_COMMAND_FUNCTION_COMMAND_DESCRIPTION] = "Stops the provided bundles, identified by the bundle ids";
    return ctx->registerFunctionService(celix::SHELL_COMMAND_FUNCTION_SERVICE_FQN, std::move(stop), std::move(props));
}

celix::ServiceRegistration impl::registerStart(std::shared_ptr<celix::BundleContext> ctx) {
    using namespace std::placeholders;
    celix::ShellCommandFunction stop = std::bind(&stopOrStart, ctx, _1, _2, _3, _4);

    celix::Properties props{};
    props[celix::SHELL_COMMAND_FUNCTION_COMMAND_NAME] = "start";
    props[celix::SHELL_COMMAND_FUNCTION_COMMAND_USAGE] = "start (bndId)+";
    props[celix::SHELL_COMMAND_FUNCTION_COMMAND_DESCRIPTION] = "Starts the provided bundles, identified by the bundle ids";
    return ctx->registerFunctionService(celix::SHELL_COMMAND_FUNCTION_SERVICE_FQN, std::move(stop), std::move(props));
}