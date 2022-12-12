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

#include <mutex>
#include "ICalc.h"
#include "celix/BundleActivator.h"

class CalcTrackerBundleActivator {
public:
    explicit CalcTrackerBundleActivator(const std::shared_ptr<celix::BundleContext>& ctx) {
        tracker = ctx->trackServices<ICalc>()
                .build();
        for (auto& calc : tracker->getServices()) {
            std::cout << "result is " << std::to_string(calc->add(2, 3)) << std::endl;
        }
    }

private:
    std::shared_ptr<celix::ServiceTracker<ICalc>> tracker{};
};

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(CalcTrackerBundleActivator)