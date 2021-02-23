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

#ifndef BAZ_H
#define BAZ_H

#include "example.h"
#include "IAnotherExample.h"
#include <thread>
#include <list>
#include <mutex>

class Baz  {
    std::list<IAnotherExample*> examples {};
    std::mutex lock_for_examples {};

    std::list<const example_t*> cExamples {};
    std::mutex lock_for_cExamples {};

    std::thread pollThread {};
    bool running = false;
public:
    Baz() = default;
    virtual ~Baz() = default;

    void start();
    void stop();

    void addAnotherExample(IAnotherExample* e);
    void removeAnotherExample(IAnotherExample* e);

    void addExample(const example_t* e);
    void removeExample(const example_t* e);

    void poll();
};

#endif //BAZ_H
