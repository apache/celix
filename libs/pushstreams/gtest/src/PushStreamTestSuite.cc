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

#include "celix/PushStreamProvider.h"

using celix::PushStreamProvider;

class TestException : public std::exception {
public:
    explicit TestException(const char* what) : w{what} {}
    explicit TestException(std::string what) : w{std::move(what)} {}

    TestException(const TestException&) = delete;
    TestException(TestException&&) noexcept = default;

    TestException& operator=(const TestException&) = delete;
    TestException& operator=(TestException&&) noexcept = default;

    [[nodiscard]] const char* what() const noexcept override { return w.c_str(); }
private:
    std::string w;
};

class EventObject {
public:
    EventObject() : val{0} {
    }

    explicit EventObject(int _val) : val{_val} {
    }

    EventObject(const EventObject& _val) = default;
    EventObject& operator=(const EventObject& other) = default;

    EventObject& operator=(int other) {
        val = other;
        return *this;
    };

    friend EventObject operator+(const EventObject &eo1, const EventObject  &eo2) {
        return EventObject{eo1.val + eo2.val};
    }
    friend int operator+(const int &eo1, const EventObject  &eo2) {
        return eo1 + eo2.val;
    }
    friend int operator+(const EventObject &eo1, const int  &eo2) {
        return eo1.val + eo2;
    }
    int val;
};

class PushStreamTestSuite : public ::testing::Test {
public:
    ~PushStreamTestSuite() noexcept override = default;

    PushStreamProvider psp {};
    std::unique_ptr<std::thread> t{};

    std::shared_ptr<celix::PromiseFactory> promiseFactory {std::make_shared<celix::PromiseFactory>()};
    std::mutex mutex{};
    std::condition_variable done{};
    std::atomic<bool> allEventsDone{false};

    template <typename T>
    std::shared_ptr<celix::AbstractPushEventSource<T>> createEventSource(T event, int publishCount, bool autoinc = false, bool syncSource = true) {
        allEventsDone = false;
        std::shared_ptr<celix::AbstractPushEventSource<T>> ses;
        if (syncSource)
            ses = psp.template createSynchronousEventSource<T>(promiseFactory);
        else
            ses = psp.template createAsynchronousEventSource<T>(promiseFactory);

        auto successLambda = [this, weakses = std::weak_ptr<celix::AbstractPushEventSource<T>>(ses), event, publishCount, autoinc](celix::Promise<void> p) -> celix::Promise<void> {
            t = std::make_unique<std::thread>([&, event, publishCount, autoinc]() {
                int counter = 0;
                T data {event};
                // Keep going as long as someone is listening
                while (counter < publishCount) {
                    auto ses = weakses.lock();
                    if (ses) {
                        ses->publish(data);
                    }
                    if (autoinc) {
                        data = data + 1;
                    }
                    counter++;
                }
            });

            t->join();
            std::unique_lock lk(mutex);
            allEventsDone = true;
            done.notify_one();
            return p;
        };

        auto x = ses->connectPromise().template then<void>(successLambda);

        return ses;
    }
};

TEST_F(PushStreamTestSuite, EventSourceCloseTest) {
    int onClosedReceived{0};
    int onErrorReceived{0};

    auto ses = psp.template createSynchronousEventSource<int>(promiseFactory);
    auto stream = psp.createUnbufferedStream<int>(ses, promiseFactory);

    auto streamEnded = stream->onClose([&](){
        onClosedReceived++;
    }).onError([&](){
        onErrorReceived++;
    }).forEach([&](int /*event*/) { });

    ses->close();

    streamEnded.wait();

    ASSERT_EQ(1, onClosedReceived);
    ASSERT_EQ(0, onErrorReceived);
}

TEST_F(PushStreamTestSuite, ChainedEventSourceCloseTest) {
    int onClosedReceived{0};
    int onErrorReceived{0};

    auto ses = psp.template createSynchronousEventSource<int>(promiseFactory);

    auto stream = psp.createUnbufferedStream<int>(ses, promiseFactory);
    auto& filteredStream = stream->filter([](const int& /*event*/) -> bool {
            return true;
        }).onClose([&](){
            onClosedReceived++;
        }).onError([&](){
            onErrorReceived++;
        });

    auto streamEnded = filteredStream.forEach([&](int /*event*/) { });

    ses->close();

    streamEnded.wait();

    ASSERT_EQ(1, onClosedReceived);
    ASSERT_EQ(0, onErrorReceived);
}


