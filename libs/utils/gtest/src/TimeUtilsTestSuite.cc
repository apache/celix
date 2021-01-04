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
#include <thread>
#include <chrono>

#include "celix_utils.h"

class TimeUtilsTestSuite : public ::testing::Test {};

TEST_F(TimeUtilsTestSuite, GetTimeTest) {
    timespec t1;
    timespec t3;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    std::this_thread::sleep_for(std::chrono::seconds {1} );
    auto t2 = celix_gettime(CLOCK_MONOTONIC);
    std::this_thread::sleep_for(std::chrono::seconds {1} );
    clock_gettime(CLOCK_MONOTONIC, &t3);

    EXPECT_GT(t2.tv_sec, t1.tv_sec);
    EXPECT_GT(t3.tv_sec, t2.tv_sec);
}


TEST_F(TimeUtilsTestSuite, DiffTimeTest) {
    auto t1 = celix_gettime(CLOCK_MONOTONIC);
    std::this_thread::sleep_for(std::chrono::microseconds {10} );
    auto t2 = celix_gettime(CLOCK_MONOTONIC);
    auto diff = celix_difftime(&t1, &t2);
    EXPECT_GE(diff, 0.00001 /*10 us*/);
    EXPECT_LT(diff, 0.01 /*1 ms*/);
}

TEST_F(TimeUtilsTestSuite, ElapsedTimeTest) {
    auto t1 = celix_gettime(CLOCK_MONOTONIC);
    std::this_thread::sleep_for(std::chrono::microseconds {10} );
    auto diff = celix_elapsedtime(CLOCK_MONOTONIC, t1);
    EXPECT_GE(diff, 0.00001 /*10 us*/);
    EXPECT_LT(diff, 0.1 /*1/10 s, note do want to rely on accuracy of sleep_for*/);
}