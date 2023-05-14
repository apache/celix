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
#include "framework_private.h"

class ScheduledEventTestSuite : public ::testing::Test {
public:
    ScheduledEventTestSuite() {
        fw = celix::createFramework({{"CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "info"}});
    }

    std::shared_ptr<celix::Framework> fw{};
};

TEST_F(ScheduledEventTestSuite, OnceShotEventTest) {
    auto ctx = fw->getFrameworkBundleContext();

    struct event_info {
        std::atomic<int> count{0};
    };
    event_info info{};

    auto callback = [](void *data) {
        auto* info = static_cast<event_info*>(data);
        info->count++;
    };

    //When I create a scheduled event with a 0 delay and a 0 interval (one short, directly scheduled)
    celix_scheduled_event_options_t opts{};
    opts.eventData = &info;
    opts.eventCallback = callback;


    //And I schedule the event
    long eventId = celix_bundleContext_addScheduledEvent(ctx->getCBundleContext(), &opts);
    EXPECT_GE(eventId, 0);

    //Then the event is called once
    std::this_thread::sleep_for(std::chrono::milliseconds{10});
    EXPECT_EQ(1, info.count.load());
}

TEST_F(ScheduledEventTestSuite, ScheduledEventTest) {
    auto ctx = fw->getFrameworkBundleContext();

    struct event_info {
        std::atomic<int> count{0};
    };
    event_info info{};

    auto callback = [](void *data) {
        auto* info = static_cast<event_info*>(data);
        info->count++;
    };

    //When I create a scheduled event with a 10ms delay and a 20 ms interval
    celix_scheduled_event_options_t opts{};
    opts.eventName = "Scheduled event test";
    opts.initialDelayInSeconds = 0.01;
    opts.intervalInSeconds = 0.02;
    opts.eventData = &info;
    opts.eventCallback = callback;

    //And I schedule the event
    long eventId = celix_bundleContext_addScheduledEvent(ctx->getCBundleContext(), &opts);
    EXPECT_GE(eventId, 0);

    //And wait more than 10 ms + 2x 20ms + 10ms error margin
    std::this_thread::sleep_for(std::chrono::milliseconds{60});

    //Then the event is called at least 3 times
    EXPECT_GE(info.count.load(),3);

    //And when I remove the event
    celix_bundleContext_removeScheduledEvent(ctx->getCBundleContext(), eventId);
}

TEST_F(ScheduledEventTestSuite, ManyScheduledEventTest) {
    auto ctx = fw->getFrameworkBundleContext();

    struct event_info {
        std::atomic<int> count{0};
    };
    event_info info{};
    auto callback = [](void *data) {
        auto* info = static_cast<event_info*>(data);
        info->count++;
    };

    std::vector<long> eventIds{};

    //When 1000 scheduled events are with a random interval between 1 and 59 ms
    for (int i = 0; i < 1000; ++i) {
        //When I create a scheduled event with a 10ms delay and a 20 ms interval
        celix_scheduled_event_options_t opts{};
        opts.eventName = "Scheduled event test";
        opts.intervalInSeconds = (i % 50) / 100.0; //note will also contain one-shot scheduled events
        opts.eventData = &info;
        opts.eventCallback = callback;
        long eventId = celix_bundleContext_addScheduledEvent(ctx->getCBundleContext(), &opts);
        EXPECT_GE(eventId, 0);
        if (opts.intervalInSeconds > 0) { //not a one-shot event
            eventIds.push_back(eventId);
        }
    }

    //And some time passes, to let some events be called
    std::this_thread::sleep_for(std::chrono::milliseconds{10});

    //Then the events can safely be removed
    for (auto id : eventIds) {
        celix_bundleContext_removeScheduledEvent(ctx->getCBundleContext(), id);
    }
    EXPECT_GT(info.count, 0);
}


TEST_F(ScheduledEventTestSuite, AddWithoutRemoveScheduledEventTest) {
    //When I create a scheduled event
    auto ctx = fw->getFrameworkBundleContext();

    auto callback = [](void */*data*/) {
        fprintf(stdout, "Scheduled event called\n");
    };
    celix_scheduled_event_options_t opts{};
    opts.eventName = "Unremoved scheduled event test";
    opts.intervalInSeconds = 0.02;
    opts.eventCallback = callback;
    long scheduledEventId = celix_bundleContext_addScheduledEvent(ctx->getCBundleContext(), &opts);
    EXPECT_GE(scheduledEventId, 0);

    //And I do not remove the event, but let the bundle framework stpp
    //Then I expect no memory leaks
}

TEST_F(ScheduledEventTestSuite, AddWithoutRemoveOneShotEventTest) {
    //When I create a one-shot scheduled event with a long initial delay
    auto ctx = fw->getFrameworkBundleContext();

    auto callback = [](void */*data*/) {
        FAIL() << "Scheduled event called, but should not be called";
    };
    celix_scheduled_event_options_t opts{};
    opts.eventName = "Unremoved one-shot scheduled event test";
    opts.initialDelayInSeconds = 100;
    opts.eventCallback = callback;
    long scheduledEventId = celix_bundleContext_addScheduledEvent(ctx->getCBundleContext(), &opts);
    EXPECT_GE(scheduledEventId, 0);

    //And I do let the one-shot event trigger, but let the bundle framework stop
    //Then I expect no memory leaks
}

TEST_F(ScheduledEventTestSuite, InvalidOptionsAndArgumentsTest) {
    //When I create a scheduled event with an invalid options
    auto ctx = fw->getFrameworkBundleContext();
    celix_scheduled_event_options_t opts{}; //no callback
    long scheduledEventId = celix_bundleContext_addScheduledEvent(ctx->getCBundleContext(), &opts);

    //Then I expect an error
    EXPECT_LT(scheduledEventId, 0);

    //celix_scheduleEvent_destroy and celix_scheduledEvent_waitAndDestroy can be called with NULL
    celix_scheduledEvent_destroy(nullptr);
    celix_scheduledEvent_waitAndDestroy(nullptr);
}
