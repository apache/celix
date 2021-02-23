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

#include "Baz.h"
#include <iostream>

void Baz::start() {
    std::cout << "start Baz\n";
    this->running = true;
    pollThread = std::thread {&Baz::poll, this};
}

void Baz::stop() {
    std::cout << "stop Baz\n";
    this->running = false;
    this->pollThread.join();
}

void Baz::addAnotherExample(IAnotherExample *e) {
    std::lock_guard<std::mutex> lock(this->lock_for_examples);
    this->examples.push_back(e);
}

void Baz::removeAnotherExample(IAnotherExample *e) {
    std::lock_guard<std::mutex> lock(this->lock_for_examples);
    this->examples.remove(e);
}

void Baz::addExample(const example_t *e) {
    std::lock_guard<std::mutex> lock(this->lock_for_cExamples);
    this->cExamples.push_back(e);
}

void Baz::removeExample(const example_t *e) {
    std::lock_guard<std::mutex> lock(this->lock_for_cExamples);
    this->cExamples.remove(e);
}

void Baz::poll() {
    double r1 = 1.0;
    double r2 = 1.0;
    while (this->running) {
        //c++ service required -> if component started always available

        {
            std::lock_guard<std::mutex> lock(this->lock_for_examples);
            int index = 0;
            for (IAnotherExample *e : this->examples) {
                r1 = e->method(3, r1);
                std::cout << "Result IAnotherExample " << index++ << " is " << r1 << "\n";
            }
        }


        {
            std::lock_guard<std::mutex> lock(this->lock_for_cExamples);
            int index = 0;
            for (const example_t *e : this->cExamples) {
                double out;
                e->method(e->handle, 4, r2, &out);
                r2 = out;
                std::cout << "Result example_t " << index++ << " is " << r2 << "\n";
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(4000));
    }
}