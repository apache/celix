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

#include <gtest/gtest.h>

#include "celix/FrameworkFactory.h"
#include "celix_bundle_context.h"
#include "celix_scheduled_event.h"

class ScheduledEventTestSuite : public ::testing::Test {
  public:
    ScheduledEventTestSuite() { fw = celix::createFramework({{"CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "trace"}}); }
    std::shared_ptr<celix::Framework> fw{};
};

TEST_F(ScheduledEventTestSuite, OnceShotEventTest) {
    auto ctx = fw->getFrameworkBundleContext();

    struct event_info {
        std::atomic<int> count{0};
        std::atomic<bool> removed{false};
    };
    event_info info{};

    auto callback = [](void* data) {
        auto* info = static_cast<event_info*>(data);
        info->count++;
    };

    auto removeCallback = [](void* data) {
        auto* info = static_cast<event_info*>(data);
        info->removed = true;
    };

    // When I create a scheduled event with a 0 delay and a 0 interval (one short, directly scheduled)
    celix_scheduled_event_options_t opts{};
    opts.callbackData = &info;
    opts.callback = callback;
    opts.removeCallbackData = &info;
    opts.removeCallback = removeCallback;

    // And I schedule the event
    long eventId = celix_bundleContext_scheduleEvent(ctx->getCBundleContext(), &opts);
    EXPECT_GE(eventId, 0);

    // Then the event is called once
    std::this_thread::sleep_for(std::chrono::milliseconds{10});
    EXPECT_EQ(1, info.count.load());

    // And the event remove callback is called
    EXPECT_TRUE(info.removed.load());
}

TEST_F(ScheduledEventTestSuite, ScheduledEventTest) {
    auto ctx = fw->getFrameworkBundleContext();

    struct event_info {
        std::atomic<int> count{0};
        std::atomic<bool> removed{false};
    };
    event_info info{};

    auto callback = [](void* data) {
        auto* info = static_cast<event_info*>(data);
        info->count++;
    };

    auto removeCallback = [](void* data) {
        auto* info = static_cast<event_info*>(data);
        info->removed = true;
    };

    // When I create a scheduled event with a 10ms delay and a 20 ms interval
    celix_scheduled_event_options_t opts{};
    opts.name = "Scheduled event test";
    opts.initialDelayInSeconds = 0.01;
    opts.intervalInSeconds = 0.02;
    opts.callbackData = &info;
    opts.callback = callback;
    opts.removeCallbackData = &info;
    opts.removeCallback = removeCallback;

    // And I schedule the event
    long eventId = celix_bundleContext_scheduleEvent(ctx->getCBundleContext(), &opts);
    EXPECT_GE(eventId, 0);

    // And wait more than 10 ms + 2x 20ms + 10ms error margin
    std::this_thread::sleep_for(std::chrono::milliseconds{60});

    // Then the event is called at least 3 times
    EXPECT_GE(info.count.load(), 3);

    // And the event remove callback is not called
    EXPECT_FALSE(info.removed.load());

    // And when I remove the event
    celix_bundleContext_removeScheduledEvent(ctx->getCBundleContext(), eventId);

    // Then the event remove callback is called
    EXPECT_TRUE(info.removed.load());
}

TEST_F(ScheduledEventTestSuite, ManyScheduledEventTest) {
    auto ctx = fw->getFrameworkBundleContext();

    struct event_info {
        std::atomic<int> count{0};
    };
    event_info info{};
    auto callback = [](void* data) {
        auto* info = static_cast<event_info*>(data);
        info->count++;
    };

    std::vector<long> eventIds{};

    // When 1000 scheduled events are with a random interval between 1 and 59 ms
    for (int i = 0; i < 100; ++i) {
        // When I create a scheduled event with a 10ms delay and a 20 ms interval
        celix_scheduled_event_options_t opts{};
        opts.name = "Scheduled event test";
        opts.intervalInSeconds = (i % 50) / 100.0; // note will also contain one-shot scheduled events
        opts.callbackData = &info;
        opts.callback = callback;
        long eventId = celix_bundleContext_scheduleEvent(ctx->getCBundleContext(), &opts);
        EXPECT_GE(eventId, 0);
        if (opts.intervalInSeconds > 0) { // not a one-shot event
            eventIds.push_back(eventId);
        }
    }

    // And some time passes, to let some events be called
    std::this_thread::sleep_for(std::chrono::milliseconds{10});

    // Then the events can safely be removed
    for (auto id : eventIds) {
        celix_bundleContext_removeScheduledEvent(ctx->getCBundleContext(), id);
    }
    EXPECT_GT(info.count, 0);
}

TEST_F(ScheduledEventTestSuite, AddWithoutRemoveScheduledEventTest) {
    // When I create a scheduled event
    auto ctx = fw->getFrameworkBundleContext();

    auto callback = [](void* /*data*/) { fprintf(stdout, "Scheduled event called\n"); };
    celix_scheduled_event_options_t opts{};
    opts.name = "Unremoved scheduled event test";
    opts.intervalInSeconds = 0.02;
    opts.callback = callback;
    long scheduledEventId = celix_bundleContext_scheduleEvent(ctx->getCBundleContext(), &opts);
    EXPECT_GE(scheduledEventId, 0);

    // And I do not remove the event, but let the bundle framework stpp
    // Then I expect no memory leaks
}

TEST_F(ScheduledEventTestSuite, AddWithoutRemoveOneShotEventTest) {
    // When I create a one-shot scheduled event with a long initial delay
    auto ctx = fw->getFrameworkBundleContext();

    auto callback = [](void* /*data*/) { FAIL() << "Scheduled event called, but should not be called"; };
    celix_scheduled_event_options_t opts{};
    opts.name = "Unremoved one-shot scheduled event test";
    opts.initialDelayInSeconds = 100;
    opts.callback = callback;
    long scheduledEventId = celix_bundleContext_scheduleEvent(ctx->getCBundleContext(), &opts);
    EXPECT_GE(scheduledEventId, 0);

    // And I do let the one-shot event trigger, but let the bundle framework stop
    // Then I expect no memory leaks
}

TEST_F(ScheduledEventTestSuite, InvalidOptionsAndArgumentsTest) {
    // When I create a scheduled event with an invalid options
    auto ctx = fw->getFrameworkBundleContext();
    celix_scheduled_event_options_t opts{}; // no callback
    long scheduledEventId = celix_bundleContext_scheduleEvent(ctx->getCBundleContext(), &opts);

    // Then I expect an error
    EXPECT_LT(scheduledEventId, 0);

    // celix_scheduleEvent_release and celix_scheduledEvent_retain can be called with NULL
    celix_scheduledEvent_release(nullptr);
    celix_scheduledEvent_retain(nullptr);

    // celix_bundleContext_removeScheduledEvent can handle invalid eventIds
    celix_bundleContext_removeScheduledEvent(ctx->getCBundleContext(), -1);
    celix_bundleContext_removeScheduledEvent(ctx->getCBundleContext(), 404);

    // celix_framework_scheduledEvent with no callback should return -1
    scheduledEventId = celix_framework_scheduleEvent(ctx->getFramework()->getCFramework(),
                                                     CELIX_FRAMEWORK_BUNDLE_ID,
                                                     nullptr,
                                                     0.0,
                                                     0.0,
                                                     nullptr,
                                                     nullptr,
                                                     nullptr,
                                                     nullptr);
    EXPECT_EQ(scheduledEventId, -1);

    // celix_framework_scheduledEvent with an invalid bndId should return -1
    scheduledEventId = celix_framework_scheduleEvent(
        ctx->getFramework()->getCFramework(), 404, nullptr, 0.0, 0.0, nullptr, [](void*) { /*nop*/ }, nullptr, nullptr);
    EXPECT_EQ(scheduledEventId, -1);

    // celix_framework_waitForScheduledEvent with an invalid bndId should return CELIX_ILLEGAL_ARGUMENT
    celix_status_t status = celix_framework_waitForScheduledEvent(
        ctx->getFramework()->getCFramework(), 404, 1);
    EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);
}

TEST_F(ScheduledEventTestSuite, WakeUpEventTest) {
    // Given a counter scheduled event with a long initial delay is added
    std::atomic<int> count{0};
    celix_scheduled_event_options_t opts{};
    opts.name = "test wakeup";
    opts.initialDelayInSeconds = 0.05;
    opts.intervalInSeconds = 0.05;
    opts.callbackData = static_cast<void*>(&count);
    opts.callback = [](void* countPtr) {
        auto* count = static_cast<std::atomic<int>*>(countPtr);
        count->fetch_add(1);
    };
    long scheduledEventId =
        celix_bundleContext_scheduleEvent(fw->getFrameworkBundleContext()->getCBundleContext(), &opts);
    ASSERT_NE(-1L, scheduledEventId);
    EXPECT_EQ(0, count.load());

    // When the scheduled event is woken up
    celix_status_t status = celix_bundleContext_wakeupScheduledEvent(fw->getFrameworkBundleContext()->getCBundleContext(), scheduledEventId);

    // Then the status is CELIX_SUCCESS
    ASSERT_EQ(CELIX_SUCCESS, status);

    // When waiting a bit to let the scheduled event trigger
    std::this_thread::sleep_for(std::chrono::milliseconds{5});

    // Then the count is increased
    EXPECT_EQ(1, count.load());

    // When waiting longer than the interval
    std::this_thread::sleep_for(std::chrono::milliseconds{55});

    // Then the count is increased
    EXPECT_EQ(2, count.load());

    // When waking up the scheduled event again
    status = celix_bundleContext_wakeupScheduledEvent(fw->getFrameworkBundleContext()->getCBundleContext(), scheduledEventId);

    // Then the status is CELIX_SUCCESS
    ASSERT_EQ(CELIX_SUCCESS, status);

    // When waiting a bit to let the scheduled event trigger
    std::this_thread::sleep_for(std::chrono::milliseconds{5});

    // Then the count is increased
    EXPECT_EQ(3, count.load());

    celix_bundleContext_removeScheduledEvent(fw->getFrameworkBundleContext()->getCBundleContext(), scheduledEventId);
}

TEST_F(ScheduledEventTestSuite, WakeUpOneShotEventTest) {
    // Given a counter scheduled event with a long initial delay is added
    std::atomic<int> count{0};
    celix_scheduled_event_options_t opts{};
    opts.name = "test one-shot wakeup";
    opts.initialDelayInSeconds = 5;
    opts.callbackData = static_cast<void*>(&count);
    opts.callback = [](void* countPtr) {
        auto* count = static_cast<std::atomic<int>*>(countPtr);
        count->fetch_add(1);
    };
    long scheduledEventId =
        celix_bundleContext_scheduleEvent(fw->getFrameworkBundleContext()->getCBundleContext(), &opts);
    ASSERT_GE(scheduledEventId, 0);
    EXPECT_EQ(0, count.load());

    // When the scheduled event is woken up
    celix_status_t status = celix_bundleContext_wakeupScheduledEvent(
        fw->getFrameworkBundleContext()->getCBundleContext(), scheduledEventId);

    // Then the status is CELIX_SUCCESS
    ASSERT_EQ(CELIX_SUCCESS, status);

    // When waiting a bit to ensure the scheduled event is woken up
    std::this_thread::sleep_for(std::chrono::milliseconds{5});

    // And the count is increased
    EXPECT_EQ(1, count.load());

    // And when the scheduled event is woken up again
    status = celix_bundleContext_wakeupScheduledEvent(fw->getFrameworkBundleContext()->getCBundleContext(),
                                                      scheduledEventId);

    // Then the status is ILLEGAL_ARGUMENT, because the scheduled event is already woken up and a one-shot event
    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST_F(ScheduledEventTestSuite, CxxScheduledEventTest) {
    // Given a count and a callback to increase the count
    std::atomic<int> count{0};
    auto callback = [&count]() { count.fetch_add(1); };

    std::atomic<bool> removed{false};
    auto removeCallback = [&removed]() { removed.store(true); };

    // And a scheduled event with a initial delay and interval of 50ms
    auto event = fw->getFrameworkBundleContext()
                     ->scheduledEvent()
                     .withName("test cxx")
                     .withInitialDelay(std::chrono::milliseconds{50})
                     .withInterval(std::chrono::milliseconds{50})
                     .withCallback(callback)
                     .withRemoveCallback(removeCallback)
                     .build();

    // Then the count is not yet increased
    ASSERT_EQ(0, count.load());

    // When waiting longer than the initial delay
    std::this_thread::sleep_for(std::chrono::milliseconds{55});

    // Then the count is increased
    EXPECT_EQ(1, count.load());

    // When waking up the event
    event.wakeup();

    // And waiting a bit to ensure the event is called
    std::this_thread::sleep_for(std::chrono::milliseconds{5});

    // Then the count is increased
    EXPECT_EQ(2, count.load());

    // When waking up the event without waiting
    event.wakeup();

    // And waiting a bit to ensure the event is called
    std::this_thread::sleep_for(std::chrono::milliseconds{5});

    // Then the count is increased
    EXPECT_EQ(3, count.load());

    // And the remove callback is not yet called
    EXPECT_FALSE(removed.load());

    // When canceling the event
    event.cancel();

    // And waiting longer than the interval
    std::this_thread::sleep_for(std::chrono::milliseconds{55});

    // Then the count is not increased
    EXPECT_EQ(3, count.load());

    // And the remove callback is called
    EXPECT_TRUE(removed.load());
}

TEST_F(ScheduledEventTestSuite, CxxScheduledEventRAIITest) {
    // Given a count and a callback to increase the count
    std::atomic<int> count{0};
    auto callback = [&count]() { count.fetch_add(1); };

    std::atomic<bool> removed{false};
    auto removeCallback = [&removed]() { removed.store(true); };

    {
        // And a scoped scheduled event with an initial delay and interval of 50ms
        auto event = fw->getFrameworkBundleContext()
                         ->scheduledEvent()
                         .withName("test cxx")
                         .withInitialDelay(std::chrono::milliseconds{50})
                         .withInterval(std::chrono::milliseconds{50})
                         .withCallback(callback)
                         .withRemoveCallback(removeCallback)
                         .build();

        // Then the count is not yet increased
        ASSERT_EQ(0, count.load());
    }
    // When the event goes out of scope

    // When waiting longer than the initial delay
    std::this_thread::sleep_for(std::chrono::milliseconds{60});

    // Then the remove callback is called
    EXPECT_TRUE(removed.load());

    // And the count is not increased
    EXPECT_EQ(0, count.load());
}

TEST_F(ScheduledEventTestSuite, CxxOneShotScheduledEventTest) {
    // Given a count and a callback to increase the count
    std::atomic<int> count{0};
    auto callback = [&count]() { count.fetch_add(1); };

    std::atomic<bool> removed{false};
    auto removeCallback = [&removed]() { removed.store(true); };

    // And a scheduled event with a initial delay of 50ms
    auto event = fw->getFrameworkBundleContext()
                     ->scheduledEvent()
                     .withName("test cxx one-shot")
                     .withInitialDelay(std::chrono::milliseconds{50})
                     .withCallback(callback)
                     .withRemoveCallback(removeCallback)
                     .build();

    // Then the count is not yet increased
    ASSERT_EQ(0, count.load());

    // And the remove callback is not yet called
    EXPECT_FALSE(removed.load());

    // When waiting longer than the initial delay
    std::this_thread::sleep_for(std::chrono::milliseconds{60});

    // Then the count is increased
    EXPECT_EQ(1, count.load());

    // And the remove callback is called
    EXPECT_TRUE(removed.load());

    // When waking up the event with a wait time of 1s
    event.wakeup();

    // And waiting a bit to ensure the event is called
    std::this_thread::sleep_for(std::chrono::milliseconds{2});

    // Then the count is not increased, because the event is a one-shot event
    EXPECT_EQ(1, count.load());

    // When the event goes out of scope, it does not leak
}

TEST_F(ScheduledEventTestSuite, CxxOneShotScheduledEventRAIITest) {
    // Given a count and a callback to increase the count
    std::atomic<int> count{0};
    auto callback = [&count]() { count.fetch_add(1); };

    std::atomic<bool> removed{false};
    auto removeCallback = [&removed]() { removed.store(true); };

    {
        // And a scoped scheduled event with a initial delay of 50ms
        auto event = fw->getFrameworkBundleContext()
                         ->scheduledEvent()
                         .withName("test cxx one-shot")
                         .withInitialDelay(std::chrono::milliseconds{50})
                         .withCallback(callback)
                         .withRemoveCallback(removeCallback)
                         .build();

        // Then the count is not yet increased
        ASSERT_EQ(0, count.load());
    }
    // When the event is out of scope

    // Then the remove callback is not yet called
    EXPECT_FALSE(removed.load());

    // And waiting longer than the initial delay
    std::this_thread::sleep_for(std::chrono::milliseconds{60});

    // Then the count is increased, because one-shot events are not canceled when out of scope
    EXPECT_EQ(1, count.load());

    // And the remove callback is called
    EXPECT_TRUE(removed.load());
}

TEST_F(ScheduledEventTestSuite, CxxCreateScheduledEventWithNoCallbackTest) {
    EXPECT_ANY_THROW(fw->getFrameworkBundleContext()->scheduledEvent().build()); // Note no callback
}

TEST_F(ScheduledEventTestSuite, CxxCancelOneShotEventBeforeFiredTest) {
    auto callback = []() {
        FAIL() << "Should not be called";
    };

    //Given a one shot scheduled event with a initial delay of 1s
    auto event = fw->getFrameworkBundleContext()->scheduledEvent()
                     .withInitialDelay(std::chrono::seconds{1})
                     .withCallback(callback)
                     .build();

    //When the event is cancelled before the initial delay
    event.cancel();

    //Then the event is not fired and does not leak
}

TEST_F(ScheduledEventTestSuite, RemoveScheduledEventAsync) {
    std::atomic<int> count{0};
    auto callback = [](void* data) {
        auto* count = static_cast<std::atomic<int>*>(data);
        count->fetch_add(1);
    };

    //Given a scheduled event with am initial delay of 1ms
    celix_scheduled_event_options_t opts{};
    opts.initialDelayInSeconds = 0.001;
    opts.callbackData = &count;
    opts.callback = callback;
    long eventId = celix_bundleContext_scheduleEvent(fw->getFrameworkBundleContext()->getCBundleContext(), &opts);
    EXPECT_GE(eventId, 0);

    //When the event is removed async
    celix_bundleContext_removeScheduledEventAsync(fw->getFrameworkBundleContext()->getCBundleContext(), eventId);

    //And waiting longer than the initial delay
    std::this_thread::sleep_for(std::chrono::milliseconds{10});

    //Then the event is not fired
    EXPECT_EQ(0, count.load());
}

TEST_F(ScheduledEventTestSuite, WaitForScheduledEvent) {
    std::atomic<int> count{0};
    auto callback = [](void* data) {
        auto* count = static_cast<std::atomic<int>*>(data);
        count->fetch_add(1);
    };

    // Given a scheduled event with an initial delay of 1ms and an interval of 1ms
    celix_scheduled_event_options_t opts{};
    opts.initialDelayInSeconds = 0.001;
    opts.intervalInSeconds = 0.001;
    opts.callbackData = &count;
    opts.callback = callback;
    long eventId = celix_bundleContext_scheduleEvent(fw->getFrameworkBundleContext()->getCBundleContext(), &opts);
    EXPECT_GE(eventId, 0);

    //When waiting for the event with a timeout longer than the initial delay
    auto status =
        celix_bundleContext_waitForScheduledEvent(fw->getFrameworkBundleContext()->getCBundleContext(), eventId, 1);

    //Then the return status is success
    EXPECT_EQ(CELIX_SUCCESS, status);

    //And the event is fired
    EXPECT_EQ(1, count.load());

    //When waiting too short for the event
    status = celix_bundleContext_waitForScheduledEvent(fw->getFrameworkBundleContext()->getCBundleContext(), eventId, 0.0001);

    //Then the return status is timeout
    EXPECT_EQ(ETIMEDOUT, status);

    //When waiting for the event with a timeout longer than the interval
    status = celix_bundleContext_waitForScheduledEvent(fw->getFrameworkBundleContext()->getCBundleContext(), eventId, 0.0012);

    //Then the return status is success
    EXPECT_EQ(CELIX_SUCCESS, status);

    //And the event is fired again
    EXPECT_EQ(2, count.load());

    celix_bundleContext_removeScheduledEvent(fw->getFrameworkBundleContext()->getCBundleContext(), eventId);
}

TEST_F(ScheduledEventTestSuite, CxxWaitForScheduledEvent) {
    std::atomic<int> count{0};
    auto callback = [&count]() {
        count.fetch_add(1);
    };

    // Given a scheduled event with an initial delay of 1ms and an interval of 1ms
    auto event = fw->getFrameworkBundleContext()->scheduledEvent()
                     .withInitialDelay(std::chrono::milliseconds{1})
                     .withInterval(std::chrono::milliseconds{1})
                     .withCallback(callback)
                     .build();

    //When waiting for the event with a timeout longer than the initial delay
    auto success = event.waitFor(std::chrono::milliseconds{1000});

    //Then the return status is success
    EXPECT_TRUE(success);

    //And the event is fired
    EXPECT_EQ(1, count.load());

    //When waiting to short for the event
    success = event.waitFor(std::chrono::microseconds {1});

    //Then the return status is false (timeout)
    EXPECT_FALSE(success);

    //When waiting for the event with a timeout longer than the interval
    success = event.waitFor(std::chrono::milliseconds{1000});

    //Then the return status is success
    EXPECT_TRUE(success);

    //And the event is fired again
    EXPECT_EQ(2, count.load());
}
