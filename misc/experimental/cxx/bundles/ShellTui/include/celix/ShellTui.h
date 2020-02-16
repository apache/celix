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

#ifndef CELIX_SHELLTUI_H
#define CELIX_SHELLTUI_H

#include <sstream>
#include <mutex>
#include <thread>

#include "celix/api.h"
#include "celix/IShellCommand.h"
#include "celix/IShell.h"




namespace celix {
    static constexpr int SHELL_TUI_LINE_SIZE = 256;

    class ShellTui {
    public:
        ShellTui(std::ostream *_outStream, std::ostream *_errStream);
        ~ShellTui();

        ShellTui(const ShellTui&) = delete;
        ShellTui& operator=(const ShellTui&) = delete;

        void setShell(std::shared_ptr<celix::IShell> _shell);
    private:
        void runnable();
        void writePrompt();
        void parseInput();

        std::ostream * const outStream;
        std::ostream * const errStream;

        std::mutex mutex{};
        std::shared_ptr<celix::IShell> shell{};

        std::thread readThread{};

        int readPipeFd{};
        int writePipeFd{};


        char in[SHELL_TUI_LINE_SIZE+1]{};
        char buffer[SHELL_TUI_LINE_SIZE+1]{};
        int pos{};
    };
}

#endif //CELIX_SHELLTUI_H
