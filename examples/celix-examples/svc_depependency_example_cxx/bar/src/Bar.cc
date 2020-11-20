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

#include "Bar.h"
#include <iostream>

void Bar::init() {
    std::cout << "init Bar\n";
}

void Bar::start() {
    std::cout << "start Bar\n";
}

void Bar::stop() {
    std::cout << "stop Bar\n";
}

void Bar::deinit() {
    std::cout << "deinit Bar\n";
}

double Bar::method(int arg1, double arg2) {
    double update = (this->seed + arg1) * arg2;
    return update;
}

int Bar::cMethod(int arg1, double arg2, double *out) {
    double r = this->method(arg1, arg2);
    *out = r;
    return 0;
}