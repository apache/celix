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

#include <gtest/gtest.h>
#include <fcntl.h>

#include "celix/FrameworkFactory.h"
#include "celix/BundleContext.h"
#include "history.h"

class ShellTuiTestSuite : public ::testing::Test {
public:
    ShellTuiTestSuite() {
        //open pipe to stimulate shell tui input
        int fds[2];
        int rc = pipe(fds);
        EXPECT_EQ(rc, 0) << strerror(errno);
        if (rc == 0) {
            inputReadFd = fds[0];
            inputWriteFd = fds[1];
        }

        //open pipe to stimulate shell tui output
        rc = pipe(fds);
        EXPECT_EQ(rc, 0) << strerror(errno);
        if (rc == 0) {
            outputReadFd = fds[0];
            outputWriteFd = fds[1];

            //set outputReadFd non blocking
            int flags = fcntl(outputReadFd, F_GETFL, 0);
            fcntl(outputReadFd, F_SETFL, flags | O_NONBLOCK);
        }
    }

    ~ShellTuiTestSuite() override {
        close(inputReadFd);
        close(inputWriteFd);
        close(outputReadFd);
        close(outputWriteFd);
    }

    void createFrameworkWithShellBundles(celix::Properties config = {}, bool configurePipes = false, bool installShell = true, bool installShellTui = true) {
        config.set("CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "trace");

        if (configurePipes) {
            config.set("SHELL_TUI_INPUT_FILE_DESCRIPTOR", std::to_string(inputReadFd).c_str());
            config.set("SHELL_TUI_OUTPUT_FILE_DESCRIPTOR", std::to_string(outputWriteFd).c_str());
            config.set("SHELL_TUI_ERROR_FILE_DESCRIPTOR", std::to_string(outputWriteFd).c_str());
        }

        fw = celix::createFramework(config);
        ctx = fw->getFrameworkBundleContext();

        if (installShell) {
            shellBundleId = ctx->installBundle(SHELL_BUNDLE_LOCATION);
            EXPECT_GT(shellBundleId, 0);
        }
        if (installShellTui) {
            shellTuiBundleId = ctx->installBundle(SHELL_TUI_BUNDLE_LOCATION);
            EXPECT_GT(shellTuiBundleId, 0);
        }
    }

    [[nodiscard]] std::string readPipeOutput() const {
        constexpr int BUFSIZE = 1024 * 10;
        char buf[BUFSIZE];
        buf[BUFSIZE-1] = '\0'; //ensure 0 terminated
        auto bytesRead = read(outputReadFd, buf, BUFSIZE-1);
        EXPECT_GT(bytesRead, 0);
        return std::string{buf};
    }

    void writeCmd(const char* cmd) const {
        write(inputWriteFd, cmd, strlen(cmd)+1);
        std::this_thread::sleep_for(std::chrono::milliseconds{100}); //sleep to let command be handled.
    }

    void testExecuteLb(bool enableAnsiControlSequence) {
        celix::Properties config{
                {"SHELL_TUI_USE_ANSI_CONTROL_SEQUENCES", enableAnsiControlSequence ? "true" : "false"},

        };
        createFrameworkWithShellBundles(std::move(config), true);

        const char* cmd = "lb\n";
        writeCmd(cmd);

        auto lbResult = readPipeOutput(); //lb output
        std::cout << lbResult << std::endl;
        EXPECT_TRUE(strstr(lbResult.c_str(), "Apache Celix Shell TUI")); //lb should print the shell tui name.
    }

    void testExecuteLbWithoutShell(bool enableAnsiControlSequence) {
        celix::Properties config{
                {"SHELL_TUI_USE_ANSI_CONTROL_SEQUENCES", enableAnsiControlSequence ? "true" : "false"}
        };
        createFrameworkWithShellBundles(std::move(config), true, false);

        const char* cmd = "lb\n";
        writeCmd(cmd);

        auto lbResult = readPipeOutput(); //lb output
        std::cout << lbResult << std::endl;
        EXPECT_TRUE(strstr(lbResult.c_str(), "[Shell TUI] Shell service not available") != nullptr);
    }

    void testAutoCompleteForCommand(const char* cmd, const char* expectedOutputPart) {
        //Test the triggering of auto complete and if the output contains the expected part
        celix::Properties config{
                {"SHELL_TUI_USE_ANSI_CONTROL_SEQUENCES", "true"},
        };
        createFrameworkWithShellBundles(std::move(config), true);

        writeCmd(cmd);

        auto result = readPipeOutput();
        std::cout << result << std::endl;
        EXPECT_TRUE(strstr(result.c_str(), expectedOutputPart) != nullptr);
    }

    std::shared_ptr<celix::Framework> fw{};
    std::shared_ptr<celix::BundleContext> ctx{};
    long shellBundleId = -1;
    long shellTuiBundleId = -1;
    int inputReadFd = -1;
    int inputWriteFd = -1;
    int outputReadFd = -1;
    int outputWriteFd = -1;
};

TEST_F(ShellTuiTestSuite, StartStopTest) {
    //simple start/stop bundles, but should not leak
    createFrameworkWithShellBundles();
}

TEST_F(ShellTuiTestSuite, ExecuteLbTest) {
    testExecuteLb(false);
}

TEST_F(ShellTuiTestSuite, ExecuteLbWithAnsiControlEnabledTest) {
    testExecuteLb(true);
}

TEST_F(ShellTuiTestSuite, AutoCompleteHelpCommandTest) {
    //note incomplete command with a tab -> should complete command to `help`
    testAutoCompleteForCommand("hel\t", "help");
}

TEST_F(ShellTuiTestSuite, AutoCompleteCelixLbCommandTest) {
    //note incomplete command with a tab -> should complete command to `celix::help`
    testAutoCompleteForCommand("celix::hel\t", "celix::help");
}

TEST_F(ShellTuiTestSuite, AutoCompleteLbUsageCommandTest) {
    //note complete help command with a tab -> should print usage
    testAutoCompleteForCommand("help \t", "Usage:");
}

TEST_F(ShellTuiTestSuite, AutoCompleteCelixLbUsageCommandTest) {
    //note complete celix::help command with a tab -> should print usage
    testAutoCompleteForCommand("celix::help \t", "Usage:");
}

TEST_F(ShellTuiTestSuite, ShellTuiWithInvalidFDTest) {
    celix::Properties config{
            {"SHELL_TUI_USE_ANSI_CONTROL_SEQUENCES", "true"},
            {"SHELL_TUI_INPUT_FILE_DESCRIPTOR", "555"}, //note invalid fd
            {"SHELL_TUI_OUTPUT_FILE_DESCRIPTOR", "556"}, //note invalid fd
            {"SHELL_TUI_ERROR_FILE_DESCRIPTOR", "557"} //note invalid fd
    };
    createFrameworkWithShellBundles(std::move(config));

    writeCmd("lb\n");
}

TEST_F(ShellTuiTestSuite, ShellTuiWithoutShellTest) {
    testExecuteLbWithoutShell(false);
}

TEST_F(ShellTuiTestSuite, ShellTuiWithAnsiControlWithoutShellTest) {
    testExecuteLbWithoutShell(true);
}

TEST_F(ShellTuiTestSuite, AnsiControlTest) {
    celix::Properties config{
            {"SHELL_TUI_USE_ANSI_CONTROL_SEQUENCES", "true"}
    };
    createFrameworkWithShellBundles(std::move(config), true);

    //this test triggers the handling of ansi control sequences, but currently does not test the output

    //build history
    const char* cmd = "lb\n";
    writeCmd(cmd);

    cmd = "\033[A"; //up
    writeCmd(cmd);

    cmd = "\033[C"; //right
    writeCmd(cmd);

    cmd = "\033[D"; //left
    writeCmd(cmd);

    cmd = "\033[3"; //del1
    writeCmd(cmd);

    cmd = "\033[~"; //del2
    writeCmd(cmd);

    cmd = "\033[9"; //tab
    writeCmd(cmd);

    cmd = "\033[127"; //backspace
    writeCmd(cmd);

    cmd = "\033[B"; //down
    writeCmd(cmd);

    std::cout << readPipeOutput() << std::endl;
}

TEST_F(ShellTuiTestSuite, WriteMoreThanMaxHistoryLines) {
    createFrameworkWithShellBundles();

    //WRITE more than hist max lines
    int maxHist = CELIX_SHELL_TUI_HIST_MAX;
    for (int i = 0; i < maxHist + 1; ++i) {
        writeCmd("lb\n");
    }
}
