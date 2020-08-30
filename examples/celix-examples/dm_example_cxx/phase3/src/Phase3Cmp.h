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

#ifndef CELIX_PHASE3CMP_H
#define CELIX_PHASE3CMP_H

#include "IPhase1.h"
#include "IPhase2.h"
#include <stdint.h>
#include <stdio.h>
#include <thread>
#include <iostream>
#include <list>
#include "celix/dm/Properties.h"

class Phase3Cmp  {
    std::map<IPhase2*,celix::dm::Properties> phases {};
    bool running {false};
    std::thread pollThread {};
public:
    Phase3Cmp() { std::cout << "Constructing Phase3Cmp\n"; }
    virtual ~Phase3Cmp() = default;

    void start();
    void stop();

    void poll();

    void addPhase2(IPhase2* phase, celix::dm::Properties&& props); //injector used by dependency manager
    void removePhase2(IPhase2* phase, celix::dm::Properties&& props); //injector used by dependency manager
    void setPhase2a(IPhase2* phase);
};

#endif //CELIX_PHASE3CMP_H
