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

#include <memory>
#include <future>
#include <functional>

#include "celix/IScheduledExecutor.h"

namespace celix {

    class DefaultDelayedScheduledFuture : public celix::IScheduledFuture {
    public:
        static std::shared_ptr<DefaultDelayedScheduledFuture> create(std::function<void()> task, std::chrono::duration<double, std::milli> delayInMs) {
            auto delayedFuture = std::shared_ptr<DefaultDelayedScheduledFuture>{new DefaultDelayedScheduledFuture{std::move(task), delayInMs}};
            delayedFuture->start();
            return delayedFuture;
        }

        virtual ~DefaultDelayedScheduledFuture() noexcept override = default;

        bool isCancelled() const override {
            std::lock_guard<std::mutex> lock{mutex};
            return cancelled;
        }

        bool isDone() const override {
            std::lock_guard<std::mutex> lock{mutex};
            return done;
        }

        void cancel() override {
            std::lock_guard<std::mutex> lock{mutex};
            cancelled = true;
        }

        void start() {
            std::lock_guard<std::mutex> lock{mutex};
            //TODO ensure that async is possible or throw RejectedExecutionException
            future = std::async(std::launch::async, [this] {
                auto t = std::chrono::steady_clock::now();
                std::unique_lock<std::mutex> lock{mutex};
                while (!cancelled && std::chrono::duration_cast<std::chrono::milliseconds>(t - startTime) < delayInMs) {
                    auto timeLeft = delayInMs - std::chrono::duration_cast<std::chrono::milliseconds>(t - startTime);
                    cond.wait_for(lock, timeLeft);
                    t = std::chrono::steady_clock::now();
                }
                if (!cancelled) {
                    task();
                }
                done = true;
            });

            //trigger async future for execution
            future.wait_for(std::chrono::seconds{0});
        }

        const std::future<void>& getFuture() const {
            std::lock_guard<std::mutex> lock{mutex};
            return future;
        }
    private:
        explicit DefaultDelayedScheduledFuture(std::function<void()> _task, std::chrono::duration<double, std::milli> _delay) :
                startTime{std::chrono::steady_clock::now()},
                task{std::move(_task)},
                delayInMs{_delay} {}

        std::chrono::duration<double, std::milli> getDelayOrPeriodInMilli() const override {
            return delayInMs;
        }

        const std::chrono::steady_clock::time_point startTime;
        const std::function<void()> task;
        const std::chrono::duration<double, std::milli> delayInMs;
        mutable std::mutex mutex{}; //protects below
        std::future<void> future{};
        std::condition_variable cond{};
        bool cancelled{false};
        bool done{false};
    };

    /**
     * @brief Simple default scheduled executor which uses std::async to run delayed tasks and steady_clock to measure delay.
     *
     * Does not support priority argument.
     */
    class DefaultScheduledExecutor : public celix::IScheduledExecutor {
    public:
        std::shared_ptr<celix::IScheduledFuture> scheduleInMilli(int /*priority*/, std::chrono::duration<double, std::milli> delay, std::function<void()> task) override {
            std::lock_guard<std::mutex> lck{mutex};
            auto scheduledFuture = DefaultDelayedScheduledFuture::create(std::move(task), delay);
            futures.emplace_back(scheduledFuture);
            removeCompletedFutures();
            return scheduledFuture;
        }

        void wait() override {
            bool done = false;
            while (!done) {
                std::lock_guard<std::mutex> lck{mutex};
                removeCompletedFutures();
                done = futures.empty();
            }
        }
    private:
        void removeCompletedFutures() {
            //note should be called while mutex is lock.
            auto it = futures.begin();
            while (it != futures.end()) {
                if ((*it)->isDone()) {
                    //remove scheduled future
                    it = futures.erase(it);
                } else {
                    ++it;
                }
            }
        }

        std::mutex mutex{}; //protect futures.
        std::vector<std::shared_ptr<DefaultDelayedScheduledFuture>> futures{};
    };
}