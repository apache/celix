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

#include "celix/BundleActivator.h"
#include "examples/ICalc.h"

/**
 * @brief A simple active consumer of the ICalc service.
 */
class SimpleConsumer {
public:
    /**
     * @brief create and start a new SimpleConsumer.
     */
    static std::shared_ptr<SimpleConsumer> create() {
        std::shared_ptr<SimpleConsumer> instance{new SimpleConsumer{}, [](SimpleConsumer *consumer) {
           consumer->stop();
           delete consumer;
        }};
        instance->start();
        return instance;
    }

    /**
     * @brief Sets the calc service
     */
    void setCalc(std::shared_ptr<examples::ICalc> _calc) {
        std::lock_guard<std::mutex> lck{mutex};
        calc = std::move(_calc);
    }
private:
    SimpleConsumer() = default;

    /**
     * @brief Start the dynamic consumer thread.
     *
     * Should be called during creation.
     */
    void start() {
        calcThread = std::thread{&SimpleConsumer::run, this};
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
                std::cout << "Calc result for input " << count << " is " << localCalc->calc(count) << std::endl;
            } else {
                std::cout << "Calc service not available!" << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::seconds{2});
            ++count;
        }
    }

    std::atomic<bool> active{true};
    std::thread calcThread{};

    std::mutex mutex{}; //protects below
    std::shared_ptr<examples::ICalc> calc{};
};

/**
 * @brief A bundle activator for a simple ICalc consumer.
 */
class SimpleConsumerBundleActivator {
public:
    explicit SimpleConsumerBundleActivator(const std::shared_ptr<celix::BundleContext>& ctx) :
            consumer{SimpleConsumer::create()}, tracker{createTracker(ctx)} {}
private:
    std::shared_ptr<celix::GenericServiceTracker> createTracker(const std::shared_ptr<celix::BundleContext>& ctx) {
        return ctx->trackServices<examples::ICalc>()
                .addSetCallback(std::bind(&SimpleConsumer::setCalc, consumer, std::placeholders::_1))
                .build();
    }

    const std::shared_ptr<SimpleConsumer> consumer;
    const std::shared_ptr<celix::GenericServiceTracker> tracker;
};

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(SimpleConsumerBundleActivator)