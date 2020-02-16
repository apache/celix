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

    void inspect(std::shared_ptr<celix::BundleContext> ctx, const std::string &, const std::vector<std::string> &cmdArgs, std::ostream &out, std::ostream &) {
        if (cmdArgs.empty()) {
            out << "Provide a bundle id" << std::endl;
        } else {
            auto &bndId = cmdArgs[0];
            auto servicesNames = ctx->registry().listAllRegisteredServiceNames();

            const std::string *what{nullptr};
            if (cmdArgs.size() >= 2) {
                what = &cmdArgs[1];
            }

            if (what == nullptr || *what == "provided") {
                out << "Provided Services: \n";
                for (auto &svcName : servicesNames) {
                    std::string filter = std::string{"("} + celix::SERVICE_BUNDLE + "=" + bndId + ")";
                    ctx->registry().useAnyServices(svcName, [&out](std::shared_ptr<void>, const celix::Properties &props, const celix::IResourceBundle &) {
                        out << "|- Service " << celix::getProperty(props, celix::SERVICE_ID, "!Error") << ":\n";
                        for (auto &pair : props) {
                            out << "   |- " << pair.first << " = " << pair.second << std::endl;
                        }
                    }, filter, ctx->bundle());
                }
            }
            if (what == nullptr || *what == "tracked") {
                //TODO trackers
            }
        }
    }
}


celix::ServiceRegistration impl::registerInspect(std::shared_ptr<celix::BundleContext> ctx) {
    using namespace std::placeholders;
    celix::ShellCommandFunction cmd = std::bind(&inspect, ctx, _1, _2, _3, _4);

    celix::Properties props{};
    props[celix::SHELL_COMMAND_FUNCTION_COMMAND_NAME] = "inspect";
    props[celix::SHELL_COMMAND_FUNCTION_COMMAND_USAGE] = "inspect bndId [provided|tracked]";
    props[celix::SHELL_COMMAND_FUNCTION_COMMAND_DESCRIPTION] = "Inspects a bundle. Showing the provided and/or tracked services";
    return ctx->registerFunctionService(celix::SHELL_COMMAND_FUNCTION_SERVICE_FQN, std::move(cmd), std::move(props));
}