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
#if defined(__APPLE__) || defined(TESTING_ON_CI)
    const int ALLOWED_ERROR_MARGIN_IN_MS = 2000;
#else
    const int ALLOWED_ERROR_MARGIN_IN_MS = 1000;
#endif

    const double ALLOWED_PROCESSING_TIME_IN_SECONDS = 0.1;

    ScheduledEventTestSuite() {
        fw = celix::createFramework({{"CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "trace"},
                                     {CELIX_ALLOWED_PROCESSING_TIME_FOR_SCHEDULED_EVENT_IN_SECONDS,
                                      std::to_string(ALLOWED_PROCESSING_TIME_IN_SECONDS)}});
    }
    std::shared_ptr<celix::Framework> fw{};
    /**
     * Wait for the given predicate to become true or the given time has elapsed.
     * @param predicate predicate to check.
     * @param within maximum time to wait.
     */
    template <typename Rep, typename Period>
    void waitFor(const std::function<bool()>& predicate, std::chrono::duration<Rep, Period> within) {
        auto start = std::chrono::steady_clock::now();
        while (!predicate()) {
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
            if (elapsed > within) {
                break;
            }
        }
    }
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

    // Then the count becomes 1 within the error margin
    waitFor([&]() { return info.count.load() == 1; }, std::chrono::milliseconds{ALLOWED_ERROR_MARGIN_IN_MS});
    EXPECT_EQ(1, info.count.load());

    // And the event remove callback is called  within the error margin
    waitFor([&]() { return info.removed.load(); }, std::chrono::milliseconds{ALLOWED_ERROR_MARGIN_IN_MS});
    EXPECT_TRUE(info.removed.load());

    celix_bundleContext_removeScheduledEvent(ctx->getCBundleContext(), eventId);
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

    // Then count becomes 3 or more within the initial delay + 2 x internal and an allowed error margin (3x)
    int allowedTimeInMs = 10 + (2 * 20) + (3 * ALLOWED_ERROR_MARGIN_IN_MS);
    waitFor([&]() { return info.count.load() >= 3; }, std::chrono::milliseconds{allowedTimeInMs});
    EXPECT_GE(info.count.load(), 3);

    // And the event remove callback is not called
    EXPECT_FALSE(info.removed.load());

    // And when I remove the event
    celix_bundleContext_removeScheduledEvent(ctx->getCBundleContext(), eventId);

    // Then the event remove callback is called
    EXPECT_TRUE(info.removed.load());
}

TEST_F(ScheduledEventTestSuite, IgnoreNegativeScheduledIdsTest) {
    // Scheduled event wakeup, remove functions will ignore negative ids
    EXPECT_EQ(CELIX_SUCCESS,
              celix_bundleContext_wakeupScheduledEvent(fw->getFrameworkBundleContext()->getCBundleContext(), -1));
    EXPECT_EQ(CELIX_SUCCESS,
              celix_bundleContext_removeScheduledEvent(fw->getFrameworkBundleContext()->getCBundleContext(), -1));
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
    opts.name = "Un-removed scheduled event test";
    opts.intervalInSeconds = 0.02;
    opts.callback = callback;
    long scheduledEventId = celix_bundleContext_scheduleEvent(ctx->getCBundleContext(), &opts);
    EXPECT_GE(scheduledEventId, 0);

    // And I do not remove the event, but let the bundle framework stop
    // Then I expect no memory leaks
}

