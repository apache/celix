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

#ifndef CELIX_PHASE2CMP_H
#define CELIX_PHASE2CMP_H

#include "IName.h"
#include "IPhase1.h"
#include "IPhase2.h"
#include <stdint.h>
#include <stdio.h>
#include <thread>
#include <iostream>

extern "C" {
#include "celix_log_service.h"
};

class Phase2Cmp : public IPhase2 {
public:
    Phase2Cmp() = default;
    virtual ~Phase2Cmp() { std::cout << "Destroying Phase2\n"; };

    Phase2Cmp(Phase2Cmp&& other) noexcept;
    Phase2Cmp& operator=(Phase2Cmp&&) = default;

    Phase2Cmp(const Phase2Cmp& other) = delete;
    Phase2Cmp operator=(const Phase2Cmp&) = delete;

    void setPhase1(IPhase1* phase); //injector used by dependency manager
    void setName(srv::info::IName* name);
    void setLogService(const celix_log_service_t* logSrv);

    double getData() override; //implements IPhase2
private:
    IPhase1* phase1 {nullptr};
    const celix_log_service_t* logSrv {nullptr};
};

#endif //CELIX_PHASE2CMP_H