TEST_F(PushStreamTestSuite, StreamCloseTest) {
    int onClosedReceived{0};
    int onErrorReceived{0};

    auto ses = psp.createSynchronousEventSource<int>(promiseFactory);
    auto stream = psp.createUnbufferedStream<int>(ses, promiseFactory);
    auto streamEnded = stream->onClose([&](){
        onClosedReceived++;
    }).onError([&](){
        onErrorReceived++;
    }).forEach([&](int /*event*/) { });

    stream->close();

    streamEnded.wait();

    ASSERT_EQ(1, onClosedReceived);
    ASSERT_EQ(0, onErrorReceived);
}

TEST_F(PushStreamTestSuite, PublishAfterStreamCloseTest) {
    int onClosedReceived{0};
    int onErrorReceived{0};
    int onEventReceived{0};

    auto ses = psp.createSynchronousEventSource<int>(promiseFactory);
    auto stream = psp.createUnbufferedStream<int>(ses, promiseFactory);
    auto streamEnded = stream->onClose([&](){
        onClosedReceived++;
    }).onError([&](){
        onErrorReceived++;
    }).forEach([&](int /*event*/) {
        onEventReceived++;
    });

    stream->close();
    ses->publish(1);

    streamEnded.wait();

    ASSERT_EQ(1, onClosedReceived);
    ASSERT_EQ(0, onErrorReceived);
    ASSERT_EQ(0, onEventReceived);
}


TEST_F(PushStreamTestSuite, ChainedStreamCloseTest) {
    int onClosedReceived{0};
    int onErrorReceived{0};

    auto ses = psp.template createSynchronousEventSource<int>(promiseFactory);
    auto stream = psp.createUnbufferedStream<int>(ses, promiseFactory);
    auto streamEnded = stream->
            filter([](const int& /*event*/) -> bool {
                return true;
            }).onClose([&](){
                onClosedReceived++;
            }).onError([&](){
                onErrorReceived++;
            }).forEach([&](int /*event*/) { });

    stream->close();

    streamEnded.wait();

    ASSERT_EQ(1, onClosedReceived);
    ASSERT_EQ(0, onErrorReceived);
}

TEST_F(PushStreamTestSuite, ChainedStreamIntermedateCloseTest) {
    int onClosedReceived{0};
    int onErrorReceived{0};

    auto ses = psp.template createSynchronousEventSource<int>(promiseFactory);
    auto stream1 = psp.createUnbufferedStream<int>(ses, promiseFactory);
    stream1->onClose([&](){
        onClosedReceived++;
    });
    auto& stream2 = stream1->filter([](const int& /*event*/) -> bool {
                return true;
            }).onClose([&](){
                onClosedReceived++;
            }).onError([&](){
                onErrorReceived++;
            });
    auto streamEnded = stream2.forEach([&](int /*event*/) { });

    stream1->close();

    streamEnded.wait();

    ASSERT_EQ(2, onClosedReceived);
    ASSERT_EQ(0, onErrorReceived);
}

TEST_F(PushStreamTestSuite, ExceptionInStreamTest) {
    int onClosedReceived{0};
    int onErrorReceived{0};

    auto ses = psp.createSynchronousEventSource<int>(promiseFactory);
    auto stream = psp.createUnbufferedStream<int>(ses, promiseFactory);
    auto streamEnded = stream->onClose([&](){
        onClosedReceived++;
    }).onError([&](){
        onErrorReceived++;
    }).forEach([&](int /*event*/) {
        throw TestException("Oops");
    });

    ses->publish(1);

    streamEnded.wait();

    ASSERT_EQ(1, onClosedReceived);
    ASSERT_EQ(1, onErrorReceived);
}

TEST_F(PushStreamTestSuite, ExceptionInChainedStreamTest) {
    int onClosedReceived{0};
    int onErrorReceived{0};
    auto ses = psp.template createSynchronousEventSource<int>(promiseFactory);
    auto stream = psp.createUnbufferedStream<int>(ses, promiseFactory);
    auto streamEnded = stream->filter([](const int& /*event*/) -> bool {
                return true;
            }).onClose([&](){
                onClosedReceived++;
            }).onError([&](){
                onErrorReceived++;
            }).forEach([&](int /*event*/) {
        throw TestException("Oops");
    });

    ses->publish(1);

    streamEnded.wait();

    ASSERT_EQ(1, onClosedReceived);
    ASSERT_EQ(1, onErrorReceived);
}

