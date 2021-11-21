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

class VoidPromiseTestSuite : public ::testing::Test {
public:
    ~VoidPromiseTestSuite() noexcept override {
        factory->wait();
    }

    std::shared_ptr<celix::DefaultExecutor> executor = std::make_shared<celix::DefaultExecutor>();
    std::shared_ptr<celix::PromiseFactory> factory = std::make_shared<celix::PromiseFactory>(executor);
};

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-result"
#endif

TEST_F(VoidPromiseTestSuite, simplePromise) {
    auto deferred =  factory->deferred<void>();
    std::thread t{[&deferred] () {
        std::this_thread::sleep_for(std::chrono::milliseconds{50});
        deferred.resolve();
    }};
    auto promise = deferred.getPromise();
    EXPECT_TRUE(promise.getValue()); //block until ready
    EXPECT_TRUE(promise.isDone()); //got value, so promise is done
    EXPECT_ANY_THROW(promise.getFailure()); //succeeded, so no exception available

    EXPECT_TRUE(promise.getValue()); //note multiple call are valid;
    t.join();
}

TEST_F(VoidPromiseTestSuite, failingPromise) {
    auto deferred =  factory->deferred<void>();
    auto cpy = deferred;
    std::thread t{[&deferred] () {
        deferred.fail(std::logic_error{"failing"});
    }};
    auto promise = deferred.getPromise();
    EXPECT_THROW(promise.getValue(), celix::PromiseInvocationException);
    EXPECT_TRUE(promise.getFailure() != nullptr);
    t.join();
}