TEST_F(ScheduledEventTestSuite, AddWithoutRemoveOneShotEventTest) {
    // When I create a one-shot scheduled event with a long initial delay
    auto ctx = fw->getFrameworkBundleContext();

    auto callback = [](void* /*data*/) { FAIL() << "Scheduled event called, but should not be called"; };
    celix_scheduled_event_options_t opts{};
    opts.name = "Un-removed one-shot scheduled event test";
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

    // When I create a scheduled event with negative initial delay
    opts.name = "Invalid scheduled event test";
    opts.initialDelayInSeconds = -1;
    opts.callback = [](void* /*data*/) { FAIL() << "Scheduled event called, but should not be called"; };
    scheduledEventId = celix_bundleContext_scheduleEvent(ctx->getCBundleContext(), &opts);
    // Then I expect an error
    EXPECT_LT(scheduledEventId, 0);

    // When I create a scheduled event with negative interval value
    opts.initialDelayInSeconds = 0.1;
    opts.intervalInSeconds = -1;
    scheduledEventId = celix_bundleContext_scheduleEvent(ctx->getCBundleContext(), &opts);
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
    celix_status_t status = celix_framework_waitForScheduledEvent(ctx->getFramework()->getCFramework(), 404, 1);
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
    celix_status_t status = celix_bundleContext_wakeupScheduledEvent(
        fw->getFrameworkBundleContext()->getCBundleContext(), scheduledEventId);

    // Then the status is CELIX_SUCCESS
    ASSERT_EQ(CELIX_SUCCESS, status);

    // And the count becomes 1 within the error margin
    waitFor([&]() { return count.load() == 1; }, std::chrono::milliseconds{ALLOWED_ERROR_MARGIN_IN_MS});
    EXPECT_EQ(1, count.load());

    // And the count becomes 2 within the interval including the error margin
    auto now = std::chrono::steady_clock::now();
    waitFor([&]() { return count.load() == 2; }, std::chrono::milliseconds{50 + ALLOWED_ERROR_MARGIN_IN_MS});
    auto end = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - now).count();
    EXPECT_EQ(2, count.load());
    EXPECT_NEAR(50, diff, ALLOWED_ERROR_MARGIN_IN_MS);

    // When waking up the scheduled event again
    status = celix_bundleContext_wakeupScheduledEvent(fw->getFrameworkBundleContext()->getCBundleContext(),
                                                      scheduledEventId);

    // Then the status is CELIX_SUCCESS
    ASSERT_EQ(CELIX_SUCCESS, status);

    // Then the count becomes 3 within the error margin
    waitFor([&]() { return count.load() == 3; }, std::chrono::milliseconds{ALLOWED_ERROR_MARGIN_IN_MS});
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

    // And the count becomes 1 within the error margin
    waitFor([&]() { return count.load() == 1; }, std::chrono::milliseconds{ALLOWED_ERROR_MARGIN_IN_MS});
    EXPECT_EQ(1, count.load());

    // When the scheduled event is woken up again
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

    // And the count becomes 1 within the initial delay, including the error margin
    auto start = std::chrono::steady_clock::now();
    waitFor([&]() { return count.load() == 1; }, std::chrono::milliseconds{50 + ALLOWED_ERROR_MARGIN_IN_MS});
    auto end = std::chrono::steady_clock::now();
    EXPECT_NEAR(
        50, std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), ALLOWED_ERROR_MARGIN_IN_MS);
    EXPECT_EQ(1, count.load());

    // When waking up the event
    event.wakeup();

    // Then the count is increased with the error margin
    waitFor([&]() { return count.load() == 2; }, std::chrono::milliseconds{ALLOWED_ERROR_MARGIN_IN_MS});
    EXPECT_EQ(2, count.load());

    // And the remove callback is not yet called
    EXPECT_FALSE(removed.load());

    // When canceling the event
    event.cancel();

    // And waiting longer than the interval
    std::this_thread::sleep_for(std::chrono::milliseconds{50 + ALLOWED_ERROR_MARGIN_IN_MS});

    // Then the count is not increased
    EXPECT_EQ(2, count.load());

    // And the remove callback is called within the error margin
    waitFor([&]() { return removed.load(); }, std::chrono::milliseconds{ALLOWED_ERROR_MARGIN_IN_MS});
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
                         .withInitialDelay(std::chrono::milliseconds{100})
                         .withInterval(std::chrono::milliseconds{50})
                         .withCallback(callback)
                         .withRemoveCallback(removeCallback)
                         .build();

        // Then the count is not yet increased
        ASSERT_EQ(0, count.load());
    }
    // When the event goes out of scope

    // Then the event removed callback is called within the allowed error margin
    waitFor([&]() { return removed.load(); }, std::chrono::milliseconds{ALLOWED_ERROR_MARGIN_IN_MS});
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

    // And count will be increased within the initial delay (including some error margin)
    waitFor([&] { return count.load() == 1; }, std::chrono::milliseconds{50 + ALLOWED_ERROR_MARGIN_IN_MS});
    EXPECT_EQ(1, count.load());

    // And the remove callback is called shortly after the initial delay, within the error margin
    waitFor([&] { return removed.load(); }, std::chrono::milliseconds{ALLOWED_ERROR_MARGIN_IN_MS});
    EXPECT_TRUE(removed.load());

    // When waking up the event with a wait time of 1s
    event.wakeup();

    // And waiting a bit
    std::this_thread::sleep_for(std::chrono::milliseconds{ALLOWED_ERROR_MARGIN_IN_MS});

    // Then the count is not increased, because the event is a one-shot event
    EXPECT_EQ(1, count.load());

    // When the event goes out of scope, it does not leak
}