//
///
/// forEach test
///
TEST_F(PushStreamTestSuite, ForEachTestBasicType) {
    for(int i = 0; i < 100; i++ ) {
        int consumeCount{0};
        int consumeSum{0};
        int lastConsumed{-1};
        std::unique_lock lk(mutex);

        auto ses = createEventSource<int>(0, 10'000, true);

        auto stream = psp.createUnbufferedStream<int>(ses, promiseFactory);
        auto streamEnded = stream->
                forEach([&](int event) {
                    GTEST_ASSERT_EQ(lastConsumed + 1, event);

                    lastConsumed = event;
                    consumeCount++;
                    consumeSum = consumeSum + event;
                });

        done.wait(lk, [&](){ return allEventsDone==true;});
        promiseFactory->getExecutor()->wait();
        ses->close();
        streamEnded.wait();

        GTEST_ASSERT_EQ(10'000, consumeCount);
        GTEST_ASSERT_EQ(49'995'000, consumeSum);
    }
}

///
/// forEach test
///
TEST_F(PushStreamTestSuite, ForEachTestBasicType_Buffered) {

    for(int i = 0; i < 100; i++ ) {
        int consumeCount{0};
        int consumeSum{0};
        int lastConsumed{-1};
        std::unique_lock lk(mutex);

        auto ses = createEventSource<int>(0, 10'000, true);

        auto stream = psp.createStream<int>(ses, promiseFactory);
        auto streamEnded = stream->
                forEach([&](int event) {
                    GTEST_ASSERT_EQ(lastConsumed + 1, event);

                    lastConsumed = event;
                    consumeCount++;
                    consumeSum = consumeSum + event;
                });

        done.wait(lk, [&](){ return allEventsDone==true;});
        promiseFactory->getExecutor()->wait();
        ses->close();
        streamEnded.wait();

        GTEST_ASSERT_EQ(10'000, consumeCount);
        GTEST_ASSERT_EQ(49'995'000, consumeSum);
    }

}

TEST_F(PushStreamTestSuite, ForEachTestObjectType) {
    std::atomic<int> consumeCount{0};
    std::atomic<int> consumeSum{0};
    std::unique_lock lk(mutex);

    auto ses = createEventSource<EventObject>(EventObject{2}, 10);

    auto stream = psp.createUnbufferedStream<EventObject>(ses, promiseFactory);
    auto streamEnded = stream->
            forEach([&](const EventObject& event) {
                consumeCount++;
                consumeSum = consumeSum + event;
            });

    done.wait(lk, [&](){ return allEventsDone==true;});
    promiseFactory->getExecutor()->wait();
    ses->close();
    streamEnded.wait();

    GTEST_ASSERT_EQ(10, consumeCount);
    GTEST_ASSERT_EQ(20, consumeSum);
}


//Filter Test
TEST_F(PushStreamTestSuite, FilterTestObjectType_true) {
    int consumeCount{0};
    int consumeSum{0};
    std::unique_lock lk(mutex);

    auto ses = createEventSource<EventObject>(EventObject{2}, 10);

    auto stream = psp.createUnbufferedStream<EventObject>(ses, promiseFactory);
    auto streamEnded = stream->
            filter([](const EventObject& /*event*/) -> bool {
               return true;
            }).
            forEach([&](EventObject event) {
                consumeCount++;
                consumeSum = consumeSum + event;
            });

    done.wait(lk, [&](){ return allEventsDone==true;});
    promiseFactory->getExecutor()->wait();
    ses->close();
    streamEnded.wait();

    GTEST_ASSERT_EQ(10, consumeCount);
    GTEST_ASSERT_EQ(20, consumeSum);
}

//Filter Test
TEST_F(PushStreamTestSuite, FilterTestObjectType_false) {
    int consumeCount{0};
    int consumeSum{0};
    std::unique_lock lk(mutex);

    auto ses = createEventSource<EventObject>(EventObject{2}, 10);

    auto stream = psp.createUnbufferedStream<EventObject>(ses, promiseFactory);
    auto streamEnded = stream->
            filter([](const EventObject& /*event*/) -> bool {
                return false;
            }).
            forEach([&](EventObject event) {
                consumeCount++;
                consumeSum = consumeSum + event;
            });

    done.wait(lk, [&](){ return allEventsDone==true;});
    promiseFactory->getExecutor()->wait();
    ses->close();
    streamEnded.wait();

    GTEST_ASSERT_EQ(0, consumeCount);
    GTEST_ASSERT_EQ(0, consumeSum);
}

TEST_F(PushStreamTestSuite, FilterTestObjectType_simple) {
    int consumeCount{0};
    int consumeSum{0};

    std::unique_lock lk(mutex);

    auto ses = createEventSource<EventObject>(EventObject{0}, 10, true);

    auto stream = psp.createUnbufferedStream<EventObject>(ses, promiseFactory);
    auto streamEnded = stream->
            filter([](const EventObject& event) -> bool {
                return event.val < 5;
            }).
            forEach([&](EventObject event) {
                consumeCount++;
                consumeSum = consumeSum + event;
            });

    done.wait(lk, [&](){ return allEventsDone==true;});
    promiseFactory->getExecutor()->wait();
    ses->close();
    streamEnded.wait();

    GTEST_ASSERT_EQ(5, consumeCount);
    GTEST_ASSERT_EQ( 0 + 1 + 2 + 3 + 4, consumeSum);
}

TEST_F(PushStreamTestSuite, FilterTestObjectType_and) {
    int consumeCount{0};
    int consumeSum{0};
    std::unique_lock lk(mutex);

    auto ses = createEventSource<EventObject>(EventObject{0}, 10, true);

    auto stream = psp.createUnbufferedStream<EventObject>(ses, promiseFactory);
    auto streamEnded = stream->
            filter([](const EventObject& predicate) -> bool {
                return predicate.val > 5;
            }).
            filter([](const EventObject& predicate) -> bool {
                return predicate.val < 8;
            }).
            forEach([&](const EventObject& event) {
                consumeCount++;
                consumeSum = consumeSum + event;
            });

    done.wait(lk, [&](){ return allEventsDone==true;});
    promiseFactory->getExecutor()->wait();
    ses->close();
    streamEnded.wait();

    GTEST_ASSERT_EQ(2, consumeCount);
    GTEST_ASSERT_EQ(6 + 7, consumeSum); // 0 + 1 + 2 + 3 + 4
}

TEST_F(PushStreamTestSuite, MapTestObjectType) {
    int consumeCount{0};
    int consumeSum{0};
    std::unique_lock lk(mutex);

    auto ses = createEventSource<EventObject>(EventObject{0}, 10, true);
    auto stream = psp.createUnbufferedStream<EventObject>(ses, promiseFactory);
    auto streamEnded = stream->
            map<int>([](const EventObject& event) -> int {
                return event.val;
            }).
            forEach([&](const int& event) {
                consumeCount++;
                consumeSum = consumeSum + event;
            });

    done.wait(lk, [&](){ return allEventsDone==true;});
    promiseFactory->getExecutor()->wait();
    ses->close();
    streamEnded.wait();

    GTEST_ASSERT_EQ(10, consumeCount);
    GTEST_ASSERT_EQ(45, consumeSum);
}

TEST_F(PushStreamTestSuite, MapTestObjectType_async) {
    for(int i = 0; i < 1000; i++) {
        std::atomic<int> consumeCount{0};
        std::atomic<int> consumeSum{0};
        std::unique_lock lk(mutex);

        auto ses = createEventSource<EventObject>(EventObject{0}, 10, true, false);
        auto stream = psp.createUnbufferedStream<EventObject>(ses, promiseFactory);
        auto streamEnded = stream->
                map<int>([](const EventObject &event) -> int {
            return event.val;
        }).
                forEach([&](const int &event) {
            consumeCount++;
            consumeSum += + event;
        });

        done.wait(lk, [&](){ return allEventsDone==true;});
        promiseFactory->getExecutor()->wait();
        ses->close();
        streamEnded.wait();

        GTEST_ASSERT_EQ(10, consumeCount);
        GTEST_ASSERT_EQ(45, consumeSum);
    }
}


TEST_F(PushStreamTestSuite, MultipleStreamsTest_CloseSource) {
    int onEventStream1{0};
    int onEventStream2{0};
    int onClosedReceived1{0};
    int onClosedReceived2{0};
    int onErrorReceived1{0};
    int onErrorReceived2{0};

    std::unique_lock lk(mutex);

    auto psp = PushStreamProvider();
    std::unique_ptr<std::thread> t{};
    auto ses = createEventSource<int>(0, 20, true);

    auto stream1 = psp.createUnbufferedStream<int>(ses, promiseFactory);
    auto streamEnded1 = stream1->
    filter([&](int event) -> bool {
        return (event > 10);
    }).
    filter([&](int event) -> bool {
        return (event < 15);
    }).onClose([&](){
        onClosedReceived1++;
    }).onError([&](){
        onErrorReceived1++;
    }).
    forEach([&](int /*event*/) {
        onEventStream1++;
    });

    auto stream2 = psp.createUnbufferedStream<int>(ses, promiseFactory);
    auto streamEnded2 = stream2->
    filter([&](int event) -> bool {
        return (event < 15);
    }).onClose([&](){
        onClosedReceived2++;
    }).onError([&](){
        onErrorReceived2++;
    }).forEach([&](int /*event*/) {
        onEventStream2++;
    });

    streamEnded1.onSuccess([]() {
    });

    streamEnded2.onSuccess([]() {
    });


    done.wait(lk, [&](){ return allEventsDone==true;});
    promiseFactory->getExecutor()->wait();
    ses->close();
    streamEnded1.wait();
    streamEnded2.wait();

    GTEST_ASSERT_EQ(4, onEventStream1);
    //The first stream will start the source, thus the number of receives in second is not guaranteed
    //GTEST_ASSERT_EQ(15, onEventStream2);


    GTEST_ASSERT_EQ(1, onClosedReceived1);
    GTEST_ASSERT_EQ(1, onClosedReceived2);
    GTEST_ASSERT_EQ(0, onErrorReceived1);
    GTEST_ASSERT_EQ(0, onErrorReceived2);
}

TEST_F(PushStreamTestSuite, MultipleStreamsTest_CloseStream) {
    int onEventStream1{0};
    int onEventStream2{0};
    auto psp = PushStreamProvider();
    std::unique_ptr<std::thread> t{};
    auto ses = psp.template createSynchronousEventSource<int>(promiseFactory);

    auto successLambda = [](celix::Promise<void> p) -> celix::Promise<void> {
        return p;
    };
    auto x = ses->connectPromise().template then<void>(successLambda);

    auto stream1 = psp.createUnbufferedStream<int>(ses, promiseFactory);

    auto streamEnded1 = stream1->
            filter([&](int event) -> bool {
                return (event > 10);
            }).
            filter([&](int event) -> bool {
                return (event < 15);
            }).
            forEach([&](int /*event*/) {
                onEventStream1++;
            });

    auto stream2 = psp.createUnbufferedStream<int>(ses, promiseFactory);
    auto streamEnded2 = stream2->
            filter([&](int event) -> bool {
                return (event < 15);
            }).
            forEach([&](int /*event*/) {
                onEventStream2++;
            });

    streamEnded1.onSuccess([]() {
    });

    streamEnded2.onSuccess([]() {
    });

    stream1->close();
    streamEnded1.wait();

    ses->publish(10);

    stream2->close();
    streamEnded2.wait();

    GTEST_ASSERT_EQ(0, onEventStream1);
    GTEST_ASSERT_EQ(1, onEventStream2);
}


TEST_F(PushStreamTestSuite, SplitStreamsTest) {
    std::map<int,int> counts{};
    counts[0] = 0;
    counts[1] = 0;
    int onClosedReceived{0};

    std::unique_lock lk(mutex);

    auto psp = PushStreamProvider();
    std::unique_ptr<std::thread> t{};
    auto ses = createEventSource<int>(0, 20, true);


    auto stream = psp.createUnbufferedStream<int>(ses, promiseFactory);
    auto splitStream = stream->
        split({
                            [&](int event) -> bool {
                                return (event > 10);
                            },
                            [&](int event) -> bool {
                                return (event < 12);
                            }
        });

    std::vector<celix::Promise<void>> streamEndeds{};

    for(long unsigned int i = 0; i < splitStream.size(); i++) {
        streamEndeds.push_back(splitStream[i]->onClose([&](){
            onClosedReceived++;
        }).forEach([&, i = i](int /*event*/) {
            counts[i] = counts[i] + 1;
        }));

        streamEndeds[i].onSuccess([]() {
        });
    }


    done.wait(lk, [&](){ return allEventsDone==true;});
    promiseFactory->getExecutor()->wait();
    ses->close();
    promiseFactory->getExecutor()->wait();

    GTEST_ASSERT_EQ(2, onClosedReceived);
    GTEST_ASSERT_EQ(9, counts[0]);
    //The first stream will start the source, thus the number of receives in second is not guaranteed
    //GTEST_ASSERT_EQ(12, counts[1]);
}

