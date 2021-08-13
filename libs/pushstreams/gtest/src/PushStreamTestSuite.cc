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

class EventObject {
public:
    //TODO discussion EventData needs to be default constructable due to close event data {}
    EventObject() : val{0} {
    }

    explicit EventObject(int _val) : val{_val} {
    }

    EventObject(const EventObject& _val) : val{_val.val} {
    }

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

    template <typename T>
    std::shared_ptr<celix::SynchronousPushEventSource<T>> createEventSource(T event, int publishCount, bool autoinc = false) {
        auto ses = psp.template createSynchronousEventSource<T>();

        auto successLambda = [this, ses, event, publishCount, autoinc](celix::Promise<void> p) -> celix::Promise<void> {
            t = std::make_unique<std::thread>([&, event, publishCount, autoinc]() {
                int counter = 0;
                T data {event};
                // Keep going as long as someone is listening
                while (counter < publishCount) {
                    ses->publish(data);
                    if (autoinc) {
                        data = data + 1;
                    }
                    counter++;
                }
            });

            t->join();
            ses->close();
            return p;
        };

        auto x = ses->connectPromise().template then<void>(successLambda);

        return ses;
    }
};

//TEST_F(PushStreamTestSuite, EventSourceCloseTest) {
//    int onClosedReceived{0};
//    auto ses = psp.template createSynchronousEventSource<int>();
//
//    auto successLambda = [](celix::Promise<void> p) -> celix::Promise<void> {
//        return p;
//    };
//    auto x = ses->connectPromise().then<void>(successLambda);
//
//    auto& stream = psp.createStream<int>(ses).onClose([&](){
//        onClosedReceived++;
//    });
//    auto streamEnded = stream.forEach([&](int /*event*/) { });
//
//    ses->close();
//
//    streamEnded.wait(); //todo marked is not in OSGi Spec
//
//    ASSERT_EQ(1, onClosedReceived);
//}

//TEST_F(PushStreamTestSuite, ChainedEventSourceCloseTest) {
//    int onClosedReceived{0};
//
//    auto ses = psp.template createSynchronousEventSource<int>();
//
//    auto successLambda = [](celix::Promise<void> p) -> celix::Promise<void> {
//        return p;
//    };
//    auto x = ses->connectPromise().then<void>(successLambda);
//
//    auto& stream = psp.createStream<int>(ses).
//        filter([](const int& /*event*/) -> bool {
//            return true;
//        }).onClose([&](){
//            onClosedReceived++;
//        });
//    auto streamEnded = stream.forEach([&](int /*event*/) { });
//
//    ses->close();
//
//    streamEnded.wait(); //todo marked is not in OSGi Spec
//
//    ASSERT_EQ(1, onClosedReceived);
//}


TEST_F(PushStreamTestSuite, StreamCloseTest) {
    int onClosedReceived{0};

    auto ses = psp.createSynchronousEventSource<int>();

    auto successLambda = [](celix::Promise<void> p) -> celix::Promise<void> {
        return p;
    };
    auto x = ses->connectPromise().then<void>(successLambda);

    auto& stream = psp.createStream<int>(ses).onClose([&](){
        onClosedReceived++;
    });;
    auto streamEnded = stream.forEach([&](int /*event*/) { });

    stream.close();

    streamEnded.wait(); //todo marked is not in OSGi Spec

    ASSERT_EQ(1, onClosedReceived);
}

