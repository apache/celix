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

#include "malloc_ei.h"

class ScheduledEventWithErrorInjectionTestSuite : public ::testing::Test {
public:
    ScheduledEventWithErrorInjectionTestSuite() {
        fw = celix::createFramework({
            {"CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "info"},
            {CELIX_FRAMEWORK_CONDITION_SERVICES_ENABLED, "false"}
        });
    }

    ~ScheduledEventWithErrorInjectionTestSuite() noexcept override {
        celix_ei_expect_malloc(nullptr, 0, nullptr);
    }

    std::shared_ptr<celix::Framework> fw{};
};


TEST_F(ScheduledEventWithErrorInjectionTestSuite, MallocFailsTest) {
    //Given malloc is primed to fail on the first call from celix_scheduledEvent_create (whitebox knowledge)
    celix_ei_expect_malloc((void*)celix_scheduledEvent_create, 0, nullptr);

    //When a scheduled event is added
    celix_scheduled_event_options_t opts{};
    opts.name = "malloc fail test";
    opts.callback = [](void*){/*nop*/};
    long scheduledEventId = celix_bundleContext_scheduleEvent(fw->getFrameworkBundleContext()->getCBundleContext(),
                                                              &opts);

    //Then the scheduled event id is -1 (error)
    EXPECT_EQ(-1L, scheduledEventId);
}
