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

#pragma once

#include "celix/Promise.h"
#include "celix/PushStream.h"

class ICalculator {
public:
    virtual ~ICalculator() noexcept = default;

    /**
     * (Remote) call which provides a stream from provider -> user.
     * @return
     */
    virtual std::shared_ptr<celix::PushStream<double>> result() = 0;

    /**
     * (Remote) call which invokes async and return a Promise object for async resolving the successful or
     * unsuccessful return value.
     */
    virtual celix::Promise<double> add(double a, double b) = 0;

};
