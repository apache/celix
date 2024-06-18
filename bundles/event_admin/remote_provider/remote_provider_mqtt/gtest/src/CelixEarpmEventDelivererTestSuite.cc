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
#include "celix_earpm_event_deliverer.h"
#include "CelixEarpmTestSuiteBaseClass.h"
#include "celix_log_helper.h"

class CelixEarpmEventDelivererTestSuite : public CelixEarpmTestSuiteBaseClass {
public:
    CelixEarpmEventDelivererTestSuite() : CelixEarpmTestSuiteBaseClass{".earpm_event_deliverer_test_cache"} {
        logHelper = std::shared_ptr <celix_log_helper_t>
                {celix_logHelper_create(ctx.get(), "EventDeliverer"), [](celix_log_helper_t *h) { celix_logHelper_destroy(h); }};
    }

    ~CelixEarpmEventDelivererTestSuite() override = default;

    void TestEventDeliverer(std::function<void (celix_earpm_event_deliverer_t*)> testBody) {
        auto* deliverer = celix_earpmd_create(ctx.get(), logHelper.get());
        ASSERT_NE(deliverer, nullptr);
        testBody(deliverer);
        celix_earpmd_destroy(deliverer);
    }

    std::shared_ptr<celix_log_helper_t> logHelper{};
};

TEST_F(CelixEarpmEventDelivererTestSuite, CreateEventDelivererTest) {
    auto* deliverer = celix_earpmd_create(ctx.get(), logHelper.get());
    ASSERT_NE(deliverer, nullptr);
    celix_earpmd_destroy(deliverer);
}

TEST_F(CelixEarpmEventDelivererTestSuite, SetEventAdminServiceTest) {
    TestEventDeliverer([](celix_earpm_event_deliverer_t *deliverer) {
        celix_event_admin_service_t eventAdminService = {
                .handle = nullptr,
                .postEvent = [](void *handle, const char *topic, const celix_properties_t *props) {
                    (void) handle;
                    (void) topic;
                    (void) props;
                    return CELIX_SUCCESS;
                },
                .sendEvent = [](void *handle, const char *topic, const celix_properties_t *props) {
                    (void) handle;
                    (void) topic;
                    (void) props;
                    return CELIX_SUCCESS;
                }
        };
        auto status = celix_earpmd_setEventAdminSvc(deliverer, &eventAdminService);
        EXPECT_EQ(status, CELIX_SUCCESS);
    });
}

TEST_F(CelixEarpmEventDelivererTestSuite, PostEventTest) {
    TestEventDeliverer([](celix_earpm_event_deliverer_t *deliverer) {
        celix_event_admin_service_t eventAdminService = {
                .handle = nullptr,
                .postEvent = [](void *handle, const char *topic, const celix_properties_t *props) {
                    (void) handle;
                    EXPECT_STREQ(topic, "test/topic");
                    EXPECT_STREQ(celix_properties_get(props, "key", ""), "value");
                    return CELIX_SUCCESS;
                },
                .sendEvent = [](void *handle, const char *topic, const celix_properties_t *props) {
                    (void) handle;
                    (void) topic;
                    (void) props;
                    return CELIX_SUCCESS;
                }
        };
        auto status = celix_earpmd_setEventAdminSvc(deliverer, &eventAdminService);
        EXPECT_EQ(status, CELIX_SUCCESS);

        celix_properties_t *props = celix_properties_create();
        celix_properties_set(props, "key", "value");
        status = celix_earpmd_postEvent(deliverer, "test/topic", props);
        EXPECT_EQ(status, CELIX_SUCCESS);
    });
}

TEST_F(CelixEarpmEventDelivererTestSuite, EventAdminPostEventFailedTest) {
    TestEventDeliverer([](celix_earpm_event_deliverer_t *deliverer) {
        celix_event_admin_service_t eventAdminService = {
                .handle = nullptr,
                .postEvent = [](void *handle, const char *topic, const celix_properties_t *props) {
                    (void) handle;
                    (void) topic;
                    (void) props;
                    return CELIX_ILLEGAL_STATE;
                },
                .sendEvent = [](void *handle, const char *topic, const celix_properties_t *props) {
                    (void) handle;
                    (void) topic;
                    (void) props;
                    return CELIX_SUCCESS;
                }
        };
        auto status = celix_earpmd_setEventAdminSvc(deliverer, &eventAdminService);
        EXPECT_EQ(status, CELIX_SUCCESS);

        celix_properties_t *props = celix_properties_create();
        celix_properties_set(props, "key", "value");
        status = celix_earpmd_postEvent(deliverer, "test/topic", props);
        EXPECT_EQ(status, CELIX_ILLEGAL_STATE);
    });
}

