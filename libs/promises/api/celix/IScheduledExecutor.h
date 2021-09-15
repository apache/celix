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

#pragma once

#include <functional>
#include <future>

#include "celix/IExecutor.h"

namespace celix {

    /**
     * @brief a Cancelable Scheduled Future.
     */
    class IScheduledFuture {
    public:
        virtual ~IScheduledFuture() noexcept = default;

        [[nodiscard]] virtual bool isCancelled() const = 0;

        [[nodiscard]] virtual bool isDone() const = 0;

        virtual void cancel() = 0;
    };

    /**
     * @brief An ScheduledExecutor that can schedule commands to run after a given delay.
     * (TODO or execute tasks periodically)
     *
     * The schedule methods create tasks with various delays and return a task object that can be used to cancel or check execution.
     *
     * (TODO The scheduleAtFixedRate and scheduleWithFixedDelay methods create and execute tasks that run periodically until cancelled)
     * Zero and negative delays (but not periods) are also allowed in schedule methods, and are treated as requests for immediate execution.
     * All schedule methods accept relative delays and periods as arguments, not absolute times or dates.
     */
    class IScheduledExecutor {
    public:
        virtual ~IScheduledExecutor () noexcept = default;

        /**
         * @brief Creates and executes a ScheduledFuture that becomes enabled after the given delay.
         * @throws celix::RejectedExecutionException if this task cannot be accepted for execution.
         */
        template<typename Rep, typename Period>
        std::shared_ptr<celix::IScheduledFuture> schedule(int priority, std::chrono::duration<Rep, Period> delay, std::function<void()> task) {
            std::chrono::duration<double, std::milli> delayInMilli = delay;
            return scheduleInMilli(priority, delayInMilli, std::move(task));
        }

        /**
         * @brief Creates and executes a ScheduledFuture that becomes enabled after the given delay.
         * @throws celix::RejectedExecutionException if this task cannot be accepted for execution.
         */
        template<typename Rep, typename Period>
        std::shared_ptr<celix::IScheduledFuture> schedule(std::chrono::duration<Rep, Period> delay, std::function<void()> task) {
            std::chrono::duration<double, std::milli> delayInMilli = delay;
            return scheduleInMilli(0, delayInMilli, std::move(task));
        }

        /**
         * @brief Wait until the scheduled executor has no pending task left.
         */
        virtual void wait() = 0;
    private:
        /**
         * @brief Creates and executes a ScheduledFuture that becomes enabled after the given delay in milli.
         * @throws celix::RejectedExecutionException if this task cannot be accepted for execution.
         */
        virtual std::shared_ptr<celix::IScheduledFuture> scheduleInMilli(int priority, std::chrono::duration<double, std::milli> delay, std::function<void()> task) = 0;
    };
}