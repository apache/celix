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
#include <unordered_map>

#include "celix/IScheduledExecutor.h"

namespace celix {

    class DefaultDelayedScheduledFuture : public celix::IScheduledFuture {
    public:
        explicit DefaultDelayedScheduledFuture(std::chrono::duration<double, std::milli> _delayInMs) : delayInMs{_delayInMs} {}

        ~DefaultDelayedScheduledFuture() noexcept override = default;

        bool isCancelled() const override {
            std::lock_guard lock{mutex};
            return cancelled;
        }

        bool isDone() const override {
            std::lock_guard lock{mutex};
            return done;
        }

        void setDone() {
            std::lock_guard lock{mutex};
            done = true;
            cond.notify_all();
        }

        void cancel() override {
            std::lock_guard lock{mutex};
            if (!done) {
                cancelled = true;
                done = true;
            }
            cond.notify_all();
        }

        std::chrono::duration<double, std::milli> getDelayInMs() const {
            return delayInMs;
        }

        void waitFor(std::chrono::duration<double, std::milli> time) {
            std::unique_lock lock{mutex};
            cond.wait_for(lock, time);
        }
    private:
        const std::chrono::duration<double, std::milli> delayInMs;
        mutable std::mutex mutex{}; //protects below
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
            auto scheduledFuture = std::make_shared<DefaultDelayedScheduledFuture>(delay);
            try {
                auto future = std::async(std::launch::async, [task = std::move(task), scheduledFuture] {
                    auto startTime = std::chrono::steady_clock::now();
                    auto t = std::chrono::steady_clock::now();
                    while (!scheduledFuture->isCancelled() && std::chrono::duration_cast<std::chrono::milliseconds>(t - startTime) < scheduledFuture->getDelayInMs()) {
                        auto timeLeft = scheduledFuture->getDelayInMs() - std::chrono::duration_cast<std::chrono::milliseconds>(t - startTime);
                        scheduledFuture->waitFor(timeLeft);
                        t = std::chrono::steady_clock::now();
                    }
                    if (!scheduledFuture->isCancelled()) {
                        task();
                    }
                    scheduledFuture->setDone();
                });
                future.wait_for(std::chrono::seconds{0}); //trigger async execution
                std::lock_guard<std::mutex> lck{mutex};
                futures[scheduledFuture] = std::move(future);
                removeCompletedFutures();
                return scheduledFuture;
            } catch (std::system_error& /*sysExp*/) {
                throw celix::RejectedExecutionException{};
            }
        }

        void wait() override {
            bool done = false;
            while (!done) {
                std::unique_lock lck{mutex};
                removeCompletedFutures();
                done = futures.empty();
                if (!done) {
                    cond.wait_for(lck, std::chrono::milliseconds{1});
                }
            }
        }
    private:
        void removeCompletedFutures() {
            //note should be called while mutex is lock.
            auto it = futures.begin();
            while (it != futures.end()) {
                if (it->first->isDone()) {
                    //remove scheduled future
                    it = futures.erase(it);
                    cond.notify_all();
                } else {
                    ++it;
                }
            }
        }

        std::mutex mutex{}; //protect futures.
        std::condition_variable cond{};
        std::unordered_map<std::shared_ptr<DefaultDelayedScheduledFuture>, std::future<void>> futures{};
    };
}