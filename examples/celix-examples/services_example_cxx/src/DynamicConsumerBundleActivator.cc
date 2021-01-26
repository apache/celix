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

#include "celix/BundleActivator.h"
#include "examples/ICalc.h"

namespace /*anon*/ {

    class DynamicConsumer {
    public:
        explicit DynamicConsumer(int _id) : id{_id} {}

        ~DynamicConsumer() {
            active = false;
            calcThread.join();
            auto msg = std::string{"Destroying consumer nr "} + std::to_string(id) + "\n";
            std::cout << msg;
        }

        void start() {
            std::lock_guard<std::mutex> lck{mutex};
            calcThread = std::thread{&DynamicConsumer::run, this};
        }

        void setCalc(std::shared_ptr<examples::ICalc> _calc) {
            std::lock_guard<std::mutex> lck{mutex};
            calc = std::move(_calc);
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
                    auto msg = std::string{"Calc result for consumer nr "} + std::to_string(id) + " with input " + std::to_string(count) + " is " + std::to_string(localCalc->calc(count)) + "\n";
                    std::cout << msg;
                } else {
                    auto msg = std::string{"Calc service not available for consumer nr "} + std::to_string(id) + "\n";
                    std::cout << msg;
                }

                //NOTE the sleep also keeps the localCalc shared_ptr activate and as result block the removal of the service
                std::this_thread::sleep_for(std::chrono::seconds{2});

                ++count;
            }
        }

        const int id;
        std::atomic<bool> active{true};
        std::shared_ptr<examples::ICalc> calc{};

        std::mutex mutex{}; //protects below
        std::thread calcThread{};
    };

    class DynamicConsumerFactory {
    public:
        static constexpr std::size_t MAX_CONSUMERS = 5;

        explicit DynamicConsumerFactory(std::shared_ptr<celix::BundleContext>  _ctx) : ctx{std::move(_ctx)} {}

        ~DynamicConsumerFactory() {
            ctx->logInfo("Destroying DynamicConsumerFactory");
            active = false;
            factoryThread.join();
        }


        void start() {
            std::lock_guard<std::mutex> lck{mutex};
            factoryThread = std::thread{&DynamicConsumerFactory::run, this};
        }

        void run() {
            std::unique_lock<std::mutex> lck{mutex,  std::defer_lock};
            int nextConsumerId = 1;
            while (active) {
                lck.lock();
                if (consumers.size() < MAX_CONSUMERS) {
                    ctx->logInfo("Creating dynamic consumer nr %i", consumers.size());
                    auto consumer = std::make_shared<DynamicConsumer>(nextConsumerId++);
                    trackers.push_back(createTracker(ctx, consumer)); //NOTE is this possible after a move?
                    consumers.push_back(consumer);
                    consumer->start();
                } else {
                    ctx->logInfo("Resetting all trackers and consumers");
                    trackers.clear();
                    ctx->logInfo("Done resetting trackers");
                    consumers.clear();
                    ctx->logInfo("Done resetting consumer");
                }
                lck.unlock();
                std::this_thread::sleep_for(std::chrono::seconds{1});
            }
        }
    private:
        static std::shared_ptr<celix::GenericServiceTracker> createTracker(const std::shared_ptr<celix::BundleContext>& ctx, const std::shared_ptr<DynamicConsumer>& consumer) {
            return ctx->trackServices<examples::ICalc>()
                    .addSetCallback([consumer](std::shared_ptr<examples::ICalc> svc) {
                        consumer->setCalc(std::move(svc));
                    })
                    .build();
        }

        const std::shared_ptr<celix::BundleContext> ctx;
        std::atomic<bool> active{true};

        std::mutex mutex{}; //protects below
        std::thread factoryThread{};
        std::vector<std::shared_ptr<DynamicConsumer>> consumers{};
        std::vector<std::shared_ptr<celix::GenericServiceTracker>> trackers{};
    };

    class DynamicConsumerBundleActivator {
    public:
        explicit DynamicConsumerBundleActivator(const std::shared_ptr<celix::BundleContext>& ctx) : factory{ctx} {
            factory.start();
        }
    private:
        DynamicConsumerFactory factory;
    };

}

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(DynamicConsumerBundleActivator)