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


#include "celix/ShellTui.h"

#include <memory>
#include <sstream>

#include <thread>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <mutex>

#include <glog/logging.h>

#include "celix/api.h"
#include "celix/IShellCommand.h"
#include "celix/IShell.h"

static constexpr const char * const PROMPT = "-> ";
static constexpr int KEY_ENTER = '\n';

celix::ShellTui::ShellTui(std::ostream *_outStream, std::ostream *_errStream) : outStream{_outStream}, errStream{_errStream} {
    int fds[2];
    int rc  = pipe(fds);
    if (rc == 0) {
        readPipeFd = fds[0];
        writePipeFd = fds[1];
    if(fcntl(writePipeFd, F_SETFL, O_NONBLOCK) == 0) {
        readThread = std::thread{&ShellTui::runnable, this};
    } else {
        LOG(ERROR) << "fcntl on pipe failed" << std::endl;
    }
    } else {
        LOG(ERROR) << "fcntl on pipe failed" << std::endl;
    }
}


celix::ShellTui::~ShellTui() {
    write(writePipeFd, "\0", 1); //trigger select to stop
    readThread.join();
}

void celix::ShellTui::runnable() {
    //setup file descriptors
    fd_set rfds;
    int nfds = writePipeFd > STDIN_FILENO ? (writePipeFd +1) : (STDIN_FILENO + 1);

    for (;;) {
        writePrompt();
        FD_ZERO(&rfds);
        FD_SET(STDIN_FILENO, &rfds);
        FD_SET(readPipeFd, &rfds);

        if (select(nfds, &rfds, NULL, NULL, NULL) > 0) {
            if (FD_ISSET(readPipeFd, &rfds)) {
                break; //something is written to the pipe -> exit thread
            } else if (FD_ISSET(STDIN_FILENO, &rfds)) {
                parseInput();
            }
        }
    }
}

void celix::ShellTui::writePrompt() {
    *outStream << PROMPT;
    std::flush(*outStream);
    outStream->flush();
}

void celix::ShellTui::parseInput() {
    char* line = NULL;
    int nr_chars = (int)read(STDIN_FILENO, buffer, SHELL_TUI_LINE_SIZE-pos-1);
    for (int bufpos = 0; bufpos < nr_chars; bufpos++) {
        if (buffer[bufpos] == KEY_ENTER) { //end of line -> forward command
            line = in; // todo trim string
            std::lock_guard<std::mutex> lck{mutex};
            if (shell) {
                shell->executeCommandLine(line, *outStream, *errStream);
            } else {
                *errStream << "Shell service not available\n";
            }
            pos = 0;
            in[pos] = '\0';
        } else { //text
            in[pos] = buffer[bufpos];
            in[pos + 1] = '\0';
            pos++;
            continue;
        }
    } //for
}

void celix::ShellTui::setShell(std::shared_ptr<celix::IShell> _shell) {
    std::lock_guard<std::mutex> lck{mutex};
    shell = std::move(_shell);
}