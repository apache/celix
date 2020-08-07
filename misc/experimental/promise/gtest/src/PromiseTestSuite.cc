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
#include <utility>

#include "celix/PromiseFactory.h"

class PromiseTestSuite : public ::testing::Test {
public:
    ~PromiseTestSuite() override = default;

    celix::PromiseFactory factory{ tbb::task_arena{5, 1} };
};

struct MovableInt {
    MovableInt(int _val) : val(_val) {}
    operator int() const { return val; }

    int val;
};

struct NonMovableInt {
    NonMovableInt(int _val) : val(_val) {}
    operator int() const { return val; }
    NonMovableInt(NonMovableInt&&) = delete;
    NonMovableInt(const NonMovableInt&) = default;
    NonMovableInt& operator=(NonMovableInt&&) = delete;
    NonMovableInt& operator=(const NonMovableInt&) = default;

    int val;
};

struct NonTrivialType {
    NonTrivialType(std::string _val) : val(std::move(_val)) {}
    operator std::string() const { return val; }
    NonTrivialType(NonTrivialType&&) = default;
    NonTrivialType(const NonTrivialType&) = default;
    NonTrivialType& operator=(NonTrivialType&&) = default;
    NonTrivialType& operator=(const NonTrivialType&) = default;


    NonTrivialType(const char *c) : val(c) {}

    NonTrivialType& operator=(const char *c) {
        val = c;
        return *this;
    }

    bool operator==(const char *c) const {
        return c == val;
    }

    std::string val;
};

