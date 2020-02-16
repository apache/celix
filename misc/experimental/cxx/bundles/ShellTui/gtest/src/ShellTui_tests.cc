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
#include "celix/ShellTui.h"

class ShellTuiTest : public ::testing::Test {
public:
    ShellTuiTest() {}
    ~ShellTuiTest(){}

    celix::Framework& framework() { return fw; }
private:
    celix::Framework fw{};
};




TEST_F(ShellTuiTest, AcceptCommand) {
    {
        std::stringstream stream{};
        celix::ShellTui tui{&stream, &stream};
        usleep(100000);//IMPROVE, for now wait till stdin reading thread is started

        stream.flush();
        std::string output = stream.str();
        size_t pos = output.find("->"); //prompt should be printed
        EXPECT_LE(pos, output.size());
    }

    //TODO make working
//    {
//        std::stringstream stream{};
//        celix::ShellTui tui{&stream, &stream};
//        std::string cmd = "lb\n";
//        write(STDIN_FILENO, cmd.c_str(), cmd.size());
//        usleep(100000);//IMPROVE, for now wait till stdin command is processed on other thread.
//        stream.flush();
//        std::string output = stream.str();
//        size_t pos = output.find("1: Framework"); //expect lb result
//        EXPECT_LE(pos, output.size());
//    }

}