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

    void version(std::shared_ptr<celix::BundleContext> ctx, const std::string &, const std::vector<std::string> &, std::ostream &out, std::ostream &err) {
        if (ctx->bundle()->hasCacheEntry("version.properties")) {
            auto path = ctx->bundle()->absPathForCacheEntry("version.properties");
            celix::Properties versionInfo = celix::loadProperties(path);
            std::string version = celix::getProperty(versionInfo, "CELIX_VERSION", "");
            out << "Celix Version: " << version << std::endl;
        } else {
            err << "Cannot find version.properties entry in the " << ctx->bundle()->group() << " " << ctx->bundle()->name() << " bundle" << std::endl;
        }
    }
}


celix::ServiceRegistration impl::registerVersion(std::shared_ptr<celix::BundleContext> ctx) {
    using namespace std::placeholders;
    celix::ShellCommandFunction cmd = std::bind(&version, ctx, _1, _2, _3, _4);

    celix::Properties props{};
    props[celix::SHELL_COMMAND_FUNCTION_COMMAND_NAME] = "version";
    props[celix::SHELL_COMMAND_FUNCTION_COMMAND_USAGE] = "version";
    props[celix::SHELL_COMMAND_FUNCTION_COMMAND_DESCRIPTION] = "Show version information about the framework (TODO and installed bundles)";
    return ctx->registerFunctionService(celix::SHELL_COMMAND_FUNCTION_SERVICE_FQN, std::move(cmd), std::move(props));
}