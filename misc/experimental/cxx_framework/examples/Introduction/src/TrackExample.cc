/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */

#include <iostream>
#include <vector>
#include "celix/Api.h"
#include "examples/IHelloWorld.h"

namespace {

    class TrackExampleActivator : public celix::IBundleActivator {
    public:
        explicit TrackExampleActivator(const std::shared_ptr<celix::BundleContext>& ctx) {
            celix::ServiceTracker tracker = ctx->buildServiceTracker<examples::IHelloWorld>()
                    .setCallback([this](auto& svc) {
                        std::lock_guard<std::mutex> lck{mutex};
                        helloWorldSvc = svc;
                        std::cout << "updated hello world service: " << (svc ? svc->sayHello() : "nullptr") << std::endl;
                    })
                    .build();
            trackers.emplace_back(std::move(tracker));
        }
    private:
        std::mutex mutex{}; //protect belows
        std::shared_ptr<examples::IHelloWorld> helloWorldSvc{nullptr};
        std::vector<celix::ServiceTracker> trackers{};
    };

    __attribute__((constructor))
    static void registerStaticBundle() {
        celix::registerStaticBundle<TrackExampleActivator>("examples::TrackExample");
    }


}