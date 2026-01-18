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
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <gtest/gtest.h>
#include <uv.h>

TEST(LibuvSmokeTest, CanInitAndCloseLoopTest) {
    uv_loop_t loop;
    EXPECT_EQ(0, uv_loop_init(&loop));
    EXPECT_EQ(0, uv_loop_close(&loop));
}

namespace {
struct ThreadState {
    uv_mutex_t mutex;
    uv_cond_t cond;
    bool ready;
};

void threadMain(void* data) {
    auto* state = static_cast<ThreadState*>(data);
    uv_mutex_lock(&state->mutex);
    state->ready = true;
    uv_cond_signal(&state->cond);
    uv_mutex_unlock(&state->mutex);
}
} // namespace

TEST(LibuvSmokeTest, CanUseThreadMutexAndConditionTest) {
    ThreadState state{};
    state.ready = false;

    ASSERT_EQ(0, uv_mutex_init(&state.mutex));
    ASSERT_EQ(0, uv_cond_init(&state.cond));

    uv_thread_t thread;
    ASSERT_EQ(0, uv_thread_create(&thread, threadMain, &state));

    uv_mutex_lock(&state.mutex);
    while (!state.ready) {
        uv_cond_wait(&state.cond, &state.mutex);
    }
    uv_mutex_unlock(&state.mutex);

    EXPECT_EQ(0, uv_thread_join(&thread));
    uv_cond_destroy(&state.cond);
    uv_mutex_destroy(&state.mutex);
}
