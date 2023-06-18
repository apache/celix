---
title: Apache Celix Events
---

<!--
Licensed to the Apache Software Foundation (ASF) under one or more
contributor license agreements.  See the NOTICE file distributed with
this work for additional information regarding copyright ownership.
The ASF licenses this file to You under the Apache License, Version 2.0
(the "License"); you may not use this file except in compliance with
the License.  You may obtain a copy of the License at
   
    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
-->

# Apache Celix Scheduled Events

Apache Celix provides supports to schedule events on the Apache Celix Event Thread. This allows users to 
reuse the existing Apache Celix events thread to do tasks on the event thread.

Scheduled events will be called repeatedly using a provided interval or once if 
only an initial delay is provided. For the interval time, a monotonic clock is used.

The event callback should be relatively fast, and the scheduled event interval should be relatively high; otherwise, 
the framework event queue will be blocked, and the framework will not function properly.

## Scheduling an Event

To schedule an event in the Apache Celix framework, use the `celix_bundleContext_scheduleEvent` C function or 
`celix::BundleContext::scheduleEvent` C++ method. For C, an options struct is used to configure the scheduled event, 
and for C++, a builder pattern is used to configure the scheduled event.

### C Example

```c
#include <stdio.h>
#include <celix_bundle_activator.h>

typedef struct schedule_events_bundle_activator_data {
    celix_bundle_context_t* ctx;
    long scheduledEventId;
} schedule_events_bundle_activator_data_t;

void scheduleEventsBundle_oneShot(void* data) {
    schedule_events_bundle_activator_data_t* act = data;
    celix_bundleContext_log(act->ctx, CELIX_LOG_LEVEL_INFO, "One shot scheduled event fired");
}

void scheduleEventsBundle_process(void* data) {
    schedule_events_bundle_activator_data_t* act = data;
    celix_bundleContext_log(act->ctx, CELIX_LOG_LEVEL_INFO, "Recurring scheduled event fired");
}

static celix_status_t scheduleEventsBundle_start(schedule_events_bundle_activator_data_t *data, celix_bundle_context_t *ctx) {
    data->ctx = ctx;

    //schedule recurring event
    {
        celix_scheduled_event_options_t opts = CELIX_EMPTY_SCHEDULED_EVENT_OPTIONS;
        opts.name = "recurring scheduled event example";
        opts.initialDelayInSeconds = 0.1;
        opts.intervalInSeconds = 1.0;
        opts.callbackData = data;
        opts.callback = scheduleEventsBundle_process;
        data->scheduledEventId = celix_bundleContext_scheduleEvent(ctx, &opts);
    }

    //schedule one time event
    {
        celix_scheduled_event_options_t opts = CELIX_EMPTY_SCHEDULED_EVENT_OPTIONS;
        opts.name = "one shot scheduled event example";
        opts.initialDelayInSeconds = 0.1;
        opts.callbackData = data;
        opts.callback = scheduleEventsBundle_oneShot;
        celix_bundleContext_scheduleEvent(ctx, &opts);
    }

    return CELIX_SUCCESS;
}

static celix_status_t scheduleEventsBundle_stop(schedule_events_bundle_activator_data_t *data, celix_bundle_context_t *ctx) {
    celix_bundleContext_removeScheduledEvent(ctx, data->scheduledEventId);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(schedule_events_bundle_activator_data_t, scheduleEventsBundle_start, scheduleEventsBundle_stop)
```

### C++ Example

```cpp
#include <iostream>
#include "celix/BundleActivator.h"

class ScheduleEventsBundleActivator {
public:
    explicit ScheduleEventsBundleActivator(const std::shared_ptr<celix::BundleContext>& ctx) {
        //schedule recurring event
        event = ctx->scheduledEvent()
                .withInitialDelay(std::chrono::milliseconds{10})
                .withInterval(std::chrono::seconds{1})
                .withCallback([ctx] {
                    ctx->logInfo("Recurring scheduled event fired");
                })
                .build();

        //schedule one time event
        ctx->scheduledEvent()
                .withInitialDelay(std::chrono::milliseconds{10})
                .withCallback([ctx] {
                    ctx->logInfo("One shot scheduled event fired");
                })
                .build();
    }

    ~ScheduleEventsBundleActivator() noexcept {
        std::cout << "Goodbye world" << std::endl;
    }
private:
    celix::ScheduledEvent event{};
};

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(ScheduleEventsBundleActivator)
```

## Waking up a Scheduled Event

To process a scheduled event directly, you can use the `celix_bundleContext_wakeupScheduledEvent` C function or 
`celix::ScheduledEvent::wakup` C++ method. This will wake up the scheduled event and call its callback function.