TEST_F(CelixEarpmEventDelivererTestSuite, PostEventWithoutEventAdminServiceTest) {
    TestEventDeliverer([](celix_earpm_event_deliverer_t *deliverer) {
        celix_properties_t *props = celix_properties_create();
        celix_properties_set(props, "key", "value");
        celix_earpmd_postEvent(deliverer, "test/topic", props);
    });
}

TEST_F(CelixEarpmEventDelivererTestSuite, SendEventTest) {
    TestEventDeliverer([](celix_earpm_event_deliverer_t *deliverer) {
        celix_event_admin_service_t eventAdminService = {
                .handle = nullptr,
                .postEvent = [](void *handle, const char *topic, const celix_properties_t *props) {
                    (void) handle;
                    (void) topic;
                    (void) props;
                    return CELIX_SUCCESS;
                },
                .sendEvent = [](void *handle, const char *topic, const celix_properties_t *props) {
                    (void) handle;
                    EXPECT_STREQ(topic, "test/topic");
                    EXPECT_STREQ(celix_properties_get(props, "key", ""), "value");
                    return CELIX_SUCCESS;
                }
        };
        auto status = celix_earpmd_setEventAdminSvc(deliverer, &eventAdminService);
        EXPECT_EQ(status, CELIX_SUCCESS);

        celix_properties_t *props = celix_properties_create();
        celix_properties_set(props, "key", "value");
        std::promise<celix_status_t> promise;
        auto future = promise.get_future();
        celix_earpmd_sendEvent(deliverer, "test/topic", props, [](void *data, const char *topic, celix_status_t status) {
            auto promise = static_cast<std::promise<celix_status_t> *>(data);
            EXPECT_STREQ(topic, "test/topic");
            promise->set_value(status);
        }, &promise);
        auto rc = future.wait_for(std::chrono::milliseconds(30000));
        ASSERT_EQ(rc, std::future_status::ready);
        status = future.get();
        EXPECT_EQ(status, CELIX_SUCCESS);
    });
}

TEST_F(CelixEarpmEventDelivererTestSuite, EventAdminSendEventFailedTest) {
    TestEventDeliverer([](celix_earpm_event_deliverer_t *deliverer) {
        celix_event_admin_service_t eventAdminService = {
                .handle = nullptr,
                .postEvent = [](void *handle, const char *topic, const celix_properties_t *props) {
                    (void) handle;
                    (void) topic;
                    (void) props;
                    return CELIX_SUCCESS;
                },
                .sendEvent = [](void *handle, const char *topic, const celix_properties_t *props) {
                    (void) handle;
                    (void) topic;
                    (void) props;
                    return CELIX_ILLEGAL_STATE;
                }
        };
        auto status = celix_earpmd_setEventAdminSvc(deliverer, &eventAdminService);
        EXPECT_EQ(status, CELIX_SUCCESS);

        celix_properties_t *props = celix_properties_create();
        celix_properties_set(props, "key", "value");
        std::promise<celix_status_t> promise;
        auto future = promise.get_future();
        celix_earpmd_sendEvent(deliverer, "test/topic", props, [](void *data, const char *topic, celix_status_t status) {
            auto promise = static_cast<std::promise<celix_status_t> *>(data);
            (void) topic;
            promise->set_value(status);
        }, &promise);
        auto rc = future.wait_for(std::chrono::milliseconds(30000));
        ASSERT_EQ(rc, std::future_status::ready);
        status = future.get();
        EXPECT_EQ(status, CELIX_ILLEGAL_STATE);
    });
}

TEST_F(CelixEarpmEventDelivererTestSuite, SendEventWithoutEventAdminServiceTest) {
    TestEventDeliverer([](celix_earpm_event_deliverer_t *deliverer) {
        celix_properties_t *props = celix_properties_create();
        celix_properties_set(props, "key", "value");
        std::promise<celix_status_t> promise;
        auto future = promise.get_future();
        celix_earpmd_sendEvent(deliverer, "test/topic", props, [](void *data, const char *topic, celix_status_t status) {
            auto promise = static_cast<std::promise<celix_status_t> *>(data);
            (void) topic;
            promise->set_value(status);
        }, &promise);
        auto rc = future.wait_for(std::chrono::milliseconds(30000));
        ASSERT_EQ(rc, std::future_status::ready);
        auto status = future.get();
        EXPECT_EQ(status, CELIX_ILLEGAL_STATE);
    });
}