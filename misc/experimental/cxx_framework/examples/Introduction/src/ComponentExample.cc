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
#include "celix/IShellCommand.h"
#include "examples/IHelloWorld.h"

namespace {
    class ComponentExample : public celix::IShellCommand {
    public:
        void setHelloWorld(const std::shared_ptr<examples::IHelloWorld>& svc) {
            std::lock_guard<std::mutex> lck{mutex};
            helloWorld = svc;
        }
        void executeCommand(const std::string&, const std::vector<std::string>&, std::ostream &out, std::ostream &/*err*/) override {
            std::lock_guard<std::mutex> lck{mutex};
            if (helloWorld) {
                out << helloWorld->sayHello() << std::endl;
            } else {
                out << "IHelloWorld service not available" << std::endl;
            }
        }
    private:
        std::mutex mutex{};
        std::shared_ptr<examples::IHelloWorld> helloWorld{nullptr};
    };

    class ComponentExampleActivator : public celix::IBundleActivator {
    public:
        explicit ComponentExampleActivator(const std::shared_ptr<celix::BundleContext>& ctx) :
        cmpMng{ctx->createComponentManager(std::make_shared<ComponentExample>())} {
            cmpMng.addProvidedService<celix::IShellCommand>()
                    .addProperty(celix::IShellCommand::COMMAND_NAME, "hello");
            cmpMng.addServiceDependency<examples::IHelloWorld>()
                    .setCallbacks(&ComponentExample::setHelloWorld)
                    .setRequired(false);
            cmpMng.enable();
        }
    private:
        celix::ComponentManager<ComponentExample> cmpMng;
    };

    __attribute__((constructor))
    static void registerStaticBundle() {
        celix::registerStaticBundle<ComponentExampleActivator>("examples::ComponentExample");
    }
}