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

#include "celix/RejectedExecutionException.h"

namespace celix {

    /**
     * @brief An object that executes submitted Runnable tasks. This interface provides a way of decoupling task submission
     * from the mechanics of how each task will be run, including details of thread use, scheduling, etc.
     *
     * Note that supporting the priority argument is optional. If not priority values are ignored.
     */
    class IExecutor {
    public:
        virtual ~IExecutor() noexcept = default;

        /**
         * @brief Executes the given command at some time in the future. The command may execute in a new thread,
         * in a pooled thread, or in the calling thread, at the discretion of the Executor implementation.
         *
         * @note After a task has been executed, the `std::function<void()>` task object must go out of scope to
         * ensure that the potential capture objects also go out of scope.
         *
         * @param priority the priority of the task. It depends on the executor implementation whether this is supported.
         * @param command the "runnable" task
         * @throws celix::RejectedExecutionException if this task cannot be accepted for execution.
         */
        virtual void execute(int priority, std::function<void()> task) = 0;

        /**
         * @brief Executes the given command at some time in the future. The command may execute in a new thread,
         * in a pooled thread, or in the calling thread, at the discretion of the Executor implementation.
         *
         * Task will be executed int the default priority (0)
         *
         * @param command the "runnable" task
         * @throws celix::RejectedExecutionException if this task cannot be accepted for execution.
         */
        void execute(std::function<void()> task) {
            execute(0, std::move(task));
        }

        /**
         * @brief Wait until the executor has no pending task left.
         */
        virtual void wait() = 0;
    };
}