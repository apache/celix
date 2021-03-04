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

#include <future>
#include <utility>

#include "celix/DefaultExecutor.h"
#include "celix/DefaultScheduledExecutor.h"

class ExecutorTestSuite : public ::testing::Test {
public:
    ~ExecutorTestSuite() noexcept override = default;
    std::shared_ptr<celix::IExecutor> executor = std::make_shared<celix::DefaultExecutor>();
    std::shared_ptr<celix::IScheduledExecutor> scheduledExecutor = std::make_shared<celix::DefaultScheduledExecutor>();
};


TEST_F(ExecutorTestSuite, ExecuteTasks) {
    std::atomic<int> counter{0};
    executor->execute([&counter]{counter++;});
    executor->execute([&counter]{counter++;});
    executor->execute([&counter]{counter++;});
    executor->wait();
    EXPECT_EQ(3, counter.load());
}

TEST_F(ExecutorTestSuite, ScheduledExecuteTasks) {
    std::atomic<int> counter{0};
    auto t1 = std::chrono::steady_clock::now();
    scheduledExecutor->schedule(std::chrono::milliseconds{50}, [&counter]{counter++;});
    scheduledExecutor->schedule(std::chrono::milliseconds{50}, [&counter]{counter++;});
    scheduledExecutor->schedule(std::chrono::milliseconds{50}, [&counter]{counter++;});
    scheduledExecutor->wait();
    auto t2 = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
    EXPECT_EQ(3, counter.load());
    EXPECT_GT(diff, std::chrono::milliseconds{49});
}