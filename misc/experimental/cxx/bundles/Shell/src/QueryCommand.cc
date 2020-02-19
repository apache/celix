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
#include <unordered_set>

#include "celix/Constants.h"
#include "celix/IShellCommand.h"

namespace {

    void query(std::shared_ptr<celix::BundleContext> ctx, const std::string &/*cmdName*/, const std::vector<std::string> &cmdArgs, std::ostream &out, std::ostream &) {

        std::vector<std::string> svcNames{};
        std::vector<celix::Filter> filters{};
        std::unordered_set<long> bndIds{};
        bool verbose = false;

        for (const auto &arg : cmdArgs) {
            if (arg == "-v" ) {
                verbose = true;
                continue;
            }
            try {
                long id = std::stol(arg);
                bndIds.insert(id);
            } catch (std::invalid_argument &) {
                //no long -> check it is a filter
                try {
                    celix::Filter f{arg};
                    filters.push_back(std::move(f));
                } catch(std::invalid_argument &) {
                    //no filter -> service name
                    svcNames.push_back(arg);
                }
            }
        }

        auto reg = ctx->bundle()->framework().registry();

        const auto fun = [&](const std::shared_ptr<void>&, const celix::Properties& props, const celix::IResourceBundle&) {
            long bndId = celix::getPropertyAsLong(props, celix::SERVICE_BUNDLE_ID, -1L); //TODO, FIXME bundle.id not yet registered on services
            bool print = bndIds.empty() || bndIds.find(bndId) != bndIds.end();
            if (print) {
                out << "|- Service: " << celix::getProperty(props, celix::SERVICE_NAME, "!Error") << std::endl;
                if (verbose) {
                    for (auto &pair : props) {
                        out << "   |- " << pair.first << " = " << pair.second << std::endl;
                    }
                }
            }
        };

        for (const auto &svcName : svcNames) {
            out << "Result '" << svcName << "':" << std::endl;
            reg->useAnyServices(svcName, celix::Filter{}, fun, ctx->bundle());
        }
        for (const auto &filter: filters) {
            out << "Result '" << filter.toString() << "':" << std::endl;
            reg->useAnyServices({}, filter, fun, ctx->bundle());
        }
        if (svcNames.empty() && filters.empty()) {
            //TODO handle verbose options
            auto names = ctx->registry()->listAllRegisteredServiceNames();
            out << "Available Service Names:" << std::endl;
            for (auto &name : names) {
                out << "|- " << name << std::endl;
            }
        }
    }
}


celix::ServiceRegistration celix::impl::registerQuery(std::shared_ptr<celix::BundleContext> ctx) {
    using namespace std::placeholders;
    celix::ShellCommandFunction cmd = std::bind(&query, ctx, _1, _2, _3, _4);

    celix::Properties props{};
    props[celix::SHELL_COMMAND_FUNCTION_COMMAND_NAME] = "query";
    props[celix::SHELL_COMMAND_FUNCTION_COMMAND_USAGE] = "query [-v] [bndId ..] [serviceName|serviceFilter ...]";
    props[celix::SHELL_COMMAND_FUNCTION_COMMAND_DESCRIPTION] = "Query the service registry. If no arguments are provided list the available services names.";
    return ctx->registerFunctionService(celix::SHELL_COMMAND_FUNCTION_SERVICE_FQN, std::move(cmd), std::move(props));
}