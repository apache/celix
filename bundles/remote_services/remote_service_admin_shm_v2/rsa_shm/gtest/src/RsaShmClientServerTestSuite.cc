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
#include <celix_shell_command.h>
#include <celix_api.h>
#include <gtest/gtest.h>

class RsaShmClientServerTestSuite : public ::testing::Test {
public:
    RsaShmClientServerTestSuite() {
        celix_properties_t *serverProps = celix_properties_load("server.properties");
        EXPECT_TRUE(serverProps != NULL);
        sfw = std::shared_ptr<celix_framework_t>{celix_frameworkFactory_createFramework(serverProps),
            [](auto* f) {celix_frameworkFactory_destroyFramework(f);}};
        sctx = std::shared_ptr<celix_bundle_context_t>{celix_framework_getFrameworkContext(sfw.get()),
            [](auto*){/*nop*/}};

        celix_properties_t *clientProps = celix_properties_load("client.properties");
        EXPECT_TRUE(clientProps != NULL);
        cfw = std::shared_ptr<celix_framework_t>{celix_frameworkFactory_createFramework(clientProps),
            [](auto* f) {celix_frameworkFactory_destroyFramework(f);}};
        cctx = std::shared_ptr<celix_bundle_context_t>{celix_framework_getFrameworkContext(cfw.get()),
            [](auto*){/*nop*/}};
    }

    std::shared_ptr<celix_framework_t> sfw{};
    std::shared_ptr<celix_bundle_context_t> sctx{};
    std::shared_ptr<celix_framework_t> cfw{};
    std::shared_ptr<celix_bundle_context_t> cctx{};
};

void rsaJsonRpcTestSuite_useCmd(void *handle, void *svc) {
    (void)handle;
    auto *cmd = static_cast<celix_shell_command_t *>(svc);
    bool executed = cmd->executeCommand(cmd->handle, "add 1 2", stdout, stderr);
    EXPECT_TRUE(executed);
}

TEST_F(RsaShmClientServerTestSuite, CallRemoteService) {
    celix_service_use_options_t opts{};
    opts.filter.serviceName = CELIX_SHELL_COMMAND_SERVICE_NAME;
    opts.callbackHandle = this;
    opts.use = rsaJsonRpcTestSuite_useCmd;
    opts.flags = CELIX_SERVICE_USE_DIRECT | CELIX_SERVICE_USE_SOD;
    auto found = celix_bundleContext_useServiceWithOptions(cctx.get(), &opts);
    EXPECT_TRUE(found);
}
