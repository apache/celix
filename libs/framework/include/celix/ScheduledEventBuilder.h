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

#pragma once

#include <memory>
#include <functional>

#include "celix/Exceptions.h"
#include "celix/ScheduledEvent.h"

namespace celix {

/**
 * @brief A C++ builder for a ScheduledEvent object.
 */
class ScheduledEventBuilder final {
public:
    /**
     * @brief Construct a scheduled event builder with the given bundle context.
     *
     * @param[in] ctx The bundle context to use.
     */
    ScheduledEventBuilder(std::shared_ptr<celix_bundle_context_t> _cCtx) : ctx{std::move(_cCtx)}, options{} {}

    /**
     * @brief Set the initial delay of the scheduled event.
     *
     * @tparam Rep The representation type of the duration.
     * @tparam Period The period type of the duration.
     * @param[in] delay The delay duration.
     * @return A reference to this builder.
     */
    template<typename Rep, typename Period>
    ScheduledEventBuilder& withInitialDelay(std::chrono::duration<Rep, Period> delay) {
        options.initialDelayInSeconds = std::chrono::duration_cast<std::chrono::duration<double>>(delay).count();
        return *this;
    }

    /**
     * @brief Set the interval of the scheduled event.
     *
     * @tparam Rep The representation type of the duration.
     * @tparam Period The period type of the duration.
     * @param[in] interval The interval duration.
     * @return A reference to this builder.
     */
    template<typename Rep, typename Period>
    ScheduledEventBuilder& withInterval(std::chrono::duration<Rep, Period> interval) {
        options.intervalInSeconds = std::chrono::duration_cast<std::chrono::duration<double>>(interval).count();
        return *this;
    }

    /**
     * @brief Set the name of the scheduled event.
     *
     * @param[in] name The name of the scheduled event.
     * @return A reference to this builder.
     */
    ScheduledEventBuilder& withName(std::string n) {
        name = std::move(n);
        return *this;
    }

    /**
     * @brief Set the callback of the scheduled event.
     *
     * The callback is called when the scheduled event is triggered on the event thread.
     *
     * @param[in] cb The callback function.
     * @return A reference to this builder.
     */
    ScheduledEventBuilder& withCallback(std::function<void()> cb) {
        callback = std::move(cb);
        return *this;
    }

    /**
     * @brief Set the remove callback of the scheduled event.
     *
     * The remove callback is called when the scheduled event is removed from the scheduler and can be called
     * from any thread.
     *
     * @param[in] cb The callback function.
     * @return A reference to this builder.
     */
    ScheduledEventBuilder& withRemoveCallback(std::function<void()> cb) {
        removeCallback = std::move(cb);
        return *this;
    }

    /**
     * @brief Build the scheduled event with the given options.
     *
     * @return The scheduled event.
     */
    ScheduledEvent build() {
        if (!callback) {
            throw celix::Exception{"Cannot build scheduled event without callback"};
        }
        return ScheduledEvent{ctx, name, std::move(callback), std::move(removeCallback), options};
    }

private:
    std::shared_ptr<celix_bundle_context_t> ctx; /**< The bundle context for the scheduled event. */
    celix_scheduled_event_options_t options; /**< The options for the scheduled event. */
    std::string name{}; /**< The name of the scheduled event. */
    std::function<void()> callback{}; /**< The callback function for the scheduled event. */
    std::function<void()> removeCallback{}; /**< The callback function for the scheduled event. */
};

} // end namespace celix
