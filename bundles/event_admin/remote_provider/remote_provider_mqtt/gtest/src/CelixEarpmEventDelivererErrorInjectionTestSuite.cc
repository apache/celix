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
#include <future>
#include <functional>

#include "celix_log_helper.h"
#include "malloc_ei.h"
#include "celix_threads_ei.h"
#include "celix_array_list_ei.h"
#include "celix_utils_ei.h"
#include "celix_earpm_event_deliverer.h"
#include "CelixEarpmTestSuiteBaseClass.h"


class CelixEarpmEventDelivererErrorInjectionTestSuite : public CelixEarpmTestSuiteBaseClass {
public:
    CelixEarpmEventDelivererErrorInjectionTestSuite() : CelixEarpmTestSuiteBaseClass{".earpm_event_deliverer_ej_test_cache"} {
        logHelper = std::shared_ptr <celix_log_helper_t>
                {celix_logHelper_create(ctx.get(), "EventDeliverer"), [](celix_log_helper_t *h) { celix_logHelper_destroy(h); }};
    }

    ~CelixEarpmEventDelivererErrorInjectionTestSuite() override {
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_ei_expect_celixThreadRwlock_create(nullptr, 0, 0);
        celix_ei_expect_celixThreadMutex_create(nullptr, 0, 0);
        celix_ei_expect_celixThreadCondition_init(nullptr, 0, 0);
        celix_ei_expect_celix_arrayList_createPointerArray(nullptr, 0, nullptr);
        celix_ei_expect_celixThread_create(nullptr, 0, 0);
        celix_ei_expect_celix_utils_strdup(nullptr, 0, nullptr);
    };

    std::shared_ptr<celix_log_helper_t> logHelper{};
};

TEST_F(CelixEarpmEventDelivererErrorInjectionTestSuite, FailedToAllocMemoryForEventDelivererTest) {
    celix_ei_expect_calloc((void*)&celix_earpmDeliverer_create, 0, nullptr);

    celix_earpm_event_deliverer_t *deliverer = celix_earpmDeliverer_create(ctx.get(), logHelper.get());
    ASSERT_EQ(nullptr, deliverer);
}

TEST_F(CelixEarpmEventDelivererErrorInjectionTestSuite, FailedToCreateEventAdminLockTest) {
    celix_ei_expect_celixThreadRwlock_create((void*)&celix_earpmDeliverer_create, 0, ENOMEM);
    celix_earpm_event_deliverer_t *deliverer = celix_earpmDeliverer_create(ctx.get(), logHelper.get());
    ASSERT_EQ(nullptr, deliverer);
}

TEST_F(CelixEarpmEventDelivererErrorInjectionTestSuite, FailedToCreateMutexTest) {
    celix_ei_expect_celixThreadMutex_create((void*)&celix_earpmDeliverer_create, 0, ENOMEM);
    celix_earpm_event_deliverer_t *deliverer = celix_earpmDeliverer_create(ctx.get(), logHelper.get());
    ASSERT_EQ(nullptr, deliverer);
}

TEST_F(CelixEarpmEventDelivererErrorInjectionTestSuite, FailedToCreateSyncEventConditionTest) {
    celix_ei_expect_celixThreadCondition_init((void*)&celix_earpmDeliverer_create, 0, ENOMEM);
    celix_earpm_event_deliverer_t *deliverer = celix_earpmDeliverer_create(ctx.get(), logHelper.get());
    ASSERT_EQ(nullptr, deliverer);
}

TEST_F(CelixEarpmEventDelivererErrorInjectionTestSuite, FailedToCreateSyncEventQueueTest) {
    celix_ei_expect_celix_arrayList_createPointerArray((void*)&celix_earpmDeliverer_create, 0, nullptr);
    celix_earpm_event_deliverer_t *deliverer = celix_earpmDeliverer_create(ctx.get(), logHelper.get());
    ASSERT_EQ(nullptr, deliverer);
}

TEST_F(CelixEarpmEventDelivererErrorInjectionTestSuite, FailedToCreateThreadTest) {
    celix_ei_expect_celixThread_create((void*)&celix_earpmDeliverer_create, 0, ENOMEM);
    celix_earpm_event_deliverer_t *deliverer = celix_earpmDeliverer_create(ctx.get(), logHelper.get());
    ASSERT_EQ(nullptr, deliverer);

    celix_ei_expect_celixThread_create((void*)&celix_earpmDeliverer_create, 0, ENOMEM, 2);
    deliverer = celix_earpmDeliverer_create(ctx.get(), logHelper.get());
    ASSERT_EQ(nullptr, deliverer);
}

TEST_F(CelixEarpmEventDelivererErrorInjectionTestSuite, FailedToAllocMemoryForSyncEventTest) {
    celix_earpm_event_deliverer_t *deliverer = celix_earpmDeliverer_create(ctx.get(), logHelper.get());
    ASSERT_NE(nullptr, deliverer);

    celix_ei_expect_calloc((void*)&celix_earpmDeliverer_sendEvent, 0, nullptr);
    auto status = celix_earpmDeliverer_sendEvent(deliverer, "test/event", nullptr, nullptr, nullptr);
    ASSERT_EQ(ENOMEM, status);

    celix_earpmDeliverer_destroy(deliverer);
}

TEST_F(CelixEarpmEventDelivererErrorInjectionTestSuite, FailedToDupSyncEventTopicTest) {
    celix_earpm_event_deliverer_t *deliverer = celix_earpmDeliverer_create(ctx.get(), logHelper.get());
    ASSERT_NE(nullptr, deliverer);

    celix_ei_expect_celix_utils_strdup((void*)&celix_earpmDeliverer_sendEvent, 0, nullptr);
    auto status = celix_earpmDeliverer_sendEvent(deliverer, "test/event", nullptr, nullptr, nullptr);
    ASSERT_EQ(ENOMEM, status);

    celix_earpmDeliverer_destroy(deliverer);
}

TEST_F(CelixEarpmEventDelivererErrorInjectionTestSuite, FailedToEnqueueSyncEventTest) {
    celix_earpm_event_deliverer_t *deliverer = celix_earpmDeliverer_create(ctx.get(), logHelper.get());
    ASSERT_NE(nullptr, deliverer);

    celix_ei_expect_celix_arrayList_add((void*)&celix_earpmDeliverer_sendEvent, 0, ENOMEM);
    auto status = celix_earpmDeliverer_sendEvent(deliverer, "test/event", nullptr, nullptr, nullptr);
    ASSERT_EQ(ENOMEM, status);

    celix_earpmDeliverer_destroy(deliverer);
}