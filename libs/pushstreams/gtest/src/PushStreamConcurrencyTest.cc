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
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>

#include <gtest/gtest.h>

#include "celix/PushStreamProvider.h"

using namespace std::chrono_literals;

TEST(PushStreamConcurrencyTest, BufferedStreamKeepsInFlightConsumerAliveWhenForEachIsReplaced) {
    celix::PushStreamProvider provider{};
    auto promiseFactory = std::make_shared<celix::PromiseFactory>();
    auto eventSource = provider.createSynchronousEventSource<int>(promiseFactory);
    auto stream = provider.createStream<int>(eventSource, promiseFactory);

    std::mutex mutex{};
    std::condition_variable consumerEntered{};
    std::condition_variable releaseConsumer{};
    bool entered = false;
    bool release = false;
    std::atomic<int> firstConsumerCount{0};
    std::atomic<int> secondConsumerCount{0};

    auto firstStreamEnded = stream->forEach([&](const int&) {
        {
            std::lock_guard lock{mutex};
            entered = true;
        }
        consumerEntered.notify_one();

        std::unique_lock lock{mutex};
        releaseConsumer.wait(lock, [&] { return release; });
        firstConsumerCount.fetch_add(1);
    });

    eventSource->publish(1);

    {
        std::unique_lock lock{mutex};
        ASSERT_TRUE(consumerEntered.wait_for(lock, 5s, [&] { return entered; }));
    }

    auto secondStreamEnded = stream->forEach([&](const int&) {
        secondConsumerCount.fetch_add(1);
    });

    {
        std::lock_guard lock{mutex};
        release = true;
    }
    releaseConsumer.notify_one();
    promiseFactory->getExecutor()->wait();

    eventSource->publish(2);
    promiseFactory->getExecutor()->wait();

    eventSource->close();
    secondStreamEnded.wait();

    EXPECT_EQ(1, firstConsumerCount.load());
    EXPECT_EQ(1, secondConsumerCount.load());

    (void)firstStreamEnded;
}
