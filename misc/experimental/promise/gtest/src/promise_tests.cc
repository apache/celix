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

#include <gtest/gtest.h>

#include <future>

#include "celix/PromiseFactory.h"

class PromiseTests : public ::testing::Test {
public:
    ~PromiseTests() override = default;

    celix::PromiseFactory factory{};
};



TEST_F(PromiseTests, simplePromise) {
    auto deferred =  factory.deferred<long>();
    std::thread t{[&deferred]{
        std::this_thread::sleep_for(std::chrono::milliseconds{50});
        deferred.resolve(42);
    }};
    auto promise = deferred.getPromise();
    EXPECT_EQ(42, promise.getValue()); //block until ready
    EXPECT_TRUE(promise.isDone()); //got value, so promise is done
    EXPECT_ANY_THROW(promise.getFailure()); //succeeded, so no exception available

    EXPECT_EQ(42, promise.getValue()); //note multiple call are valid;

    EXPECT_EQ(42, promise.moveValue()); //data is now moved
    EXPECT_THROW(promise.getValue(), celix::PromiseInvocationException); //data is already moved -> exception
    t.join();
}

TEST_F(PromiseTests, failingPromise) {
    auto deferred =  factory.deferred<long>();
    std::thread t{[&deferred]{
        try {
            std::string().at(1); // this generates an std::out_of_range
            //or use std::make_exception_ptr
        } catch (...) {
            deferred.fail(std::current_exception());
        }
    }};
    auto promise = deferred.getPromise();
    EXPECT_THROW(promise.getValue(), celix::PromiseInvocationException);
    EXPECT_TRUE(promise.getFailure() != nullptr);
    t.join();
}

TEST_F(PromiseTests, onSuccessHandling) {
    auto deferred =  factory.deferred<long>();
    std::thread t{[&deferred]{
        std::this_thread::sleep_for(std::chrono::milliseconds{50});
        deferred.resolve(42);
    }};
    bool called = false;
    deferred.getPromise()
        .onSuccess([&called](const long& /*value*/) { //TODO also support value based callback
            //EXPECT_EQ(42, value);
            called = true;
        });
    t.join();
    EXPECT_EQ(true, called);
}