TEST_F(ScheduledEventTestSuite, CxxOneShotScheduledEventRAIITest) {
    // Given a count and a callback to increase the count
    std::atomic<int> count{0};
    auto callback = [&count]() { count.fetch_add(1); };

    // And a remove boolean and a remove callback to set the boolean
    std::atomic<bool> removed{false};
    auto removeCallback = [&removed]() { removed.store(true); };

    {
        // And a scoped scheduled event with an initial delay of 50ms
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
    }
    // When the event is out of scope

    // Then the remove callback is not yet called, because a one-shot event is not canceled when it goes out of scope
    EXPECT_FALSE(removed.load());

    // And count will be increased within the initial delay (including some error margin)
    waitFor([&] { return count.load() == 1; }, std::chrono::milliseconds{50 + ALLOWED_ERROR_MARGIN_IN_MS});
    EXPECT_EQ(1, count.load());

    // And the remove callback is called shortly after the initial delay
    waitFor([&] { return removed.load(); }, std::chrono::milliseconds{ALLOWED_ERROR_MARGIN_IN_MS});
    EXPECT_TRUE(removed.load());
}

TEST_F(ScheduledEventTestSuite, CxxCreateScheduledEventWithNoCallbackTest) {
    // When a scheduled event is created without a callback an exception is thrown
    EXPECT_ANY_THROW(fw->getFrameworkBundleContext()->scheduledEvent().build()); // Note no callback
}

TEST_F(ScheduledEventTestSuite, CxxCancelOneShotEventBeforeFiredTest) {
    auto callback = []() { FAIL() << "Should not be called"; };

    // Given a one shot scheduled event with an initial delay of 1s
    auto event = fw->getFrameworkBundleContext()
                     ->scheduledEvent()
                     .withInitialDelay(std::chrono::seconds{1})
                     .withCallback(callback)
                     .build();

    // When the event is cancelled before the initial delay
    event.cancel();

    // And waiting a bit
    std::this_thread::sleep_for(std::chrono::milliseconds{ALLOWED_ERROR_MARGIN_IN_MS});

    // Then the event is not fired and does not leak
}

TEST_F(ScheduledEventTestSuite, RemoveScheduledEventAsyncTest) {
    std::atomic<int> count{0};
    auto callback = [](void* data) {
        auto* count = static_cast<std::atomic<int>*>(data);
        count->fetch_add(1);
    };

    // Given a scheduled event with am initial delay of 1ms
    celix_scheduled_event_options_t opts{};
    opts.initialDelayInSeconds = 0.01;
    opts.callbackData = &count;
    opts.callback = callback;
    long eventId = celix_bundleContext_scheduleEvent(fw->getFrameworkBundleContext()->getCBundleContext(), &opts);
    EXPECT_GE(eventId, 0);

    // When the event is removed async
    celix_bundleContext_removeScheduledEventAsync(fw->getFrameworkBundleContext()->getCBundleContext(), eventId);

    // And waiting longer than the initial delay, including some error margin
    std::this_thread::sleep_for(std::chrono::milliseconds{10 + ALLOWED_ERROR_MARGIN_IN_MS});

    // Then the event is not fired
    EXPECT_EQ(0, count.load());
}

