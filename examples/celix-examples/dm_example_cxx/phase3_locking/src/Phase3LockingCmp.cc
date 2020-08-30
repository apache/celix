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

#include "Phase3LockingCmp.h"
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <thread>
#include <chrono>
#include <sstream>

void Phase3LockingCmp::start() {
    std::cout << "start Phase3LockingCmp\n";
    running = true;
    //pollThread = std::thread {&Phase3LockingCmp::poll, std::ref(*this)};
    pollThread = std::thread {&Phase3LockingCmp::poll, this};
}

void Phase3LockingCmp::stop() {
    std::cout << "stop Phase3LockingCmp\n";
    running = false;
    pollThread.join();
}

void Phase3LockingCmp::poll() {
    while (running) {
        std::cout << "polling Phase3LockingCmp\n";
        mutex.lock();
        for (std::pair<IPhase2*, celix::dm::Properties> pair : this->phases) {
            std::ostringstream oss {};
            oss << "current data for " << pair.second["name"] << " is " << pair.first->getData() << "\n";
            std::cout << oss.str();
        }
        mutex.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

void Phase3LockingCmp::addPhase2(IPhase2* phase2, celix::dm::Properties&& props) {
    std::cout << "adding Iphase2 for Phase3LockingCmp\n";
    std::lock_guard<std::mutex> lock(mutex);
    std::pair<IPhase2*, celix::dm::Properties> pair {phase2, props};
    this->phases[phase2] = props;
}

void Phase3LockingCmp::removePhase2(IPhase2* phase2, [[gnu::unused]] celix::dm::Properties&& props) {
    std::cout << "adding Iphase2 for Phase3LockingCmp\n";
    std::lock_guard<std::mutex> lock(mutex);
    this->phases.erase(phase2);
}
