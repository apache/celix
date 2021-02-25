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

#include <iostream>
#include "ICalc.h"
#include "celix/BundleActivator.h"

class CalcUserBundleActivator {
public:
    explicit CalcUserBundleActivator(const std::shared_ptr<celix::BundleContext>& ctx) {
        ctx->useService<ICalc>()
                .addUseCallback([](ICalc& calc) {
                    std::cout << "result is " << calc.add(2, 3) << std::endl;
                })
                .setTimeout(std::chrono::seconds{1}) //wait for 1 second if a service is not directly found
                .build();
    }
};

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(CalcUserBundleActivator)