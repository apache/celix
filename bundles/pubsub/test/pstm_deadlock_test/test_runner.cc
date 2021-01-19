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

#include "celix_api.h"
#include <celix/dm/Component.h>
#include <celix/dm/DependencyManager.h>
#include "pubsub/api.h"
#include <unistd.h>
#include <memory>
#include <mutex>

#include <gtest/gtest.h>
#include <future>

constexpr const char *deadlockSutBundleFile = DEADLOCK_SUT_BUNDLE_FILE;

struct DeadlockTestSuite : public ::testing::Test {
    celix_framework_t *fw = NULL;
    celix_bundle_context_t *ctx = NULL;
    std::shared_ptr<DependencyManager> mng = NULL;
    long sutBundleId = 0;

    DeadlockTestSuite() {
        celixLauncher_launch("config.properties", &fw);
        ctx = celix_framework_getFrameworkContext(fw);
        mng = std::shared_ptr<DependencyManager>(new DependencyManager(ctx));
        sutBundleId = celix_bundleContext_installBundle(ctx, deadlockSutBundleFile, false);
    }

    ~DeadlockTestSuite() override {
        celix_bundleContext_uninstallBundle(ctx, sutBundleId);
        mng = nullptr;
        celixLauncher_stop(fw);
        celixLauncher_waitForShutdown(fw);
        celixLauncher_destroy(fw);
        ctx = NULL;
        fw = NULL;
    }

    DeadlockTestSuite(const DeadlockTestSuite&) = delete;
    DeadlockTestSuite& operator=(const DeadlockTestSuite&) = delete;
};

class DependencyCmp;

struct activator {
    std::string cmpUUID{};
    celix_bundle_context_t *ctx{};
    std::promise<void> promise{};
    std::shared_ptr<celix::dm::DependencyManager> mng{};
};

class IDependency {
protected:
    IDependency() = default;
    ~IDependency() = default;
public:
    virtual double getData() = 0;
};

class DependencyCmp : public IDependency {
public:
    DependencyCmp() {
        std::cout << "Creating DependencyCmp\n";
    }
    virtual ~DependencyCmp() { std::cout << "Destroying DependencyCmp\n"; };

    DependencyCmp(DependencyCmp&& other) noexcept = default;
    DependencyCmp& operator=(DependencyCmp&&) = default;

    DependencyCmp(const DependencyCmp& other) = delete;
    DependencyCmp operator=(const DependencyCmp&) = delete;

    double getData() override {
        return 1.0;
    }

    void setPublisher(const pubsub_publisher_t *pub) {
        if (pub == nullptr) {
            return; //nothing on "unsetting" svc
        }
        auto cmp = act->mng->findComponent<DependencyCmp>(act->cmpUUID);
        EXPECT_TRUE(cmp);
        if (cmp) {
            cmp->createCServiceDependency<pubsub_publisher_t>(PUBSUB_PUBLISHER_SERVICE_NAME)
                    .setVersionRange("[3.0.0,4)")
                    .setFilter("(topic=deadlock)(scope=scope2)")
                    .setStrategy(celix::dm::DependencyUpdateStrategy::suspend)
                    .setCallbacks([](const pubsub_publisher_t *, Properties&&) { std::cout << "success\n"; })
                    .setRequired(true)
                    .build();
        }
        act->promise.set_value();
    }

    int init() {
        return CELIX_SUCCESS;
    }

    int start() {
        return CELIX_SUCCESS;
    }

    int stop() {
        return CELIX_SUCCESS;
    }

    int deinit() {
        return CELIX_SUCCESS;
    }

    activator *act{nullptr};
};

TEST_F(DeadlockTestSuite, test) {

    Component<DependencyCmp>& cmp = mng->createComponent<DependencyCmp>()
            .addInterface<IDependency>("1.0.0", {});

    cmp.setCallbacks(&DependencyCmp::init, &DependencyCmp::start, &DependencyCmp::stop, &DependencyCmp::deinit);

    activator act{};
    act.ctx = ctx;
    act.mng = mng;
    act.cmpUUID = cmp.getUUID();
    cmp.getInstance().act = &act;

    cmp.createCServiceDependency<pubsub_publisher_t>(PUBSUB_PUBLISHER_SERVICE_NAME)
            .setVersionRange("[3.0.0,4)")
            .setFilter("(topic=deadlock)(scope=scope)")
            .setStrategy(celix::dm::DependencyUpdateStrategy::suspend)
            .setCallbacks([&cmp](const pubsub_publisher_t *pub, Properties&&) { cmp.getInstance().setPublisher(pub); })
            .setRequired(true);

    ASSERT_TRUE(cmp.isValid());
    mng->start();

    celix_bundleContext_startBundle(ctx, sutBundleId);

    //wait till setPublisher has added an service dependency. If not the findComponent can return NULL
    act.promise.get_future().wait();
    mng->stop();
}
