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

#ifndef CELIX_PHASE1CMP_H
#define CELIX_PHASE1CMP_H

#include "IPhase1.h"
#include "IName.h"
#include <stdint.h>
#include <stdio.h>

class Phase1Cmp : public srv::info::IName, public IPhase1 {
    uint32_t counter = 0;
public:
    Phase1Cmp() = default;
    virtual ~Phase1Cmp() = default;

    void init();
    void start();
    void stop();
    void deinit();

    int getData() override; //implements IPhase1
    int infoCmd(const char* line, FILE *out, FILE* err);  //implements cmd service
    std::string getName() override;
};

#endif //CELIX_PHASE1CMP_H
