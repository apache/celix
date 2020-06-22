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
#include <chrono>
#include "celix/Api.h"
#include "examples/IHelloWorld.h"

namespace {

    class UseServiceActivator : public celix::IBundleActivator {
    public:
        explicit UseServiceActivator(const std::shared_ptr<celix::BundleContext>& ctx) {
            ctx->buildUseService<examples::IHelloWorld>()
                    .setFilter("(meta.info=value)")
                    .waitFor(std::chrono::seconds{1})
                    .setCallback([](auto &svc) {
                        std::cout << "Called from UseService: " << svc.sayHello() << std::endl;
                    })
                    .use();
        }
    };

    __attribute__((constructor))
    static void registerStaticBundle() {
        celix::registerStaticBundle<UseServiceActivator>("examples::UseService");
    }
}