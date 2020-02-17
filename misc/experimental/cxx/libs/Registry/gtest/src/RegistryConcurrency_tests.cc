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


#include <thread>
#include <mutex>
#include <condition_variable>

#include "gtest/gtest.h"

#include "celix/ServiceRegistry.h"
#include "celix/Constants.h"

class RegistryConcurrencyTest : public ::testing::Test {
public:
    RegistryConcurrencyTest() {}

    ~RegistryConcurrencyTest() {}

    celix::ServiceRegistry &registry() { return reg; }

private:
    celix::ServiceRegistry reg{"test"};
};


TEST_F(RegistryConcurrencyTest, ServiceRegistrationTest) {
    struct calc {
        int (*calc)(int);
    };

    auto svc = std::make_shared<calc>();
    svc->calc = [](int a) {
        return a * 42;
    };


    auto svcReg = registry().registerService(svc);
    EXPECT_GE(svcReg.serviceId(), 0);

    struct sync {
        std::mutex mutex{};
        std::condition_variable sync{};
        bool inUseCall{false};
        bool readyToExitUseCall{false};
        bool unregister{false};
        int result{0};
    };
    struct sync callInfo{};

    auto use = [&callInfo](const std::shared_ptr<calc>& svc) {
        std::cout << "setting isUseCall to true and syncing on readyToExitUseCall" << std::endl;
        std::unique_lock<std::mutex> lock(callInfo.mutex);
        callInfo.inUseCall = true;
        callInfo.sync.notify_all();
        callInfo.sync.wait(lock, [&callInfo]{return callInfo.readyToExitUseCall;});
        lock.unlock();

        std::cout << "Calling calc " << std::endl;
        int tmp = svc->calc(2);
        callInfo.result = tmp;
    };

    auto call = [&] {
        celix::UseServiceOptions<calc> opts{};
        opts.targetServiceId = svcReg.serviceId();
        opts.use = use;
        bool called = registry().useService(opts);
        EXPECT_TRUE(called);
        EXPECT_EQ(84, callInfo.result);
    };
    std::thread useThread{call};


    std::thread unregisterThread{[&]{
        std::cout << "syncing to wait if use function is called ..." << std::endl;
        std::unique_lock<std::mutex> lock(callInfo.mutex);
        callInfo.sync.wait(lock, [&]{return callInfo.inUseCall;});
        lock.unlock();
        std::cout << "trying to unregister ..." << std::endl;
        svcReg.unregister();
        std::cout << "done unregistering" << std::endl;
    }};


    //sleep 100 milli to give unregister a change to sink in
    std::cout << "before sleep" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "after sleep" << std::endl;


    std::cout << "setting readyToExitUseCall and notify" << std::endl;
    std::unique_lock<std::mutex> lock(callInfo.mutex);
    callInfo.readyToExitUseCall = true;
    lock.unlock();
    callInfo.sync.notify_all();

    useThread.join();
    std::cout << "use thread joined" << std::endl;
    unregisterThread.join();
    std::cout << "unregister thread joined" << std::endl;
};


class ICalc {
public:
    virtual ~ICalc() = default;
    virtual double calc();
};

class NodeCalc : public ICalc {
public:
    virtual ~NodeCalc() = default;

    double calc() override {
        double val = 1.0;
        std::lock_guard<std::mutex> lck{mutex};
        for (auto *calc : childs) {
            val *= calc->calc();
        }
        return val;
    }
private:
    std::mutex mutex{};
    std::vector<ICalc*> childs{};
};

class LeafCalc : public ICalc {

public:
    virtual ~LeafCalc() = default;

    double calc() override {
        return seed;
    }
private:
    double const seed = std::rand() / 100.0;
};

TEST_F(RegistryConcurrencyTest, ManyThreadsTest) {
//TODO start many thread and cals using the tiers of other calcs eventually leading ot leafs and try to break the registry.
//tier 1 : NodeCalc
//tier 2 : NodeCalc
//tier 3 : LeafCalc
}
