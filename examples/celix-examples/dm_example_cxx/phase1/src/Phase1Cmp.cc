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

#include "Phase1Cmp.h"
#include <iostream>
#include <stdlib.h>
#include <stdio.h>

void Phase1Cmp::init() {
    std::cout << "init Phase1Cmp\n";
}

void Phase1Cmp::start() {
    std::cout << "start Phase1Cmp\n";
}

void Phase1Cmp::stop() {
    std::cout << "stop Phase1Cmp\n";
}

void Phase1Cmp::deinit() {
    std::cout << "deinit Phase1Cmp\n";
}

int Phase1Cmp::getData() {
    counter += 1;
    return (int)random();
};

std::string Phase1Cmp::getName() {
    return std::string {"IPhase"};
}

int Phase1Cmp::infoCmd([[gnu::unused]] const char * line, FILE *out, [[gnu::unused]] FILE* err) {
    fprintf(out, "Phase1: number of getData calls: %u\n", counter);
    return 0;
}
