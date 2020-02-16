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

#include <atomic>

#include "gtest/gtest.h"

#include "celix/Framework.h"

class FrameworkTest : public ::testing::Test {
public:
    FrameworkTest() {}
    ~FrameworkTest(){}

    celix::Framework& framework() { return fw; }
private:
    celix::Framework fw{};
};


TEST_F(FrameworkTest, CreateDestroy) {
    //no bundles installed bundle (framework bundle)
    EXPECT_EQ(1, framework().listBundles(true).size());
    EXPECT_EQ(0, framework().listBundles(false).size());

    bool isFramework = false;
    framework().useBundle(celix::FRAMEWORK_BUNDLE_ID, [&](const celix::IBundle &bnd) {
       isFramework = bnd.isFrameworkBundle();
    });
    EXPECT_TRUE(isFramework);
}

class EmbeddedActivator : public celix::IBundleActivator {
public:
    explicit EmbeddedActivator(std::shared_ptr<celix::BundleContext>) {
        startCount.fetch_add(1);
    }

    virtual ~EmbeddedActivator() {
        stopCount.fetch_add(1);
    }

    static std::atomic<int> startCount;
    static std::atomic<int> stopCount;
};

std::atomic<int> EmbeddedActivator::startCount{0};
std::atomic<int> EmbeddedActivator::stopCount{0};

TEST_F(FrameworkTest, InstallBundle) {
    EmbeddedActivator::startCount = 0;
    EmbeddedActivator::stopCount = 0;

    auto actFactory = [](std::shared_ptr<celix::BundleContext> ctx) -> celix::IBundleActivator* {
        return new EmbeddedActivator{std::move(ctx)};
    };
    long bndId1 = framework().installBundle("embedded", actFactory);
    EXPECT_GE(bndId1, 0);
    EXPECT_EQ(1, EmbeddedActivator::startCount);
    EXPECT_EQ(0, EmbeddedActivator::stopCount);

    long bndId2 = framework().installBundle<EmbeddedActivator>("embedded2");
    EXPECT_GE(bndId2, 0);
    EXPECT_NE(bndId1, bndId2);
    EXPECT_EQ(2, EmbeddedActivator::startCount);
    EXPECT_EQ(0, EmbeddedActivator::stopCount);

    framework().stopBundle(bndId2);
    EXPECT_EQ(1, EmbeddedActivator::stopCount);

    {
        celix::Framework fw{};
        fw.installBundle<EmbeddedActivator>("embedded3");
        EXPECT_EQ(3, EmbeddedActivator::startCount);
        EXPECT_EQ(1, EmbeddedActivator::stopCount);

        //NOTE fw out of scope -> bundle stopped
    }
    EXPECT_EQ(3, EmbeddedActivator::startCount);
    EXPECT_EQ(2, EmbeddedActivator::stopCount);
}

TEST_F(FrameworkTest, StaticBundleTest) {
    class EmbeddedActivator : public celix::IBundleActivator {
    public:
        EmbeddedActivator() {}
        virtual ~EmbeddedActivator() = default;
    };

    int count = 0;
    auto factory = [&](std::shared_ptr<celix::BundleContext>) -> celix::IBundleActivator * {
        count++;
        return new EmbeddedActivator{};
    };

    EXPECT_EQ(0, framework().listBundles().size()); //no bundles installed;
    celix::registerStaticBundle("static", factory);
    EXPECT_EQ(1, framework().listBundles().size()); //static bundle instance installed
    EXPECT_EQ(1, count);

    celix::Framework fw{};
    EXPECT_EQ(1, framework().listBundles().size()); //already registered static bundle instance installed.
    EXPECT_EQ(2, count);
}