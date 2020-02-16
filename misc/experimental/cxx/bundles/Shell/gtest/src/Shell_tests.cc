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

#include <sstream>

#include "gtest/gtest.h"

#include "celix/api.h"
#include "celix/IShellCommand.h"
#include "celix/IShell.h"

class ShellTest : public ::testing::Test {
public:
    ShellTest() {}
    ~ShellTest(){}

    celix::Framework& framework() { return fw; }
private:
    celix::Framework fw{};
};




TEST_F(ShellTest, AreCommandsAndShellRegistered) {

    std::string filter = std::string{"("} + celix::IShellCommand::COMMAND_NAME + "=lb)";
    long svcId = framework().context().findService<celix::IShellCommand>(filter);
    EXPECT_TRUE(svcId > 0);

    filter = std::string{"("} + celix::SHELL_COMMAND_FUNCTION_COMMAND_NAME + "=help)";
    svcId = framework().context().findFunctionService<celix::ShellCommandFunction>(celix::SHELL_COMMAND_FUNCTION_SERVICE_FQN, filter);
    EXPECT_TRUE(svcId > 0);

    filter = std::string{"("} + celix::SHELL_COMMAND_FUNCTION_COMMAND_NAME + "=inspect)";
    svcId = framework().context().findFunctionService<celix::ShellCommandFunction>(celix::SHELL_COMMAND_FUNCTION_SERVICE_FQN, filter);
    EXPECT_TRUE(svcId > 0);

    filter = std::string{"("} + celix::SHELL_COMMAND_FUNCTION_COMMAND_NAME + "=query)";
    svcId = framework().context().findFunctionService<celix::ShellCommandFunction>(celix::SHELL_COMMAND_FUNCTION_SERVICE_FQN, filter);
    EXPECT_TRUE(svcId > 0);

    filter = std::string{"("} + celix::SHELL_COMMAND_FUNCTION_COMMAND_NAME + "=stop)";
    svcId = framework().context().findFunctionService<celix::ShellCommandFunction>(celix::SHELL_COMMAND_FUNCTION_SERVICE_FQN, filter);
    EXPECT_TRUE(svcId > 0);

    filter = std::string{"("} + celix::SHELL_COMMAND_FUNCTION_COMMAND_NAME + "=start)";
    svcId = framework().context().findFunctionService<celix::ShellCommandFunction>(celix::SHELL_COMMAND_FUNCTION_SERVICE_FQN, filter);
    EXPECT_TRUE(svcId > 0);

    filter = std::string{"("} + celix::SHELL_COMMAND_FUNCTION_COMMAND_NAME + "=version)";
    svcId = framework().context().findFunctionService<celix::ShellCommandFunction>(celix::SHELL_COMMAND_FUNCTION_SERVICE_FQN, filter);
    EXPECT_TRUE(svcId > 0);

    svcId = framework().context().findService<celix::IShell>();
    EXPECT_TRUE(svcId > 0);
};

TEST_F(ShellTest, LbCommandTest) {
    std::stringstream ss{};
    std::function<void(celix::IShellCommand&)> useLb = [&ss](celix::IShellCommand& cmd){
        cmd.executeCommand("lb", {}, ss, ss);
    };
    std::string filter = std::string{"("} + celix::SHELL_COMMAND_FUNCTION_COMMAND_NAME + "=lb)";
    bool called = framework().context().useService(useLb, filter);
    EXPECT_TRUE(called);

    ss.flush();
    std::string output = ss.str();

    size_t pos = output.find("1: Framework");
    EXPECT_LE(pos, output.size());
    pos = output.find("2: Shell");
    EXPECT_LE(pos, output.size());

}

TEST_F(ShellTest, HelpCommandTest) {
    std::string filter = std::string{"("} + celix::SHELL_COMMAND_FUNCTION_COMMAND_NAME + "=help)";
    {
        //call general help
        std::stringstream ss{};
        std::function<void(celix::ShellCommandFunction&)> useHelp = [&ss](celix::ShellCommandFunction& cmd){
            cmd("help", {}, ss, ss);
        };
        bool called = framework().context().useFunctionService(celix::SHELL_COMMAND_FUNCTION_SERVICE_FQN, useHelp, filter);
        EXPECT_TRUE(called);

        ss.flush();
        std::string output = ss.str();
        size_t pos = output.find("help"); //Expect help as command
        EXPECT_LE(pos, output.size());
        pos = output.find("lb"); //Expect lb as command
        EXPECT_LE(pos, output.size());
        pos = output.find("inspect"); //Expect inspect as command
        EXPECT_LE(pos, output.size());
    }

    {
        //call help with arg
        std::stringstream ss{};
        std::vector<std::string> args{};
        std::function<void(celix::ShellCommandFunction&)> useHelp = [&ss, &args](celix::ShellCommandFunction& cmd){
            cmd("help", args, ss, ss);
        };

        args.emplace_back("lb");
        bool called = framework().context().useFunctionService(celix::SHELL_COMMAND_FUNCTION_SERVICE_FQN, useHelp, filter);
        EXPECT_TRUE(called);
        ss.flush();
        std::string output = ss.str();
        size_t pos = output.find("list bundles.");
        EXPECT_LE(pos, output.size());

        args.clear();
        args.emplace_back("unknown");
        ss.clear();
        called = framework().context().useFunctionService(celix::SHELL_COMMAND_FUNCTION_SERVICE_FQN, useHelp, filter);
        EXPECT_TRUE(called);
        ss.flush();
        output = ss.str();
        pos = output.find("not available");
        EXPECT_LE(pos, output.size());
    }
}

TEST_F(ShellTest, VersionCommandTest) {
    //NOTE that this also test if the resources zip usage (extender pattern).

    std::string filter = std::string{"("} + celix::SHELL_COMMAND_FUNCTION_COMMAND_NAME + "=version)";

    //call general help
    std::stringstream ss{};
    std::function<void(celix::ShellCommandFunction &)> useHelp = [&ss](celix::ShellCommandFunction &cmd) {
        cmd("version", {}, ss, ss);
    };
    bool called = framework().context().useFunctionService(celix::SHELL_COMMAND_FUNCTION_SERVICE_FQN, useHelp, filter);
    EXPECT_TRUE(called);

    ss.flush();
    std::string output = ss.str();
    size_t pos = output.find("3.0.0"); //Expect 3.0.0 as version
    EXPECT_LE(pos, output.size());
}


//TODO rest of the commands