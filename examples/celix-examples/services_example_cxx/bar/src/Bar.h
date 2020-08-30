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

#ifndef BAR_H
#define BAR_H

#include "IAnotherExample.h"

class Bar : public IAnotherExample {
    const double seed = 42;
public:
    Bar() = default;
    virtual ~Bar() = default;

    void init();
    void start();
    void stop();
    void deinit();

    virtual double method(int arg1, double arg2) override; //implementation of IAnotherExample::method
    int cMethod(int arg1, double arg2, double *out); //implementation of example_t->method;
};

#endif //BAR_H
