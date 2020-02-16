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

#include "celix/IShellCommand.h"

namespace {
    class LbCommand : public celix::IShellCommand {
    public:
        LbCommand(std::shared_ptr<celix::BundleContext> _ctx) : ctx{std::move(_ctx)} {}

        void executeCommand(const std::string &, const std::vector<std::string> &cmdArgs, std::ostream &out,
                            std::ostream &) noexcept override {
            if (cmdArgs.empty()) {
                out << "Bundles: " << std::endl;
                ctx->useBundles([&out](const celix::IBundle &bnd) {
                    //TODO make aligned text table
                    out << "|- " << bnd.id() << ": " << bnd.name() << std::endl;
                }, true);
            }
            //TODO parse args
        }

    private:
        std::shared_ptr<celix::BundleContext> ctx;
    };
}

celix::ServiceRegistration impl::registerLb(std::shared_ptr<celix::BundleContext> ctx) {
    celix::Properties props{};
    props[celix::IShellCommand::COMMAND_NAME] = "lb";
    props[celix::IShellCommand::COMMAND_USAGE] = "list bundles. Default only the groupless bundles are listed. Use -a to list all bundles." \
                            "\nIf a group string is provided only bundles matching the group string will be listed." \
                            "\nUse -l to print the bundle locations.\nUse -s to print the bundle symbolic names\nUse -u to print the bundle update location.";
    props[celix::IShellCommand::COMMAND_DESCRIPTION] = "lb [-l | -s | -u | -a] [group]";
    return ctx->registerService(std::shared_ptr<celix::IShellCommand>{new LbCommand{ctx}}, std::move(props));
}