TEST_F(ScheduledEventTestSuite, WaitForScheduledEventTest) {
    std::atomic<int> count{0};
    auto callback = [](void* data) {
        auto* count = static_cast<std::atomic<int>*>(data);
        count->fetch_add(1);
    };

    // Given a scheduled event with an initial delay of 10ms and an interval of 10ms
    celix_scheduled_event_options_t opts{};
    opts.initialDelayInSeconds = 0.01;
    opts.intervalInSeconds = 0.01;
    opts.callbackData = &count;
    opts.callback = callback;
    long eventId = celix_bundleContext_scheduleEvent(fw->getFrameworkBundleContext()->getCBundleContext(), &opts);
    EXPECT_GE(eventId, 0);

    // When waiting for the event with a timeout longer than the initial delay
    auto status =
        celix_bundleContext_waitForScheduledEvent(fw->getFrameworkBundleContext()->getCBundleContext(), eventId, 2);

    // Then the return status is success
    EXPECT_EQ(CELIX_SUCCESS, status) << "Unexpected status" << celix_strerror(status) << std::endl;

    // And the event is fired
    EXPECT_EQ(1, count.load());

    // When waiting for the event with a timeout longer than the interval
    status =
        celix_bundleContext_waitForScheduledEvent(fw->getFrameworkBundleContext()->getCBundleContext(), eventId, 2);

    // Then the return status is success
    EXPECT_EQ(CELIX_SUCCESS, status);

    // And the event is fired again
    EXPECT_EQ(2, count.load());

    celix_bundleContext_removeScheduledEvent(fw->getFrameworkBundleContext()->getCBundleContext(), eventId);
}

TEST_F(ScheduledEventTestSuite, WaitTooShortForScheduledEventTest) {
    std::atomic<int> count{0};
    auto callback = [](void *data) {
        auto *count = static_cast<std::atomic<int> *>(data);
        count->fetch_add(1);
    };

    // Given a scheduled event with an initial delay of 1s and an interval of 1s
    celix_scheduled_event_options_t opts{};
    opts.initialDelayInSeconds = 1;
    opts.intervalInSeconds = 1;
    opts.callbackData = &count;
    opts.callback = callback;
    long eventId = celix_bundleContext_scheduleEvent(fw->getFrameworkBundleContext()->getCBundleContext(), &opts);
    EXPECT_GE(eventId, 0);

    // When waiting too short for the event
    celix_status_t status = celix_bundleContext_waitForScheduledEvent(
            fw->getFrameworkBundleContext()->getCBundleContext(), eventId, 0.0001);

    // Then the return status is timeout
    EXPECT_EQ(ETIMEDOUT, status);

    // And th event is not fired
    EXPECT_EQ(0, count.load());

    // When event is woken up
    status = celix_bundleContext_wakeupScheduledEvent(fw->getFrameworkBundleContext()->getCBundleContext(), eventId);

    // Then the return status is success
    EXPECT_EQ(CELIX_SUCCESS, status);

    // And the event will be fired
    waitFor([&count]() { return count.load() == 1; }, std::chrono::milliseconds{ALLOWED_ERROR_MARGIN_IN_MS});
    EXPECT_EQ(1, count.load());

    // When waiting too short for the next (interval based) event
    status = celix_bundleContext_waitForScheduledEvent(
            fw->getFrameworkBundleContext()->getCBundleContext(), eventId, 0.0001);

    // Then the return status is timeout
    EXPECT_EQ(ETIMEDOUT, status);

    celix_bundleContext_removeScheduledEvent(fw->getFrameworkBundleContext()->getCBundleContext(), eventId);
}

