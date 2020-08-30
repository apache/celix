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

#include "Phase3Cmp.h"
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <thread>
#include <chrono>
#include <sstream>

void Phase3Cmp::start() {
    std::cout << "start Phase3Cmp\n";
    running = true;
    pollThread = std::thread {&Phase3Cmp::poll, this};
}

void Phase3Cmp::stop() {
    std::cout << "stop Phase3Cmp\n";
    running = false;
    pollThread.join();
}

void Phase3Cmp::poll() {
    while (running) {
        std::cout << "polling Phase3Cmp\n";
        for (std::pair<IPhase2*, celix::dm::Properties> pair : this->phases) {
            std::ostringstream oss {};
            oss << "current data for " << pair.second["name"] << " is " << pair.first->getData() << "\n";
            std::cout << oss.str();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

void Phase3Cmp::addPhase2(IPhase2* phase2, celix::dm::Properties&& props) {
    std::cout << "adding Iphase2 for Phase3Cmp\n";
    std::pair<IPhase2*, celix::dm::Properties> pair {phase2, props};
    this->phases[phase2] = props;
}

void Phase3Cmp::removePhase2(IPhase2* phase2, [[gnu::unused]] celix::dm::Properties&& props) {
    std::cout << "adding Iphase2 for Phase3Cmp\n";
    this->phases.erase(phase2);
}

void Phase3Cmp::setPhase2a([[gnu::unused]] IPhase2* phase) {
    std::cout << "setting phase2a\n";
}
