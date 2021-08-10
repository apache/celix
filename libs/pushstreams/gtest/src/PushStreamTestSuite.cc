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

#include <gtest/gtest.h>

#include "celix/PushStreamProvider.h"

using celix::PushStreamProvider;

class PushStreamTestSuite : public ::testing::Test {
public:
    ~PushStreamTestSuite() noexcept override = default;
};

TEST_F(PushStreamTestSuite, BasicTest) {
    auto psp = PushStreamProvider();
    std::unique_ptr<std::thread> t{};
    auto ses = psp.createSimpleEventSource<int>();

    auto successLamba = [&](celix::Promise<void> p) -> celix::Promise<void> {
        t = std::make_unique<std::thread>([&]() {
            long counter = 0;
            // Keep going as long as someone is listening
            while (ses->isConnected()) {
                ses->publish(++counter);
                std::this_thread::sleep_for(std::chrono::milliseconds{50});
                std::cout << "Published: " << counter << std::endl;
            }
            // Restart delivery when a new listener connects
            // ses.connectPromise().then(success);            
        });         
        return p;
    };

    ses->connectPromise().then<void>(successLamba);

    auto streamEnded = psp.createStream<int>(ses)->
    filter([](int event) -> bool {
        return (event > 10);
    })->
    filter([&](int event) -> bool {
        return (event < 15);
    })->
    forEach([](int event) {
        std::cout << "Consumed event: " << event << std::endl;
    });

    streamEnded.onSuccess([]() {
        std::cout << "Stream ended" << std::endl;
    });

    std::this_thread::sleep_for(std::chrono::seconds{1});

    ses->close();

    t->join();

}


TEST_F(PushStreamTestSuite, MultipleStreamsTest) {
    auto psp = PushStreamProvider();
    std::unique_ptr<std::thread> t{};
    auto ses = psp.createSimpleEventSource<int>();

    auto success = [&](celix::Promise<void> p) -> celix::Promise<void> {
        t = std::make_unique<std::thread>([&]() {
            long counter = 0;
            // Keep going as long as someone is listening
            while (ses->isConnected()) {
                ses->publish(++counter);
                std::this_thread::sleep_for(std::chrono::milliseconds{50});
                std::cout << "Published: " << counter << std::endl;
            }
            // Restart delivery when a new listener connects
            // ses.connectPromise().then(success);
        });
        return p;
    };

    ses->connectPromise().then<void>(success);

    auto streamEnded = psp.createStream<int>(ses)->
    filter([&](int event) -> bool {
        return (event > 10);
    })->
    filter([&](int event) -> bool {
        return (event < 15);
    })->
    forEach([&](int event) {
        std::cout << "Consumed event 1: " << event << std::endl;
    });

    auto streamEnded2 = psp.createStream<int>(ses)->
    filter([&](int event) -> bool {
        return (event < 15);
    })->
    forEach([&](int event) {
        std::cout << "Consumed event 2: " << event << std::endl;
    });

    streamEnded.onSuccess([]() {
        std::cout << "Stream ended" << std::endl;
    });

    streamEnded2.onSuccess([]() {
        std::cout << "Stream2 ended" << std::endl;
    });

    std::this_thread::sleep_for(std::chrono::seconds{1});

    ses->close();
    t->join();
}