bool operator==( const char *c, const NonTrivialType &ntt) {
    return c == ntt.val;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-result"

TEST_F(PromiseTestSuite, simplePromise) {
    auto deferred =  factory.deferred<NonTrivialType>();
    std::thread t{[&deferred] () {
        std::this_thread::sleep_for(std::chrono::milliseconds{50});
        deferred.resolve("test");
    }};
    auto promise = deferred.getPromise();
    EXPECT_EQ("test", promise.getValue()); //block until ready
    EXPECT_TRUE(promise.isDone()); //got value, so promise is done
    EXPECT_ANY_THROW(promise.getFailure()); //succeeded, so no exception available

    EXPECT_EQ("test", promise.getValue()); //note multiple call are valid;

    EXPECT_EQ("test", promise.moveOrGetValue()); //data is now moved
    EXPECT_THROW(promise.getValue(), celix::PromiseInvocationException); //data is already moved -> exception
    t.join();
}

TEST_F(PromiseTestSuite, failingPromise) {
    auto deferred =  factory.deferred<long>();
    auto cpy = deferred;
    std::thread t{[&deferred] () {
        deferred.fail(std::logic_error{"failing"});
    }};
    auto promise = deferred.getPromise();
    EXPECT_THROW(promise.getValue(), celix::PromiseInvocationException);
    EXPECT_TRUE(promise.getFailure() != nullptr);
    t.join();
}

TEST_F(PromiseTestSuite, failingPromiseWithExceptionPtr) {
    auto deferred =  factory.deferred<long>();
    std::thread t{[&deferred]{
        try {
            std::string{}.at(1); // this generates an std::out_of_range
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

TEST_F(PromiseTestSuite, onSuccessHandling) {
    auto deferred =  factory.deferred<long>();
    bool called = false;
    bool resolveCalled = false;
    auto p = deferred.getPromise()
        .onSuccess([&called](long value) {
            EXPECT_EQ(42, value);
            called = true;
        })
        .onResolve([&resolveCalled]() {
            resolveCalled = true;
        });
    deferred.resolve(42);
    p.wait();
    EXPECT_EQ(true, called);
    EXPECT_EQ(true, resolveCalled);
}

TEST_F(PromiseTestSuite, onFailureHandling) {
    auto deferred =  factory.deferred<long>();
    bool successCalled = false;
    bool failureCalled = false;
    bool resolveCalled = false;
    auto p = deferred.getPromise()
            .onSuccess([&](long /*value*/) {
                successCalled = true;
            })
            .onFailure([&](const std::exception &e) {
                failureCalled = true;
                std::cout << "got error: " << e.what() << std::endl;
            })
            .onResolve([&resolveCalled]() {
                resolveCalled = true;
            });
    try {
        std::string{}.at(1); // this generates an std::out_of_range
        //or use std::make_exception_ptr
    } catch (...) {
        deferred.fail(std::current_exception());
    }
    p.wait();
    EXPECT_EQ(false, successCalled);
    EXPECT_EQ(true, failureCalled);
    EXPECT_EQ(true, resolveCalled);
}

TEST_F(PromiseTestSuite, resolveSuccessWith) {
    auto deferred1 = factory.deferred<long>();
    auto deferred2 = factory.deferred<long>();

    bool called = false;
    deferred1.getPromise()
            .onSuccess([&called](long value) {
                EXPECT_EQ(42, value);
                called = true;
            });

    //currently deferred1 will be resolved in thread, and onSuccess is trigger on the promise of deferred2
    //now resolving deferred2 with the promise of deferred1
    deferred2.resolveWith(deferred1.getPromise());
    auto p = deferred2.getPromise();
    deferred1.resolve(42);
    p.wait();
    EXPECT_EQ(true, called);
}

TEST_F(PromiseTestSuite, resolveFailureWith) {
    auto deferred1 = factory.deferred<long>();
    auto deferred2 = factory.deferred<long>();
    bool failureCalled = false;
    bool successCalled = false;
    deferred2.getPromise()
            .onSuccess([&](long /*value*/) {
                successCalled = true;
            })
            .onFailure([&](const std::exception &e) {
                failureCalled = true;
                std::cout << "got error: " << e.what() << std::endl;
            });

    //currently deferred1 will be resolved in thread, and onSuccess is trigger on the promise of deferred2
    //now resolving deferred2 with the promise of deferred1
    deferred2.resolveWith(deferred1.getPromise());
    auto p = deferred2.getPromise();
    try {
        std::string().at(1); // this generates an std::out_of_range
        //or use std::make_exception_ptr
    } catch (...) {
        deferred1.fail(std::current_exception());
    }
    p.wait();
    EXPECT_EQ(false, successCalled);
    EXPECT_EQ(true, failureCalled);
}

TEST_F(PromiseTestSuite, resolveWithTimeout) {
    auto deferred1 = factory.deferred<long>();
    std::thread t{[&deferred1]{
        std::this_thread::sleep_for(std::chrono::milliseconds{50});
        try {
            deferred1.resolve(42);
        } catch(...) {
            //note resolve with throws an exception if promise is already resolved
        }
    }};

    bool firstSuccessCalled = false;
    bool secondSuccessCalled = false;
    bool secondFailedCalled = false;
    auto p = deferred1.getPromise()
            .onSuccess([&firstSuccessCalled](long value) {
                EXPECT_EQ(42, value);
                firstSuccessCalled = true;
            })
            .timeout(std::chrono::milliseconds{10})
            .onSuccess([&secondSuccessCalled](long value) {
                EXPECT_EQ(42, value);
                secondSuccessCalled = true;
            })
            .onFailure([&secondFailedCalled](const std::exception&) {
                secondFailedCalled = true;
            });
    t.join();
    p.wait();
    EXPECT_EQ(true, firstSuccessCalled);
    EXPECT_EQ(false, secondSuccessCalled);
    EXPECT_EQ(true, secondFailedCalled);

    firstSuccessCalled = false;
    secondSuccessCalled = false;
    secondFailedCalled = false;
    auto p2 = deferred1.getPromise()
            .onSuccess([&firstSuccessCalled](long value) {
                EXPECT_EQ(42, value);
                firstSuccessCalled = true;
            })
            .timeout(std::chrono::milliseconds{50})
            .onSuccess([&secondSuccessCalled](long value) {
                EXPECT_EQ(42, value);
                secondSuccessCalled = true;
            })
            .onFailure([&secondFailedCalled](const std::exception&) {
                secondFailedCalled = true;
            });
    p2.wait();
    EXPECT_EQ(true, firstSuccessCalled);
    EXPECT_EQ(true, secondSuccessCalled);
    EXPECT_EQ(false, secondFailedCalled);
}

TEST_F(PromiseTestSuite, resolveWithDelay) {
    auto deferred1 = factory.deferred<long>();
    bool successCalled = false;
    bool failedCalled = false;
    auto t1 = std::chrono::system_clock::now();
    std::chrono::system_clock::time_point t2;
    auto p = deferred1.getPromise()
            .delay(std::chrono::milliseconds{50})
            .onSuccess([&successCalled, &t2](long value) {
                EXPECT_EQ(42, value);
                successCalled = true;
                t2 = std::chrono::system_clock::now();
            })
            .onFailure([&failedCalled](const std::exception&) {
                failedCalled = true;
            });
    deferred1.resolve(42);
    p.wait();
    EXPECT_EQ(true, successCalled);
    EXPECT_EQ(false, failedCalled);
    auto durationInMs = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
    EXPECT_GE(durationInMs, std::chrono::milliseconds{10});
}


TEST_F(PromiseTestSuite, resolveWithRecover) {
    auto deferred1 = factory.deferred<long>();
    bool successCalled = false;
    deferred1.getPromise()
            .recover([]{ return 42; })
            .onSuccess([&successCalled](long v) {
                EXPECT_EQ(42, v);
                successCalled = true;
            });
    try {
        throw std::logic_error("failure");
    } catch (...) {
        deferred1.fail(std::current_exception());
    }
    EXPECT_EQ(true, successCalled);
}

TEST_F(PromiseTestSuite, chainAndMapResult) {
    auto deferred1 = factory.deferred<long>();
    std::thread t{[&deferred1]{
        deferred1.resolve(42);
    }};
    int half = deferred1.getPromise()
            .map<int>([](long v) {
                return (int)v/2;
            }).getValue();
    t.join();
    EXPECT_EQ(21, half);
}

TEST_F(PromiseTestSuite, chainWithThenAccept) {
    auto deferred1 = factory.deferred<long>();
    bool called = false;
    deferred1.getPromise()
            .thenAccept([&called](long v){
                EXPECT_EQ(42, v);
                called = true;
            });
    deferred1.resolve(42);
    EXPECT_TRUE(called);
}

TEST_F(PromiseTestSuite, promiseWithFallbackTo) {
    celix::Deferred<long> deferred1 = factory.deferred<long>();
    try {
        throw std::logic_error("failure");
    } catch (...) {
        deferred1.fail(std::current_exception());
    }

    auto deferred2 = factory.deferred<long>();
    deferred2.resolve(42);


    long val = deferred1.getPromise().fallbackTo(deferred2.getPromise()).getValue();
    EXPECT_EQ(42, val);
}

TEST_F(PromiseTestSuite, promiseWithPredicate) {
    auto deferred1 = factory.deferred<long>();
    std::thread t1{[&deferred1]{
        deferred1.resolve(42);
    }};

    EXPECT_ANY_THROW(deferred1.getPromise().filter([](long v) {return v == 0; }).getValue());
    EXPECT_EQ(42, deferred1.getPromise().filter([](long v) {return v == 42; }).getValue());
    t1.join();
}

TEST_F(PromiseTestSuite, outOfScopeUnresolvedPromises) {
    bool called = false;
    {
        auto deferred1 = factory.deferred<long>();
        deferred1.getPromise().onResolve([&]{
            called = true;
        });
        //promise and deferred out of scope
    }
    EXPECT_FALSE(called);
}

TEST_F(PromiseTestSuite, chainPromises) {
    auto success = [](celix::Promise<long> p) -> celix::Promise<long> {
        //TODO Promises::resolved(p.getValue() + p.getValue())
        celix::Deferred<long> result;
        result.resolve(p.getValue() + p.getValue());
        return result.getPromise();
    };
    celix::Deferred<long> initial;
    initial.resolve(42);
    long result = initial.getPromise().then<long>(success).then<long>(success).getValue();
    EXPECT_EQ(168, result);
}

TEST_F(PromiseTestSuite, chainFailedPromises) {
    bool called = false;
    auto success = [](celix::Promise<long> p) -> celix::Promise<long> {
        //nop
        return p;
    };
    auto failed = [&called](const celix::Promise<long>& /*p*/) -> void {
        called = true;
    };
    celix::Deferred<long> deferred;
    deferred.fail(std::logic_error{"fail"});
    deferred.getPromise().then<long>(success, failed).wait();
    EXPECT_TRUE(called);
}

TEST_F(PromiseTestSuite, failedResolvedWithPromiseFactory) {
    auto factory = celix::PromiseFactory{};
    auto p1 = factory.failed<long>(std::logic_error{"test"});
    EXPECT_TRUE(p1.isDone());
    EXPECT_NE(nullptr, p1.getFailure());

    auto p2 = factory.resolved(42);
    EXPECT_TRUE(p2.isDone());
    EXPECT_EQ(42, p2.getValue());
}

TEST_F(PromiseTestSuite, movableStruct) {
    auto deferred =  factory.deferred<MovableInt>();
    std::thread t{[&deferred] () {
        std::this_thread::sleep_for(std::chrono::milliseconds{50});
        deferred.resolve(42);
    }};
    auto promise = deferred.getPromise();
    EXPECT_EQ(42, promise.getValue());
    EXPECT_EQ(42, *promise);
    t.join();
}

TEST_F(PromiseTestSuite, movableStructTemporary) {
    auto deferred =  factory.deferred<MovableInt>();
    std::thread t{[&deferred] () {
        std::this_thread::sleep_for(std::chrono::milliseconds{50});
        deferred.resolve(42);
    }};
    EXPECT_EQ(42, deferred.getPromise().getValue());
    t.join();
}

TEST_F(PromiseTestSuite, nonMovableStruct) {
    auto deferred =  factory.deferred<NonMovableInt>();
    std::thread t{[&deferred] () {
        std::this_thread::sleep_for(std::chrono::milliseconds{50});
        deferred.resolve(42);
    }};
    auto promise = deferred.getPromise();
    EXPECT_EQ(42, promise.getValue());
    EXPECT_EQ(42, *promise);
    t.join();
}

TEST_F(PromiseTestSuite, nonMovableStructTemporary) {
    auto deferred =  factory.deferred<NonMovableInt>();
    std::thread t{[&deferred] () {
        std::this_thread::sleep_for(std::chrono::milliseconds{50});
        deferred.resolve(42);
    }};
    EXPECT_EQ(42, deferred.getPromise().getValue());
    t.join();
}

#pragma clang diagnostic pop
