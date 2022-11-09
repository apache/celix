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

#include <future>
#include <random>

#include "celix/IExecutor.h"

namespace celix {

    /**
     * Simple default executor which uses std::async to run tasks.
     * Does not support priority argument.
     */
    class DefaultExecutor : public celix::IExecutor {
    public:
        explicit DefaultExecutor(std::launch _policy = std::launch::async | std::launch::deferred) : policy{_policy} {}

        void execute(int /*priority*/, std::function<void()> task) override {
            std::lock_guard lck{mutex};
            futures.emplace_back(std::async(policy, [task = std::move(task)]() mutable {
                task();
                task = nullptr; //to ensure captures of task go out of scope
            }));
            removeCompletedFutures();
        }

        void wait() override {
            bool done = false;
            while (!done) {
                std::lock_guard lck{mutex};
                removeCompletedFutures();
                done = futures.empty();
            }
        }
    private:
        void removeCompletedFutures() {
            //note should be called while mutex is lock.
            auto it = futures.begin();
            while (it != futures.end()) {
                auto status = it->wait_for(std::chrono::seconds{0});
                if (status == std::future_status::ready) {
                    //remove future
                    it = futures.erase(it);
                } else {
                    ++it;
                }
            }
        }

        const std::launch policy;
        std::mutex mutex{}; //protect futures.
        std::vector<std::future<void>> futures{};
    };
}