TEST_F(ScheduledEventTestSuite, CxxWaitForScheduledEventTest) {
    std::atomic<int> count{0};
    auto callback = [&count]() { count.fetch_add(1); };

    // Given a scheduled event with an initial delay of 1ms and an interval of 1ms
    auto event = fw->getFrameworkBundleContext()
                     ->scheduledEvent()
                     .withInitialDelay(std::chrono::milliseconds{10})
                     .withInterval(std::chrono::milliseconds{10})
                     .withCallback(callback)
                     .build();

    // When waiting for the event with a timeout longer than the initial delay
    auto success = event.waitFor(std::chrono::milliseconds{10 + ALLOWED_ERROR_MARGIN_IN_MS});

    // Then the return status is success
    EXPECT_TRUE(success);

    // And the event is fired
    EXPECT_EQ(1, count.load());

    // When waiting to short for the event
    success = event.waitFor(std::chrono::microseconds{1});

    // Then the return status is false (timeout)
    EXPECT_FALSE(success);

    // When waiting for the event with a timeout longer than the interval
    success = event.waitFor(std::chrono::milliseconds{10 + ALLOWED_ERROR_MARGIN_IN_MS});

    // Then the return status is success
    EXPECT_TRUE(success);

    // And the event is fired again
    EXPECT_EQ(2, count.load());
}

#ifndef __APPLE__
TEST_F(ScheduledEventTestSuite, ScheduledEventTimeoutLogTest) {
    //Disabled for __APPLE__, because the expected timeout in celix_scheduledEvent_waitForRemoved does not always
    //trigger. see also ticket #587.

    //Given a framework with a log callback that counts the number of warning log messages
    std::atomic<int> logCount{0};
    auto logCallback = [](void* handle, celix_log_level_e level, const char*, const char*, int, const char* format, va_list args){
        std::atomic<int>& count = *static_cast<std::atomic<int>*>(handle);
        if (level == CELIX_LOG_LEVEL_WARNING) {
            count.fetch_add(1);
        }
        FILE* output = stdout;
        if (level == CELIX_LOG_LEVEL_FATAL || level == CELIX_LOG_LEVEL_ERROR || level == CELIX_LOG_LEVEL_WARNING) {
            output = stderr;
        }
        fprintf(output, "%s: ", celix_logLevel_toString(level));
        vfprintf(output, format, args);
        fprintf(output, "\n");
    };
    celix_framework_setLogCallback(fw->getCFramework(), &logCount, logCallback);


    //And a scheduled event with an initial delay of 1ms and an interval of 1ms that waits for 200ms in the callback
    //and remove callback.

    auto busyWaitCallback = [](void*){
        auto start = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start < std::chrono::milliseconds{200}) {
            //busy wait
        }
    };

    celix_scheduled_event_options_t opts{};
    opts.name = "Sleep while Processing and Removing Scheduled Event";
    opts.initialDelayInSeconds = 0.01;
    opts.intervalInSeconds = 0.01;
    opts.callback = busyWaitCallback;
    opts.removeCallback = busyWaitCallback;
    long eventId = celix_bundleContext_scheduleEvent(fw->getFrameworkBundleContext()->getCBundleContext(), &opts);

    //When the event is woken up
    celix_bundleContext_wakeupScheduledEvent(fw->getFrameworkBundleContext()->getCBundleContext(), eventId);

    // Then the log callback is called with a warning log message within an error margin and processing sleep time,
    // because callback took too long.
    waitFor([&] { return logCount.load() == 1; },
            std::chrono::milliseconds{ALLOWED_ERROR_MARGIN_IN_MS + 200});
    EXPECT_EQ(1, logCount.load());

    //When removing the event
    celix_bundleContext_removeScheduledEvent(fw->getFrameworkBundleContext()->getCBundleContext(), eventId);

    //Then the log callback is called at least one more time with a warning log message, because remove
    //callback took too long
    //(note the logCount can be increased more than once, due to another processing thread)
    EXPECT_GE(logCount.load(), 2);
}
#endif
