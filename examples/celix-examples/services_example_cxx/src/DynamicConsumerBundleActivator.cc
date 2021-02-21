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

    /**
     * @brief A simple active consumer of the ICalc service.
     */
    class DynamicConsumer {
    public:
        /**
         * @brief create and start a new DynamicConsumer.
         * @param id The consumer id
         * @return A shared ptr to the newly created DynamicConsumer
         */
        static std::shared_ptr<DynamicConsumer> create(int id) {
            std::shared_ptr<DynamicConsumer> instance{new DynamicConsumer{id}, [](DynamicConsumer* consumer) {
               consumer->stop();
               delete consumer;
            }};
            instance->start();
            return instance;
        }

        /**
         * @brief Sets the calc service
         */
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
        explicit DynamicConsumer(int _id) : id{_id} {}

        /**
         * @brief Start the dynamic consumer thread.
         *
         * Should be called during creation.
         */
        void start() {
            calcThread = std::thread{&DynamicConsumer::run, this};
        }

        /**
         * @brief Stops the dynamic consumer thread.
         */
        void stop() {
            bool wasActive = active.exchange(false);
            if (wasActive) {
                calcThread.join();
            }
        }

        /**
         * @brief The run method. Calls the calc service (if not nullptr) and sleeps for 2 seconds
         */
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
                    std::cout << "Calc result is " << std::to_string(localCalc->calc(count)) << std::endl;
                    localCalc = nullptr;
                }

                std::this_thread::sleep_for(std::chrono::seconds{2});
                ++count;
            }
        }

        const int id;
        std::atomic<bool> active{true};
        std::thread calcThread{};

        std::mutex mutex{}; //protects below
        std::shared_ptr<examples::ICalc> calc{};
        long svcId{-1};
        long ranking{0};
    };

    /**
     * @brief A ICalc consumer factory which can be trigger with a Celix shell command.
     */
    class DynamicConsumerFactory : public celix::IShellCommand {
    public:
        static constexpr std::size_t MAX_CONSUMERS = 10;

        explicit DynamicConsumerFactory(std::shared_ptr<celix::BundleContext>  _ctx) : ctx{std::move(_ctx)} {}

        /**
         * @brief Executes the factory command by clearing all consumers and creating new ones.
         */
        void executeCommand(const std::string& /*commandLine*/, const std::vector<std::string>& /*commandArgs*/, FILE* /*outStream*/, FILE* /*errorStream*/) override {
            clearConsumer();
            createConsumers();
        }

        /**
         * @brief Create new consumers (as long as MAX_CONSUMERS is not reached)
         */
        void createConsumers() {
            std::lock_guard<std::mutex> lock{mutex};
            int nextConsumerId = 1;
            while (consumers.size() < MAX_CONSUMERS) {
                ctx->logInfo("Creating dynamic consumer nr %i", consumers.size());
                auto consumer = DynamicConsumer::create(nextConsumerId++);
                consumers[consumer] = ctx->trackServices<examples::ICalc>()
                        .addSetWithPropertiesCallback([consumer](std::shared_ptr<examples::ICalc> calc, std::shared_ptr<const celix::Properties> properties) {
                            consumer->setCalc(std::move(calc), std::move(properties));
                        })
                        .build();
            }
        }

        /**
         * @brief Clears all consumers.
         */
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

    /**
     * @brief A bundle activator for a dynamic ICalc consumer factory.
     */
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