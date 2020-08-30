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

#include "Foo.h"
#include <iostream>

void Foo::start() {
    std::cout << "start Foo\n";
    this->running = true;
    pollThread = std::thread {&Foo::poll, this};
}

void Foo::stop() {
    std::cout << "stop Foo\n";
    this->running = false;
    this->pollThread.join();
}

void Foo::setAnotherExample(IAnotherExample *e) {
    this->example = e;
}

void Foo::setExample(const example_t *e) {
    this->cExample = e;
}

void Foo::poll() {
    double r1 = 1.0;
    double r2 = 1.0;
    while (this->running) {
        //c++ service required -> if component started always available
        r1 = this->example->method(3, r1);
        std::cout << "Result IAnotherExample is " << r1 << "\n";

        //c service is optional, can be nullptr
        if (this->cExample != nullptr) {
            double out;
            this->cExample->method(this->cExample->handle, 4, r2, &out);
            r2 = out;
            std::cout << "Result example_t is " << r2 << "\n";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    }
}