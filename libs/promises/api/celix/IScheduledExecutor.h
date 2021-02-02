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

    class enum ScheduledType {
        OneShot,
        FixedDelay,
        FixedRate
    };

    class IScheduledFuture {
    public:
        virtual ~IScheduledFuture() noexcept = default;

        virtual ScheduledType getType() const = 0;

        virtual bool isCancelled() const = 0;

        virtual void cancel() = 0;

        template<typename Rep, typename Period>
        std::chrono::duration<Rep, Period> getDelayOrPeriod() const {
            auto milli = getDelayOrPeriodInMilli();
            return std::chrono::duration<Rep, Period>{milli};
        }

    private:
        virtual std::chrono::duration<double, std::milli> getDelayOrPeriodInMilli() const = 0;
    };



    /**
     * An ExecutorService that can schedule commands to run after a given delay, or to execute periodically.
     *
     * The schedule methods create tasks with various delays and return a task object that can be used to cancel or check execution.
     * The scheduleAtFixedRate and scheduleWithFixedDelay methods create and execute tasks that run periodically until cancelled.
     * Commands submitted using the celix::IExecutor::execute method are scheduled with a requested delay of zero.
     * Zero and negative delays (but not periods) are also allowed in schedule methods, and are treated as requests for immediate execution.
     * All schedule methods accept relative delays and periods as arguments, not absolute times or dates.
     */
    class IScheduledExecutor : public celix::IExecutor {
    public:
        virtual ~IScheduledExecutor () noexcept = default;

        //TODO TBD schedule uses a executor, is a executor abstraction still needed or is a scheduled executor enough?

        /**
         * Creates and executes a ScheduledFuture that becomes enabled after the given delay.
         * @throws celix::RejectedExecutionException if this task cannot be accepted for execution.
         */
        template<typename Rep, typename Period>
        std::shared_ptr<celix::IScheduledFuture> schedule(std::function<void()> task, std::chrono::duration<Rep, Period> delay) {
            std::chrono::duration<double, std::milli> delayInMilli = delay;
            return scheduleInMilli(std::move(task), delayInMilli);
        }

        /**
         * Creates and executes a periodic action that becomes enabled first after the given initial delay,
         * and subsequently with the given period; that is executions will commence after initialDelay then
         * initialDelay+period, then initialDelay + 2 * period, and so on.
         * If any execution of the task encounters an exception, subsequent executions are suppressed.
         * Otherwise, the task will only terminate via cancellation or termination of the executor.
         * If any execution of this task takes longer than its period,
         * then subsequent executions may start late, but will not concurrently execute.
         *
         * @param task          the task to execute
         * @param period        the period between successive executions
         * @param initialDelay  the time to delay first execution
         * @return a ScheduledFuture representing pending completion of the task.
         */
        template<typename Rep1, typename Period1, typename Rep2 = double, typename Period2 = std::milli>
        std::shared_ptr<celix::IScheduledFuture> scheduleAtFixedRate(std::function<void()> task, std::chrono::duration<std::chrono::duration<Rep1, Period1> period, std::chrono::duration<Rep2, Period2> initialDelay = 0) {
            std::chrono::duration<double, std::milli> periodInMilli = period;
            std::chrono::duration<double, std::milli> initialDelayInMilli = initialDelay;
            return scheduleAtFixedRateInMilli(std::move(task), periodInMilli, initialDelay);
        }

        /**
         * Creates and executes a periodic action that becomes enabled first after the given initial delay,
         * and subsequently with the given delay between the termination of one execution and the commencement of
         * the next. If any execution of the task encounters an exception, subsequent executions are suppressed.
         * Otherwise, the task will only terminate via cancellation or termination of the executor.
         *
         * @param task          the task to execute
         * @param delay         the delay between the termination of one execution and the commencement of the next
         * @param initialDelay  the time to delay first execution
         * @return a ScheduledFuture representing pending completion of the task.
         */
        template<typename Rep1, typename Period1, typename Rep2 = double, typename Period2 = std::milli>
        std::shared_ptr<celix::IScheduledFuture> scheduleWithFixedDelay(std::function<void()> task, std::chrono::duration<Rep, Period> delay, std::chrono::duration<Rep2, Period2> initialDelay = 0) {
            std::chrono::duration<double, std::milli> delayInMilli = delay;
            std::chrono::duration<double, std::milli> initialDelayInMilli = initialDelay;
            return scheduleWithFixedDelayInMilli(std::move(task), delayInMilli, initialDelay);
        }
    private:
        virtual std::shared_ptr<celix::IScheduledFuture> scheduleInMilli(std::function<void()> task, std::chrono::duration<double, std::milli> delay) = 0;

        virtual std::shared_ptr<celix::IScheduledFuture> scheduleAtFixedRateInMilli(std::function<void()> task, std::chrono::duration<double, std::milli> period, std::chrono::duration<double, std::milli> initialDelay) = 0;

        virtual std::shared_ptr<celix::IScheduledFuture> scheduleWithFixedDelayInMilli(std::function<void()> task, std::chrono::duration<double, std::milli> delay, std::chrono::duration<double, std::milli> initialDelay) = 0;
    };
}