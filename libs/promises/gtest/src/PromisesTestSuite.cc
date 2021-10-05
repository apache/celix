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
#include "celix/DefaultExecutor.h"
#include "celix/DefaultScheduledExecutor.h"

/**
 * A special executor which introduces some sleeps to ensure some entropy during testing
 */
class ExecutorWithRandomPrePostSleep : public celix::DefaultExecutor {
public:
    ExecutorWithRandomPrePostSleep() : celix::DefaultExecutor{std::launch::async} {}

    void execute( int priority, std::function<void()> task) override {
        int roll1 = distribution(generator); //1-100
        int roll2 = distribution(generator); //1-100
        celix::DefaultExecutor::execute(priority, [roll1, roll2, task = std::move(task)] {
            std::this_thread::sleep_for(std::chrono::milliseconds{roll1});
            task();
            std::this_thread::sleep_for(std::chrono::milliseconds{roll2});
        });
    }

private:
    std::default_random_engine generator{};
    std::uniform_int_distribution<int> distribution{1,100};
};

class PromiseTestSuite : public ::testing::Test {
public:
    ~PromiseTestSuite() noexcept override = default;

    std::shared_ptr<celix::IExecutor> executor = std::make_shared<ExecutorWithRandomPrePostSleep>();
    std::shared_ptr<celix::IScheduledExecutor> scheduledExecutor = std::make_shared<celix::DefaultScheduledExecutor>();
    std::shared_ptr<celix::PromiseFactory> factory = std::make_shared<celix::PromiseFactory>(executor, scheduledExecutor);
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

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-result"
#endif 

TEST_F(PromiseTestSuite, simplePromise) {
    auto deferred =  factory->deferred<NonTrivialType>();
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
    auto deferred =  factory->deferred<long>();
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
    auto deferred =  factory->deferred<long>();
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
    auto deferred =  factory->deferred<long>();
    std::atomic<bool> called = false;
    std::atomic<bool> resolveCalled = false;
    auto p = deferred.getPromise()
        .onSuccess([&](long value) {
            EXPECT_EQ(42, value);
            called = true;
        })
        .onResolve([&]() {
            resolveCalled = true;
        });
    deferred.resolve(42);

    factory->wait();
    EXPECT_EQ(true, called);
    EXPECT_EQ(true, resolveCalled);
}

static void test_throwing_function() {
    throw celix::PromiseInvocationException{"PromiseInvocationException test"};
}

TEST_F(PromiseTestSuite, onFailureHandling) {
    auto deferred =  factory->deferred<long>();
    std::atomic<bool> successCalled = false;
    std::atomic<bool> failureCalled = false;
    std::atomic<bool> resolveCalled = false;
    auto p = deferred.getPromise()
            .onSuccess([&](long /*value*/) {
                successCalled = true;
            })
            .onFailure([&](const std::exception &e) {
                failureCalled = true;
                auto what = e.what();
                ASSERT_STREQ(what, "PromiseInvocationException test");
            })
            .onResolve([&]() {
                resolveCalled = true;
            });
    try {
        test_throwing_function(); // this generates a celix::PromiseInvocationException
    } catch (...) {
        deferred.fail(std::current_exception());
    }

    factory->wait();
    EXPECT_EQ(false, successCalled);
    EXPECT_EQ(true, failureCalled);
    EXPECT_EQ(true, resolveCalled);
}

TEST_F(PromiseTestSuite, onFailureHandlingLogicError) {
    auto deferred =  factory->deferred<long>();
    std::atomic<bool> successCalled = false;
    std::atomic<bool> failureCalled = false;
    std::atomic<bool> resolveCalled = false;
    auto p = deferred.getPromise()
            .onSuccess([&](long /*value*/) {
                successCalled = true;
            })
            .onFailure([&](const std::exception &e) {
                failureCalled = true;
                ASSERT_TRUE(0 == strcmp("Some Exception",  e.what())) << std::string(e.what());
            })
            .onResolve([&]() {
                resolveCalled = true;
            });

    deferred.fail(std::logic_error("Some Exception"));

    factory->wait();
    EXPECT_EQ(false, successCalled);
    EXPECT_EQ(true, failureCalled);
    EXPECT_EQ(true, resolveCalled);
}

TEST_F(PromiseTestSuite, onFailureHandlingPromiseIllegalStateException) {
    auto deferred =  factory->deferred<long>();
    std::atomic<bool> successCalled = false;
    std::atomic<bool> failureCalled = false;
    std::atomic<bool> resolveCalled = false;
    auto p = deferred.getPromise()
            .onSuccess([&](long /*value*/) {
                successCalled = true;
            })
            .onFailure([&](const std::exception &e) {
                failureCalled = true;
                ASSERT_TRUE(0 == strcmp("Illegal state",  e.what())) << std::string(e.what());
            })
            .onResolve([&]() {
                resolveCalled = true;
            });

    deferred.fail(celix::PromiseIllegalStateException());

    factory->wait();
    EXPECT_EQ(false, successCalled);
    EXPECT_EQ(true, failureCalled);
    EXPECT_EQ(true, resolveCalled);
}

TEST_F(PromiseTestSuite, onFailureHandlingPromiseInvocationException) {
    auto deferred =  factory->deferred<long>();
    std::atomic<bool> successCalled = false;
    std::atomic<bool> failureCalled = false;
    std::atomic<bool> resolveCalled = false;
    auto p = deferred.getPromise()
            .onSuccess([&](long /*value*/) {
                successCalled = true;
            })
            .onFailure([&](const std::exception &e) {
                failureCalled = true;
                ASSERT_TRUE(0 == strcmp("MyExceptionText",  e.what())) << std::string(e.what());
            })
            .onResolve([&]() {
                resolveCalled = true;
            });

    deferred.fail(celix::PromiseInvocationException("MyExceptionText"));

    factory->wait();
    EXPECT_EQ(false, successCalled);
    EXPECT_EQ(true, failureCalled);
    EXPECT_EQ(true, resolveCalled);
}

TEST_F(PromiseTestSuite, onFailureHandlingPromiseTimeoutException) {
    auto deferred =  factory->deferred<long>();
    std::atomic<bool> successCalled = false;
    std::atomic<bool> failureCalled = false;
    std::atomic<bool> resolveCalled = false;
    auto p = deferred.getPromise()
            .onSuccess([&](long /*value*/) {
                successCalled = true;
            })
            .onFailure([&](const std::exception &e) {
                failureCalled = true;
                ASSERT_TRUE(0 == strcmp("Timeout",  e.what())) << std::string(e.what());
            })
            .onResolve([&]() {
                resolveCalled = true;
            });

    deferred.fail(celix::PromiseTimeoutException());

    factory->wait();
    EXPECT_EQ(false, successCalled);
    EXPECT_EQ(true, failureCalled);
    EXPECT_EQ(true, resolveCalled);
}

TEST_F(PromiseTestSuite, onFailureHandlingRejectedExecutionException) {
    auto deferred =  factory->deferred<long>();
    std::atomic<bool> successCalled = false;
    std::atomic<bool> failureCalled = false;
    std::atomic<bool> resolveCalled = false;
    auto p = deferred.getPromise()
            .onSuccess([&](long /*value*/) {
                successCalled = true;
            })
            .onFailure([&](const std::exception &e) {
                failureCalled = true;
                ASSERT_TRUE(0 == strcmp("Cannot accept task for execution",  e.what())) << std::string(e.what());
            })
            .onResolve([&]() {
                resolveCalled = true;
            });

    deferred.fail(celix::RejectedExecutionException());

    factory->wait();
    EXPECT_EQ(false, successCalled);
    EXPECT_EQ(true, failureCalled);
    EXPECT_EQ(true, resolveCalled);
}

TEST_F(PromiseTestSuite, resolveSuccessWith) {
    auto deferred1 = factory->deferred<long>();
    std::atomic<bool> called = false;
    deferred1.getPromise()
            .onSuccess([&](long value) {
                EXPECT_EQ(42, value);
                called = true;
            });

    auto deferred2 = factory->deferred<long>();
    deferred1.resolveWith(deferred2.getPromise());
    deferred2.resolve(42);

    factory->wait();
    EXPECT_EQ(true, called);
}

TEST_F(PromiseTestSuite, resolveFailureWith) {
    auto deferred1 = factory->deferred<long>();
    auto deferred2 = factory->deferred<long>();

   std::atomic<bool> failureCalled = false;

    deferred2.getPromise()
            .onFailure([&](const std::exception &e) {
                std::cout << "got error: " << e.what() << std::endl;
                failureCalled = true;
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

    factory->wait();
    EXPECT_EQ(true, failureCalled);
}

TEST_F(PromiseTestSuite, returnPromiseOnSuccessfulResolveWith) {
    auto deferred1 = factory->deferred<long>();
    auto deferred2 = factory->deferred<long>();
    celix::Promise<void> promise = deferred1.resolveWith(deferred2.getPromise());
    EXPECT_FALSE(promise.isDone());
    deferred2.resolve(42);
    factory->wait();
    EXPECT_TRUE(promise.isDone());
    EXPECT_TRUE(promise.isSuccessfullyResolved());
}

TEST_F(PromiseTestSuite, returnPromiseOnInvalidResolveWith) {
    auto deferred1 = factory->deferred<long>();
    auto deferred2 = factory->deferred<long>();
    celix::Promise<void> promise = deferred1.resolveWith(deferred2.getPromise());
    EXPECT_FALSE(promise.isDone());
    deferred1.resolve(42); //Rule is deferred1 is resolved, before the resolveWith the returned promise should fail
    deferred2.resolve(42);
    factory->wait();
    EXPECT_TRUE(promise.isDone());
    EXPECT_FALSE(promise.isSuccessfullyResolved());
    EXPECT_THROW(std::rethrow_exception(promise.getFailure()), celix::PromiseIllegalStateException);
}


//gh-333 fix timeout test on macos
#ifndef __APPLE__
TEST_F(PromiseTestSuite, resolveWithTimeout) {
    auto promise1 = factory->deferredTask<long>([](auto d) {
        std::this_thread::sleep_for(std::chrono::milliseconds{50});
        try {
            d.resolve(42);
        } catch(...) {
            //note resolve with throws an exception if promise is already resolved
        }
    });

    std::atomic<bool> firstSuccessCalled = false;
    std::atomic<bool> secondSuccessCalled = false;
    std::atomic<bool> secondFailedCalled = false;
    promise1
            .onSuccess([&](long value) {
                EXPECT_EQ(42, value);
                firstSuccessCalled = true;
            })
            .timeout(std::chrono::milliseconds{5})
            .onSuccess([&](long value) {
                EXPECT_EQ(42, value);
                secondSuccessCalled = true;
            })
            .onFailure([&](const std::exception&) {
                secondFailedCalled = true;
            });

    promise1.wait();
    factory->wait();
    EXPECT_EQ(true, firstSuccessCalled);
    EXPECT_EQ(false, secondSuccessCalled);
    EXPECT_EQ(true, secondFailedCalled);

    firstSuccessCalled = false;
    secondSuccessCalled = false;
    secondFailedCalled = false;
    auto promise2 = factory->deferredTask<long>([](auto d) {
        d.resolve(42);
    });

    promise2
            .onSuccess([&](long value) {
                EXPECT_EQ(42, value);
                firstSuccessCalled = true;
            })
            .timeout(std::chrono::milliseconds{200}) /*NOTE: more than the possible delay introduced by the executor*/
            .onSuccess([&](long value) {
                EXPECT_EQ(42, value);
                secondSuccessCalled = true;
            })
            .onFailure([&](const std::exception&) {
                secondFailedCalled = true;
            });
    promise2.wait();
    factory->wait();
    EXPECT_EQ(true, firstSuccessCalled);
    EXPECT_EQ(true, secondSuccessCalled);
    EXPECT_EQ(false, secondFailedCalled);
}
#endif

TEST_F(PromiseTestSuite, resolveWithSetTimeout) {
    auto promise = factory->deferred<int>().getPromise().setTimeout(std::chrono::milliseconds{5});
    factory->wait();
    EXPECT_TRUE(promise.isDone());
    EXPECT_FALSE(promise.isSuccessfullyResolved());
}

TEST_F(PromiseTestSuite, resolveWithDelay) {
    auto deferred1 = factory->deferred<long>();
    std::atomic<bool> successCalled = false;
    auto t1 = std::chrono::steady_clock::now();
    std::atomic<std::chrono::steady_clock::time_point> t2{t1};
    auto p = deferred1.getPromise()
            .delay(std::chrono::milliseconds{50})
            .onSuccess([&](long value) {
                EXPECT_EQ(42, value);
                t2 = std::chrono::steady_clock::now();
                successCalled = true;
            });
    deferred1.resolve(42);

    p.wait();
    factory->wait();
    EXPECT_EQ(true, successCalled.load());
    auto durationInMs = std::chrono::duration_cast<std::chrono::milliseconds>(t2.load() - t1);
    EXPECT_GE(durationInMs, std::chrono::milliseconds{50});
}

TEST_F(PromiseTestSuite, resolveWithRecover) {
    auto deferred1 = factory->deferred<long>();
    std::atomic<bool> successCalled = false;
    deferred1.getPromise()
            .recover([]{ return 42; })
            .onSuccess([&](long v) {
                EXPECT_EQ(42, v);
                successCalled = true;
            });
    try {
        throw std::logic_error("failure");
    } catch (...) {
        deferred1.fail(std::current_exception());
    }
    factory->wait();
    EXPECT_EQ(true, successCalled);
}

TEST_F(PromiseTestSuite, chainAndMapResult) {
    auto deferred1 = factory->deferred<long>();
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
    auto deferred1 = factory->deferred<long>();
    std::atomic<bool> called = false;
    deferred1.getPromise()
            .thenAccept([&](long v){
                EXPECT_EQ(42, v);
                called = true;
            });
    deferred1.resolve(42);
    factory->wait();
    EXPECT_TRUE(called);
}

TEST_F(PromiseTestSuite, promiseWithFallbackTo) {
    celix::Deferred<long> deferred1 = factory->deferred<long>();
    try {
        throw std::logic_error("failure");
    } catch (...) {
        deferred1.fail(std::current_exception());
    }

    auto deferred2 = factory->deferred<long>();
    deferred2.resolve(42);


    long val = deferred1.getPromise().fallbackTo(deferred2.getPromise()).getValue();
    EXPECT_EQ(42, val);
}

TEST_F(PromiseTestSuite, promiseWithPredicate) {
    auto deferred1 = factory->deferred<long>();
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
        auto deferred1 = factory->deferred<long>();
        deferred1.getPromise().onResolve([&]{
            called = true;
        });
        //promise and deferred out of scope
    }
    EXPECT_FALSE(called);
}

TEST_F(PromiseTestSuite, chainPromises) {
    auto success = [&](celix::Promise<long> p) -> celix::Promise<long> {
        auto result = factory->deferred<long>();
        result.resolve(p.getValue() + p.getValue());
        return result.getPromise();
    };
    auto initial = factory->deferred<long>();
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
    auto deferred = factory->deferred<long>();
    deferred.fail(std::logic_error{"fail"});
    deferred.getPromise().then<long>(success, failed);
    factory->wait();
    EXPECT_TRUE(called);
}

TEST_F(PromiseTestSuite, failedResolvedWithPromiseFactory) {
    auto p1 = factory->failed<long>(std::logic_error{"test"});
    EXPECT_TRUE(p1.isDone());
    EXPECT_NE(nullptr, p1.getFailure());

    auto p2 = factory->resolved(42);
    EXPECT_TRUE(p2.isDone());
    EXPECT_EQ(42, p2.getValue());
}

TEST_F(PromiseTestSuite, movableStruct) {
    auto deferred =  factory->deferred<MovableInt>();
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
    auto deferred =  factory->deferred<MovableInt>();
    std::thread t{[&deferred] () {
        std::this_thread::sleep_for(std::chrono::milliseconds{50});
        deferred.resolve(42);
    }};
    EXPECT_EQ(42, deferred.getPromise().getValue());
    t.join();
}

TEST_F(PromiseTestSuite, nonMovableStruct) {
    auto deferred =  factory->deferred<NonMovableInt>();
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
    auto deferred =  factory->deferred<NonMovableInt>();
    std::thread t{[&deferred] () {
        std::this_thread::sleep_for(std::chrono::milliseconds{50});
        deferred.resolve(42);
    }};
    EXPECT_EQ(42, deferred.getPromise().getValue());
    t.join();
}

TEST_F(PromiseTestSuite, deferredTaskCall) {
    auto promise = factory->deferredTask<long>([](auto deferred) {
        deferred.resolve(42);
    });
    EXPECT_EQ(42, promise.getValue());
}

TEST_F(PromiseTestSuite, getExecutorFromFactory) {
    auto exec = factory->getExecutor();
    EXPECT_EQ(executor.get(), exec.get());
}

TEST_F(PromiseTestSuite, testRobustFailAndResolve) {
    std::atomic<int> failCount{};
    std::atomic<int> successCount{};
    auto failCb = [&failCount](const std::exception& /*e*/) {
            failCount++;
    };
    auto successCb = [&successCount](int val) {
        EXPECT_EQ(42, val);
        successCount++;
    };

    auto def = factory->deferred<int>();
    def.getPromise().onFailure(failCb);

    //Rule a second fail should not lead to an exception, to ensure a more robust usage.
    //But also should only lead to a single resolve chain.
    def.fail(std::logic_error{"error1"});
    def.fail(std::logic_error{"error2"});
    factory->wait();
    EXPECT_EQ(failCount.load(), 1);

    def = factory->deferred<int>();
    def.getPromise().onSuccess(successCb);
    //Rule a second resolve should not lead to an exception, to ensure a more robust usage.
    //But also should only lead to a single resolve chain.
    def.resolve(42);
    def.resolve(43);
    factory->wait();
    EXPECT_EQ(successCount.load(), 1);


    failCount = 0;
    successCount = 0;
    def = factory->deferred<int>();
    def.getPromise().onSuccess(successCb).onFailure(failCb);
    //Rule a resolve after fail should not lead to an exception, to ensure a more robust usage.
    //But also should only lead to a single resolve chain.
    def.fail(std::logic_error("error3"));
    def.resolve(43);
    factory->wait();
    EXPECT_EQ(failCount.load(), 1);
    EXPECT_EQ(successCount.load(), 0);

    failCount = 0;
    successCount = 0;
    def = factory->deferred<int>();
    def.getPromise().onSuccess(successCb).onFailure(failCb);
    //Rule a fail after resolve should not lead to an exception, to ensure a more robust usage.
    //But also should only lead to a single resolve chain.
    def.resolve(42);
    def.fail(std::logic_error("error3"));
    factory->wait();
    EXPECT_EQ(failCount.load(), 0);
    EXPECT_EQ(successCount.load(), 1);
}

TEST_F(PromiseTestSuite, testTryFailAndResolve) {
    std::atomic<int> failCount{};
    std::atomic<int> successCount{};
    auto failCb = [&failCount](const std::exception& /*e*/) {
        failCount++;
    };
    auto successCb = [&successCount](int val) {
        EXPECT_EQ(42, val);
        successCount++;
    };
    const int val = 42;

    //first resolve with &&, then try rest
    auto def = factory->deferred<int>();
    def.getPromise().onSuccess(successCb).onFailure(failCb);
    EXPECT_TRUE(def.tryResolve(42));
    EXPECT_FALSE(def.tryResolve(43));
    EXPECT_FALSE(def.tryFail(std::logic_error{"error"}));
    EXPECT_FALSE(def.tryFail(std::make_exception_ptr(std::logic_error{"error"})));
    factory->wait();
    EXPECT_EQ(failCount.load(), 0);
    EXPECT_EQ(successCount.load(), 1);

    //first resolve with const int&, then try rest
    failCount = 0;
    successCount = 0;
    def = factory->deferred<int>();
    def.getPromise().onSuccess(successCb).onFailure(failCb);
    EXPECT_TRUE(def.tryResolve(val));
    EXPECT_FALSE(def.tryResolve(43));
    EXPECT_FALSE(def.tryResolve(val));
    EXPECT_FALSE(def.tryFail(std::logic_error{"error"}));
    EXPECT_FALSE(def.tryFail(std::make_exception_ptr(std::logic_error{"error"})));
    factory->wait();
    EXPECT_EQ(failCount.load(), 0);
    EXPECT_EQ(successCount.load(), 1);

    //first fail with exp ref, then try rest
    failCount = 0;
    successCount = 0;
    def = factory->deferred<int>();
    def.getPromise().onSuccess(successCb).onFailure(failCb);
    EXPECT_TRUE(def.tryFail(std::logic_error{"error"}));
    EXPECT_FALSE(def.tryResolve(43));
    EXPECT_FALSE(def.tryResolve(val));
    EXPECT_FALSE(def.tryFail(std::logic_error{"error"}));
    EXPECT_FALSE(def.tryFail(std::make_exception_ptr(std::logic_error{"error"})));
    factory->wait();
    EXPECT_EQ(failCount.load(), 1);
    EXPECT_EQ(successCount.load(), 0);

    //first fail with exp ptr, then try rest
    failCount = 0;
    successCount = 0;
    def = factory->deferred<int>();
    def.getPromise().onSuccess(successCb).onFailure(failCb);
    EXPECT_TRUE(def.tryFail(std::make_exception_ptr(std::logic_error{"error"})));
    EXPECT_FALSE(def.tryResolve(43));
    EXPECT_FALSE(def.tryResolve(val));
    EXPECT_FALSE(def.tryFail(std::logic_error{"error"}));
    EXPECT_FALSE(def.tryFail(std::make_exception_ptr(std::logic_error{"error"})));
    factory->wait();
    EXPECT_EQ(failCount.load(), 1);
    EXPECT_EQ(successCount.load(), 0);
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