TEST_F(VoidPromiseTestSuite, failingPromiseWithExceptionPtr) {
    auto deferred =  factory->deferred<void>();
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

TEST_F(VoidPromiseTestSuite, onSuccessHandling) {
    auto deferred =  factory->deferred<void>();
    bool called = false;
    bool resolveCalled = false;
    auto p = deferred.getPromise()
        .onSuccess([&called]() {
            called = true;
        })
        .onResolve([&resolveCalled]() {
            resolveCalled = true;
        });
    deferred.resolve();
    factory->wait();
    EXPECT_EQ(true, called);
    EXPECT_EQ(true, resolveCalled);
}

TEST_F(VoidPromiseTestSuite, onFailureHandling) {
    auto deferred =  factory->deferred<void>();
    bool successCalled = false;
    bool failureCalled = false;
    bool resolveCalled = false;
    auto p = deferred.getPromise()
            .onSuccess([&]() {
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
    factory->wait();
    EXPECT_EQ(false, successCalled);
    EXPECT_EQ(true, failureCalled);
    EXPECT_EQ(true, resolveCalled);
}

TEST_F(VoidPromiseTestSuite, onFailureHandlingLogicError) {
    auto deferred =  factory->deferred<void>();
    std::atomic<bool> successCalled = false;
    std::atomic<bool> failureCalled = false;
    std::atomic<bool> resolveCalled = false;
    auto p = deferred.getPromise()
            .onSuccess([&]() {
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

TEST_F(VoidPromiseTestSuite, onFailureHandlingPromiseIllegalStateException) {
    auto deferred =  factory->deferred<void>();
    std::atomic<bool> successCalled = false;
    std::atomic<bool> failureCalled = false;
    std::atomic<bool> resolveCalled = false;
    auto p = deferred.getPromise()
            .onSuccess([&]() {
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

TEST_F(VoidPromiseTestSuite, onFailureHandlingPromiseInvocationException) {
    auto deferred =  factory->deferred<void>();
    std::atomic<bool> successCalled = false;
    std::atomic<bool> failureCalled = false;
    std::atomic<bool> resolveCalled = false;
    auto p = deferred.getPromise()
            .onSuccess([&]() {
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

TEST_F(VoidPromiseTestSuite, resolveSuccessWith) {
    auto deferred1 = factory->deferred<void>();
    bool called = false;
    deferred1.getPromise()
            .onSuccess([&called]() {
                called = true;
            });

    auto deferred2 = factory->deferred<int>();
    deferred1.resolveWith(deferred2.getPromise());
    deferred2.resolve(42);

    factory->wait();
    EXPECT_EQ(true, called);

    auto deferred3 = factory->deferred<void>();
    called = false;
    deferred3.getPromise()
            .onSuccess([&called]() {
                called = true;
            });

    auto deferred4 = factory->deferred<void>();
    deferred3.resolveWith(deferred4.getPromise());
    deferred4.resolve();

    factory->wait();
    EXPECT_EQ(true, called);
}


TEST_F(VoidPromiseTestSuite, resolveFailureWith) {
    auto deferred1 = factory->deferred<void>();
    auto deferred2 = factory->deferred<void>();
    std::atomic<bool> failureCalled = false;
    std::atomic<bool> successCalled = false;
    deferred2.getPromise()
            .onSuccess([&]() {
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
    factory->wait();
    EXPECT_EQ(false, successCalled);
    EXPECT_EQ(true, failureCalled);
}

TEST_F(VoidPromiseTestSuite, returnPromiseOnSuccessfulResolveWith) {
    auto deferred1 = factory->deferred<void>();
    auto deferred2 = factory->deferred<void>();
    celix::Promise<void> promise = deferred1.resolveWith(deferred2.getPromise());
    EXPECT_FALSE(promise.isDone());
    deferred2.resolve();
    factory->wait();
    EXPECT_TRUE(promise.isDone());
    EXPECT_TRUE(promise.isSuccessfullyResolved());
}

TEST_F(VoidPromiseTestSuite, returnPromiseOnInvalidResolveWith) {
    auto deferred1 = factory->deferred<void>();
    auto deferred2 = factory->deferred<long>();
    celix::Promise<void> promise = deferred1.resolveWith(deferred2.getPromise());
    EXPECT_FALSE(promise.isDone());
    deferred1.resolve(); //Rule is deferred1 is resolved, before the resolveWith the returned promise should fail
    deferred2.resolve(42);
    factory->wait();
    EXPECT_TRUE(promise.isDone());
    EXPECT_FALSE(promise.isSuccessfullyResolved());
    EXPECT_THROW(std::rethrow_exception(promise.getFailure()), celix::PromiseIllegalStateException);
}

//gh-333 fix timeout test on macos
#ifndef __APPLE__
TEST_F(VoidPromiseTestSuite, resolveWithTimeout) {
    auto promise1 = factory->deferredTask<void>([](auto d) {
        std::this_thread::sleep_for(std::chrono::milliseconds{50});
        try {
            d.resolve();
        } catch(...) {
            //note resolve with throws an exception if promise is already resolved
        }
    });

    std::atomic<bool> firstSuccessCalled = false;
    std::atomic<bool> secondSuccessCalled = false;
    std::atomic<bool> secondFailedCalled = false;
    promise1
            .onSuccess([&]() {
                firstSuccessCalled = true;
            })
            .timeout(std::chrono::milliseconds{10})
            .onSuccess([&]() {
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
    auto promise2 = factory->deferredTask<void>([](auto d) {
        d.resolve();
    });

    promise2
            .onSuccess([&]() {
                firstSuccessCalled = true;
            })
            .timeout(std::chrono::milliseconds{250}) /*NOTE: more than the possible delay introduced by the executor*/
            .onSuccess([&]() {
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

TEST_F(VoidPromiseTestSuite, resolveWithSetTimeout) {
    auto promise = factory->deferred<void>().getPromise().setTimeout(std::chrono::milliseconds{5});
    factory->wait();
    EXPECT_TRUE(promise.isDone());
    EXPECT_FALSE(promise.isSuccessfullyResolved());
}

TEST_F(VoidPromiseTestSuite, resolveWithDelay) {
    auto deferred1 = factory->deferred<void>();

    std::atomic<bool> successCalled = false;
    std::atomic<bool> failedCalled = false;
    auto t1 = std::chrono::system_clock::now();
    std::atomic<std::chrono::system_clock::time_point> t2{t1};
    auto p = deferred1.getPromise()
            .delay(std::chrono::milliseconds{50})
            .onSuccess([&]() {
                successCalled = true;
                t2 = std::chrono::system_clock::now();
            })
            .onFailure([&](const std::exception&) {
                failedCalled = true;
            });
    deferred1.resolve();
    p.wait();
    factory->wait();
    EXPECT_EQ(true, successCalled);
    EXPECT_EQ(false, failedCalled);
    auto durationInMs = std::chrono::duration_cast<std::chrono::milliseconds>(t2.load() - t1);
    EXPECT_GE(durationInMs, std::chrono::milliseconds{10});
}

TEST_F(VoidPromiseTestSuite, resolveWithRecover) {
    auto deferred1 = factory->deferred<void>();
    std::atomic<bool> successCalled = false;
    deferred1.getPromise()
            .recover([]{ return 42; })
            .onSuccess([&]() {
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

TEST_F(VoidPromiseTestSuite, chainAndMapResult) {
    auto deferred1 = factory->deferred<void>();
    std::thread t{[&deferred1]{
        deferred1.resolve();
    }};
    int two = deferred1.getPromise()
            .map<int>([]() {
                return 2;
            }).getValue();
    t.join();
    factory->wait();
    EXPECT_EQ(2, two);
}

TEST_F(VoidPromiseTestSuite, chainWithThenAccept) {
    auto deferred1 = factory->deferred<void>();
    std::atomic<bool> called = false;
    deferred1.getPromise()
            .thenAccept([&](){
                called = true;
            });
    deferred1.resolve();
    factory->wait();
    EXPECT_TRUE(called);
}

TEST_F(VoidPromiseTestSuite, promiseWithFallbackTo) {
    auto deferred1 = factory->deferred<void>();
    try {
        throw std::logic_error("failure");
    } catch (...) {
        deferred1.fail(std::current_exception());
    }

    auto deferred2 = factory->deferred<void>();
    deferred2.resolve();


    long val = deferred1.getPromise().fallbackTo(deferred2.getPromise()).getValue();
    EXPECT_TRUE(val);
}

TEST_F(VoidPromiseTestSuite, outOfScopeUnresolvedPromises) {
    std::atomic<bool> called = false;
    {
        auto deferred1 = factory->deferred<void>();
        deferred1.getPromise().onResolve([&]{
            called = true;
        });
        //promise and deferred out of scope
    }
    factory->wait();
    EXPECT_FALSE(called);
}

TEST_F(VoidPromiseTestSuite, chainPromises) {
    auto success = [&](celix::Promise<void> p) -> celix::Promise<long> {
        //TODO Promises::resolved(p.getValue() + p.getValue())
        auto result = factory->deferred<long>();
        p.getValue();
        result.resolve(42);
        return result.getPromise();
    };
    auto initial = factory->deferred<void>();
    initial.resolve();
    long result = initial.getPromise().then<long>(success).getValue();
    EXPECT_EQ(42, result);
}

TEST_F(VoidPromiseTestSuite, chainFailedPromises) {
    std::atomic<bool> called = false;
    auto success = [](celix::Promise<void> p) -> celix::Promise<void> {
        //nop
        return p;
    };
    auto failed = [&](const celix::Promise<void>& /*p*/) -> void {
        called = true;
    };
    auto deferred = factory->deferred<void>();
    deferred.fail(std::logic_error{"fail"});
    deferred.getPromise().then<void>(success, failed);
    factory->wait();
    EXPECT_TRUE(called);
}

TEST_F(VoidPromiseTestSuite, failedResolvedWithPromiseFactory) {
    auto factory = celix::PromiseFactory{};
    auto p1 = factory.failed<void>(std::logic_error{"test"});
    EXPECT_TRUE(p1.isDone());
    EXPECT_NE(nullptr, p1.getFailure());

    auto p2 = factory.resolved();
    EXPECT_TRUE(p2.isDone());
    EXPECT_TRUE(p2.getValue());
}

TEST_F(VoidPromiseTestSuite, deferredTaskCall) {
    auto t1 = std::chrono::system_clock::now();
    auto promise = factory->deferredTask<void>([](auto deferred) {
        std::this_thread::sleep_for(std::chrono::milliseconds{12});
        deferred.resolve();
    });
    promise.wait();
    auto t2 = std::chrono::system_clock::now();
    auto durationInMs = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
    EXPECT_GT(durationInMs, std::chrono::milliseconds{10});
}

TEST_F(VoidPromiseTestSuite, testRobustFailAndResolve) {
    std::atomic<int> failCount{};
    std::atomic<int> successCount{};
    auto failCb = [&failCount](const std::exception& /*e*/) {
        failCount++;
    };
    auto successCb = [&successCount]() {
        successCount++;
    };

    auto def = factory->deferred<void>();
    def.getPromise().onFailure(failCb);

    //Rule a second fail should not lead to an exception, to ensure a more robust usage.
    //But also should only lead to a single resolve chain.
    def.fail(std::logic_error{"error1"});
    def.fail(std::logic_error{"error2"});
    factory->wait();
    EXPECT_EQ(failCount.load(), 1);

    def = factory->deferred<void>();
    def.getPromise().onSuccess(successCb);
    //Rule a second resolve should not lead to an exception, to ensure a more robust usage.
    //But also should only lead to a single resolve chain.
    def.resolve();
    def.resolve();
    factory->wait();
    EXPECT_EQ(successCount.load(), 1);


    failCount = 0;
    successCount = 0;
    def = factory->deferred<void>();
    def.getPromise().onSuccess(successCb).onFailure(failCb);
    //Rule a resolve after fail should not lead to an exception, to ensure a more robust usage.
    //But also should only lead to a single resolve chain.
    def.fail(std::logic_error("error3"));
    def.resolve();
    factory->wait();
    EXPECT_EQ(failCount.load(), 1);
    EXPECT_EQ(successCount.load(), 0);

    failCount = 0;
    successCount = 0;
    def = factory->deferred<void>();
    def.getPromise().onSuccess(successCb).onFailure(failCb);
    //Rule a fail after resolve should not lead to an exception, to ensure a more robust usage.
    //But also should only lead to a single resolve chain.
    def.resolve();
    def.fail(std::logic_error("error3"));
    factory->wait();
    EXPECT_EQ(failCount.load(), 0);
    EXPECT_EQ(successCount.load(), 1);
}

TEST_F(VoidPromiseTestSuite, testTryFailAndResolve) {
    std::atomic<int> failCount{};
    std::atomic<int> successCount{};
    auto failCb = [&failCount](const std::exception& /*e*/) {
        failCount++;
    };
    auto successCb = [&successCount]() {
        successCount++;
    };

    //first resolve, then try rest
    auto def = factory->deferred<void>();
    def.getPromise().onSuccess(successCb).onFailure(failCb);
    EXPECT_TRUE(def.tryResolve());
    EXPECT_FALSE(def.tryFail(std::logic_error{"error"}));
    EXPECT_FALSE(def.tryFail(std::make_exception_ptr(std::logic_error{"error"})));
    factory->wait();
    EXPECT_EQ(failCount.load(), 0);
    EXPECT_EQ(successCount.load(), 1);

    //first fail with exp ref, then try rest
    failCount = 0;
    successCount = 0;
    def = factory->deferred<void>();
    def.getPromise().onSuccess(successCb).onFailure(failCb);
    EXPECT_TRUE(def.tryFail(std::logic_error{"error"}));
    EXPECT_FALSE(def.tryResolve());
    EXPECT_FALSE(def.tryFail(std::logic_error{"error"}));
    EXPECT_FALSE(def.tryFail(std::make_exception_ptr(std::logic_error{"error"})));
    factory->wait();
    EXPECT_EQ(failCount.load(), 1);
    EXPECT_EQ(successCount.load(), 0);

    //first fail with exp ptr, then try rest
    failCount = 0;
    successCount = 0;
    def = factory->deferred<void>();
    def.getPromise().onSuccess(successCb).onFailure(failCb);
    EXPECT_TRUE(def.tryFail(std::make_exception_ptr(std::logic_error{"error"})));
    EXPECT_FALSE(def.tryResolve());
    EXPECT_FALSE(def.tryFail(std::logic_error{"error"}));
    EXPECT_FALSE(def.tryFail(std::make_exception_ptr(std::logic_error{"error"})));
    factory->wait();
    EXPECT_EQ(failCount.load(), 1);
    EXPECT_EQ(successCount.load(), 0);
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
