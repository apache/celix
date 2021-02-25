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

#ifndef CELIX_PHASE1ACTIVATOR_H
#define CELIX_PHASE1ACTIVATOR_H

#include "celix/dm/DependencyManager.h"
#include "celix_shell_command.h"

using namespace celix::dm;
struct DummyService {
    void *handle;
};

class Phase1Activator {
public:
    Phase1Activator(std::shared_ptr<celix::dm::DependencyManager> mng);
    Phase1Activator(const Phase1Activator&) = delete;
    Phase1Activator& operator=(const Phase1Activator&) = delete;
private:
    std::shared_ptr<celix::dm::DependencyManager> dm{};
    std::string cmpUUID{};
    celix_shell_command_t cmd {nullptr, nullptr};
    celix_shell_command_t addCmd {nullptr, nullptr};
    celix_shell_command_t removeCmd {nullptr, nullptr};
    DummyService *dummySvc{new DummyService};
};

#endif //CELIX_PHASE1ACTIVATOR_H
