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
        .onSuccess([&called](const long& value) { //TODO also support value based callback
            EXPECT_EQ(42, value);
            called = true;
        });
    t.join();
    EXPECT_EQ(true, called);
}

TEST_F(PromiseTests, onFailreHandling) {
    auto deferred =  factory.deferred<long>();
    std::thread t{[&deferred]{
        try {
            std::string().at(1); // this generates an std::out_of_range
            //or use std::make_exception_ptr
        } catch (...) {
            deferred.fail(std::current_exception());
        }
    }};
    bool successCalled = false;
    bool failureCalled = false;
    deferred.getPromise()
            .onSuccess([&](const long& /*value*/) { //TODO also support value based callback
                successCalled = true;
            })
            .onFailure([&](const std::exception &e) {
                failureCalled = true;
                std::cout << "got error: " << e.what() << std::endl;
            });
    t.join();
    EXPECT_EQ(false, successCalled);
    EXPECT_EQ(true, failureCalled);
}

TEST_F(PromiseTests, resolveSuccessWith) {
    auto deferred1 = factory.deferred<long>();
    auto deferred2 = factory.deferred<long>();
    std::thread t{[&deferred1]{
        std::this_thread::sleep_for(std::chrono::milliseconds{50});
        deferred1.resolve(42);
    }};
    bool called = false;
    deferred2.getPromise()
            .onSuccess([&called](const long& value) { //TODO also support value based callback
                EXPECT_EQ(42, value);
                called = true;
            });

    //currently deferred1 will be resolved in thread, and onSuccess is trigger on the promise of deferred2
    //now resolving deferred2 with the promise of deferred1
    deferred2.resolveWith(deferred1.getPromise());
    t.join();
    EXPECT_EQ(true, called);
}

TEST_F(PromiseTests, resolveFailureWith) {
    auto deferred1 = factory.deferred<long>();
    auto deferred2 = factory.deferred<long>();
    std::thread t{[&deferred1]{
        try {
            std::string().at(1); // this generates an std::out_of_range
            //or use std::make_exception_ptr
        } catch (...) {
            deferred1.fail(std::current_exception());
        }
    }};
    bool failureCalled = false;
    bool successCalled = false;
    deferred2.getPromise()
            .onSuccess([&](const long& /*value*/) { //TODO also support value based callback
                successCalled = true;
            })
            .onFailure([&](const std::exception &e) {
                failureCalled = true;
                std::cout << "got error: " << e.what() << std::endl;
            });

    //currently deferred1 will be resolved in thread, and onSuccess is trigger on the promise of deferred2
    //now resolving deferred2 with the promise of deferred1
    deferred2.resolveWith(deferred1.getPromise());
    t.join();
    EXPECT_EQ(false, successCalled);
    EXPECT_EQ(true, failureCalled);
}

TEST_F(PromiseTests, resolveWithTimeout) {
    auto deferred1 = factory.deferred<long>();
    std::thread t{[&deferred1]{
        std::this_thread::sleep_for(std::chrono::milliseconds{50});
        try {
            deferred1.resolve(42);
        } catch(...) {
            //note resolve with throw an exception if promise is already resolved
        }
    }};

    bool succesCalled = false;
    bool failedCalled = false;
    deferred1.getPromise()
            .timeout(std::chrono::milliseconds{10})
            .onSuccess([&succesCalled](const long& value) { //TODO also support value based callback
                EXPECT_EQ(42, value);
                succesCalled = true;
            })
            .onFailure([&failedCalled](const std::exception&) {
               failedCalled = true;
            })
            //NOTE what happens if we add a time here?
            //.timeout(std::chrono::milliseconds{10})
            ;
    t.join();
    EXPECT_EQ(false, succesCalled);
    EXPECT_EQ(true, failedCalled);
}

TEST_F(PromiseTests, resolveWithDelay) {
    auto deferred1 = factory.deferred<long>();
    std::thread t{[&deferred1]{
        deferred1.resolve(42);
    }};

    bool succesCalled = false;
    bool failedCalled = false;
    auto t1 = std::chrono::system_clock::now();
    deferred1.getPromise()
            .delay(std::chrono::milliseconds{50})
            .onSuccess([&succesCalled](const long& value) { //TODO also support value based callback
                EXPECT_EQ(42, value);
                succesCalled = true;
            })
            .onFailure([&failedCalled](const std::exception&) {
                failedCalled = true;
            })
            .timeout(std::chrono::milliseconds{10});
    t.join();
    auto t2 = std::chrono::system_clock::now();
    auto durationInMs = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
    EXPECT_EQ(true, succesCalled);
    EXPECT_EQ(false, failedCalled);
    EXPECT_GE(durationInMs, std::chrono::milliseconds{10});
}

//TEST_F(PromiseTests, chainPromises) {
//    auto deferred1 = factory.deferred<long>();
//    std::thread t{[&deferred1]{
//        std::this_thread::sleep_for(std::chrono::milliseconds{50});
//        deferred1.resolve(42);
//    }};
//
//    celix::Promise<int> promise2 = deferred1.getPromise().then<int>([](celix::Promise<long> p) {
//        celix::Deferred<int> def;
//        def.resolve(p.getValue());
//        return def.getPromise();
//    });
//
//    EXPECT_EQ(promise2.getValue(), 42);
//    t.join();
//}