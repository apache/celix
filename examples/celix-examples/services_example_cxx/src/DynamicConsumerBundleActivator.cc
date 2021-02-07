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

#include <utility>

#include "celix/IShellCommand.h"
#include "celix/BundleActivator.h"
#include "examples/ICalc.h"

namespace /*anon*/ {

    class DynamicConsumer {
    public:
        explicit DynamicConsumer(int _id) : id{_id} {}

        ~DynamicConsumer() {
            stop();
        }

        void start() {
            std::lock_guard<std::mutex> lck{mutex};
            calcThread = std::thread{&DynamicConsumer::run, this};
        }

        void stop() {
            active = false;
            if (calcThread.joinable()) {
                calcThread.join();
                auto msg = std::string{"Destroying consumer nr "} + std::to_string(id) + "\n";
                std::cout << msg;
            }
        }

        void setCalc(std::shared_ptr<examples::ICalc> _calc, const std::shared_ptr<const celix::Properties>& props) {
            std::lock_guard<std::mutex> lck{mutex};
            if (_calc) {
                calc = std::move(_calc);
                svcId = props->getAsLong(celix::SERVICE_ID, -1);
                ranking = props->getAsLong(celix::SERVICE_RANKING, 0);

                std::string msg = "Setted calc svc for consumer nr " +  std::to_string(id);
                msg += ". svc id is  "  + std::to_string(svcId);
                msg += ", and ranking is " + std::to_string(ranking);
                msg += "\n";
                std::cout << msg;
            } else {
                std::string msg = "Resetting calc svc for consumer " + std::to_string(id)  + " to nullptr\n";
                std::cout << msg;

                calc = nullptr;
                svcId = -1;
                ranking = 0;
            }
        }
    private:
        void run() {
            std::unique_lock<std::mutex> lck{mutex,  std::defer_lock};
            int count = 1;
            while (active) {
                lck.lock();
                auto localCalc = calc;
                lck.unlock();

                /*
                 * note it is safe to use the localCalc outside a mutex,
                 * because the shared_prt count will ensure the service cannot be unregistered while in use.
                 */
                if (localCalc) {
                    auto msg = std::string{"Calc result is "} + std::to_string(localCalc->calc(count)) + "\n";
                    std::cout << msg;
                    localCalc = nullptr;
                }

                std::this_thread::sleep_for(std::chrono::seconds{2});
                ++count;
            }
        }

        const int id;
        std::atomic<bool> active{true};

        std::mutex mutex{}; //protects below
        std::shared_ptr<examples::ICalc> calc{};
        long svcId{-1};
        long ranking{0};
        std::thread calcThread{};
    };

    class DynamicConsumerFactory : public celix::IShellCommand {
    public:
        static constexpr std::size_t MAX_CONSUMERS = 10;

        explicit DynamicConsumerFactory(std::shared_ptr<celix::BundleContext>  _ctx) : ctx{std::move(_ctx)} {}

        void executeCommand(std::string /*commandLine*/, std::vector<std::string> /*commandArgs*/, FILE* /*outStream*/, FILE* /*errorStream*/) override {
            clearConsumer();
            createConsumers();
        }

        void createConsumers() {
            std::lock_guard<std::mutex> lock{mutex};
            int nextConsumerId = 1;
            while (consumers.size() < MAX_CONSUMERS) {
                ctx->logInfo("Creating dynamic consumer nr %i", consumers.size());
                auto consumer = std::make_shared<DynamicConsumer>(nextConsumerId++);
                consumers[consumer] = ctx->trackServices<examples::ICalc>()
                        .addSetWithPropertiesCallback([consumer](std::shared_ptr<examples::ICalc> calc, std::shared_ptr<const celix::Properties> properties) {
                            consumer->setCalc(std::move(calc), std::move(properties));
                        })
                        .build();
                consumer->start();
            }
        }

        void clearConsumer() {
            ctx->logInfo("Resetting all trackers and consumers");
            std::lock_guard<std::mutex> lock{mutex};
            consumers.clear();
            ctx->logInfo("Done resetting consumers");
        }
    private:
        const std::shared_ptr<celix::BundleContext> ctx;

        std::mutex mutex{}; //protects below
        std::unordered_map<std::shared_ptr<DynamicConsumer>, std::shared_ptr<celix::GenericServiceTracker>> consumers{};
    };

    class DynamicConsumerBundleActivator {
    public:
        explicit DynamicConsumerBundleActivator(const std::shared_ptr<celix::BundleContext>& ctx) : factory{std::make_shared<DynamicConsumerFactory>(ctx)} {
            factory->createConsumers();
            cmdReg = ctx->registerService<celix::IShellCommand>(factory)
                    .addProperty(celix::IShellCommand::COMMAND_NAME, "consumers_reset")
                    .build();
        }
    private:
        std::shared_ptr<DynamicConsumerFactory> factory;
        std::shared_ptr<celix::ServiceRegistration> cmdReg{};
    };

}

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(DynamicConsumerBundleActivator)