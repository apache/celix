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

#include "celix/Api.h"
#include "examples/IHelloWorld.h"
#include <vector>

namespace {

    class HelloWorldImpl : public examples::IHelloWorld {
    public:
        virtual ~HelloWorldImpl() = default;
        virtual std::string sayHello() override { return "Hello World"; }
    };

    class ProvideExampleActivator : public celix::IBundleActivator {
    public:
        explicit ProvideExampleActivator(const std::shared_ptr<celix::BundleContext>& ctx) {
            celix::ServiceRegistration reg = ctx->buildServiceRegistration<examples::IHelloWorld>()
                    .setService(std::make_shared<HelloWorldImpl>())
                    .addProperty("meta.info", "value")
                    .build();
            registrations.emplace_back(std::move(reg));
        }
    private:
        std::vector<celix::ServiceRegistration> registrations{};
    };

    __attribute__((constructor))
    static void registerStaticBundle() {
        celix::registerStaticBundle<ProvideExampleActivator>("examples::ProvideExample");
    }


}