TEST_F(PushStreamTestSuite, ChainedStreamCloseTest) {
    auto ses = psp.template createSynchronousEventSource<int>();

    auto successLambda = [](celix::Promise<void> p) -> celix::Promise<void> {
        return p;
    };
    auto x = ses->connectPromise().template then<void>(successLambda);

    auto& stream = psp.createStream<int>(ses).
            filter([](const int& /*event*/) -> bool {
                return true;
            });
    auto streamEnded = stream.forEach([&](int /*event*/) { });

    stream.close();

    streamEnded.wait(); //todo marked is not in OSGi Spec
}
///
/// forEach test
///
TEST_F(PushStreamTestSuite, ForEachTestBasicType) {
    int consumeCount{0};
    int consumeSum{0};
    int lastConsumed{-1};
    auto ses = createEventSource<int>(0, 10'000, true);

    auto streamEnded = psp.createStream<int>(ses).
            forEach([&](int event) {
                GTEST_ASSERT_EQ(lastConsumed + 1, event);

                lastConsumed = event;
                consumeCount++;
                consumeSum = consumeSum + event;
            });

    streamEnded.wait(); //todo marked is not in OSGi Spec

    GTEST_ASSERT_EQ(10'000, consumeCount);
    GTEST_ASSERT_EQ(49'995'000, consumeSum);
}

TEST_F(PushStreamTestSuite, ForEachTestObjectType) {
    int consumeCount{0};
    int consumeSum{0};
    auto ses = createEventSource<EventObject>(EventObject{2}, 10);

    auto streamEnded = psp.createStream<EventObject>(ses).
            forEach([&](const EventObject& event) {
                consumeCount++;
                consumeSum = consumeSum + event;
            });

    streamEnded.wait(); //todo marked is not in OSGi Spec

    GTEST_ASSERT_EQ(10, consumeCount);
    GTEST_ASSERT_EQ(20, consumeSum);
}

//Filter Test
TEST_F(PushStreamTestSuite, FilterTestObjectType_true) {
    int consumeCount{0};
    int consumeSum{0};
    auto ses = createEventSource<EventObject>(EventObject{2}, 10);

    auto streamEnded = psp.createStream<EventObject>(ses).
            filter([](const EventObject& /*event*/) -> bool {
               return true;
            }).
            forEach([&](EventObject event) {
                consumeCount++;
                consumeSum = consumeSum + event;
            });

    streamEnded.wait(); //todo marked is not in OSGi Spec

    GTEST_ASSERT_EQ(10, consumeCount);
    GTEST_ASSERT_EQ(20, consumeSum);
}

//Filter Test
TEST_F(PushStreamTestSuite, FilterTestObjectType_false) {
    int consumeCount{0};
    int consumeSum{0};
    auto ses = createEventSource<EventObject>(EventObject{2}, 10);

    auto streamEnded = psp.createStream<EventObject>(ses).
            filter([](const EventObject& /*event*/) -> bool {
                return false;
            }).
            forEach([&](EventObject event) {
                consumeCount++;
                consumeSum = consumeSum + event;
            });

    streamEnded.wait(); //todo marked is not in OSGi Spec

    GTEST_ASSERT_EQ(0, consumeCount);
    GTEST_ASSERT_EQ(0, consumeSum);
}

TEST_F(PushStreamTestSuite, FilterTestObjectType_simple) {
    int consumeCount{0};
    int consumeSum{0};
    auto ses = createEventSource<EventObject>(EventObject{0}, 10, true);

    auto streamEnded = psp.createStream<EventObject>(ses).
            filter([](const EventObject& event) -> bool {
                return event.val < 5;
            }).
            forEach([&](EventObject event) {
                consumeCount++;
                consumeSum = consumeSum + event;
            });

    streamEnded.wait(); //todo marked is not in OSGi Spec

    GTEST_ASSERT_EQ(5, consumeCount);
    GTEST_ASSERT_EQ( 0 + 1 + 2 + 3 + 4, consumeSum);
}

TEST_F(PushStreamTestSuite, FilterTestObjectType_and) {
    int consumeCount{0};
    int consumeSum{0};
    auto ses = createEventSource<EventObject>(EventObject{0}, 10, true);

    auto streamEnded = psp.createStream<EventObject>(ses).
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

    streamEnded.wait(); //todo marked is not in OSGi Spec

    GTEST_ASSERT_EQ(2, consumeCount);
    GTEST_ASSERT_EQ(6 + 7, consumeSum); // 0 + 1 + 2 + 3 + 4
}

TEST_F(PushStreamTestSuite, MapTestObjectType) {
    int consumeCount{0};
    int consumeSum{0};
    auto ses = createEventSource<EventObject>(EventObject{0}, 10, true);

    auto streamEnded = psp.createStream<EventObject>(ses).
            map<int>([](const EventObject& event) -> int {
                return event.val;
            }).
            forEach([&](const int& event) {
                consumeCount++;
                consumeSum = consumeSum + event;
            });

    streamEnded.wait(); //todo marked is not in OSGi Spec

    GTEST_ASSERT_EQ(10, consumeCount);
    GTEST_ASSERT_EQ(45, consumeSum);
}



//TEST_F(PushStreamTestSuite, BasicTest) {
//    auto psp = PushStreamProvider();
//    std::unique_ptr<std::thread> t{};
//    auto ses = psp.createSimpleEventSource<int>();
//
//    auto successLamba = [&](celix::Promise<void> p) -> celix::Promise<void> {
//        t = std::make_unique<std::thread>([&]() {
//            long counter = 0;
//            // Keep going as long as someone is listening
//            while (ses->isConnected()) {
//                ses->publish(++counter);
//                std::this_thread::sleep_for(std::chrono::milliseconds{50});
//                std::cout << "Published: " << counter << std::endl;
//            }
//            // Restart delivery when a new listener connects
//            // ses.connectPromise().then(success);
//        });
//        return p;
//    };
//
//    auto x = ses->connectPromise().then<void>(successLamba);
//
//    auto streamEnded = psp.createStream<int>(ses).
//    filter([&](int event) -> bool {
//        return (event > 10);
//    }).
//    filter([&](int event) -> bool {
//        return (event < 15);
//    }).
//    forEach([](int event) {
//        std::cout << "Consumed event: " << event << std::endl;
//    });
//
//    streamEnded.onSuccess([]() {
//        std::cout << "Stream ended" << std::endl;
//    });
//
//    std::this_thread::sleep_for(std::chrono::seconds{1});
//
//    ses->close();
//
//    t->join();
//}
//
//
//TEST_F(PushStreamTestSuite, MultipleStreamsTest) {
//    auto psp = PushStreamProvider();
//    std::unique_ptr<std::thread> t{};
//    auto ses = psp.createSimpleEventSource<int>();
//
//    auto success = [&](celix::Promise<void> p) -> celix::Promise<void> {
//        t = std::make_unique<std::thread>([&]() {
//            long counter = 0;
//            // Keep going as long as someone is listening
//            while (ses->isConnected()) {
//                ses->publish(++counter);
//                std::this_thread::sleep_for(std::chrono::milliseconds{50});
//                std::cout << "Published: " << counter << std::endl;
//            }
//            // Restart delivery when a new listener connects
//            // ses.connectPromise().then(success);
//        });
//        return p;
//    };
//
//    ses->connectPromise().then<void>(success);
//
//    auto streamEnded = psp.createStream<int>(ses).
//    filter([&](int event) -> bool {
//        return (event > 10);
//    }).
//    filter([&](int event) -> bool {
//        return (event < 15);
//    }).
//    forEach([&](int event) {
//        std::cout << "Consumed event 1: " << event << std::endl;
//    });
//
//    auto streamEnded2 = psp.createStream<int>(ses).
//    filter([&](int event) -> bool {
//        return (event < 15);
//    }).
//    forEach([&](int event) {
//        std::cout << "Consumed event 2: " << event << std::endl;
//    });
//
//    streamEnded.onSuccess([]() {
//        std::cout << "Stream ended" << std::endl;
//    });
//
//    streamEnded2.onSuccess([]() {
//        std::cout << "Stream2 ended" << std::endl;
//    });
//
//    std::this_thread::sleep_for(std::chrono::seconds{1});
//
//    ses->close();
//    t->join();
//}
//
//
//TEST_F(PushStreamTestSuite, SplitStreamsTest) {
//    auto psp = PushStreamProvider();
//    std::unique_ptr<std::thread> t{};
//    auto ses = psp.createSimpleEventSource<int>();
//
//    auto success = [&](celix::Promise<void> p) -> celix::Promise<void> {
//        t = std::make_unique<std::thread>([&]() {
//            long counter = 0;
//            // Keep going as long as someone is listening
//            while (ses->isConnected()) {
//                ses->publish(++counter);
//                std::this_thread::sleep_for(std::chrono::milliseconds{50});
//                std::cout << "Published: " << counter << std::endl;
//            }
//            // Restart delivery when a new listener connects
//            // ses.connectPromise().then(success);
//        });
//        return p;
//    };
//
//    ses->connectPromise().then<void>(success);
//
//    auto splitStream = psp.createStream<int>(ses).
//        split({
//                            [&](int event) -> bool {
//                                return (event > 10);
//                            },
//                            [&](int event) -> bool {
//                                return (event < 12);
//                            }
//        });
//
//    std::vector<celix::Promise<void>> streamEndeds{};
//
//    for(long unsigned int i = 0; i < splitStream.size(); i++) {
//        streamEndeds.push_back(splitStream[i]->forEach([=](int event) {
//            std::cout << "consume split " << i << ", event data " << event << std::endl;
//        }));
//
//        streamEndeds[i].onSuccess([]() {
//            std::cout << "Stream ended" << std::endl;
//        });
//    }
//
//    std::this_thread::sleep_for(std::chrono::seconds{1});
//
//    ses->close();
//    t->join();
//}
