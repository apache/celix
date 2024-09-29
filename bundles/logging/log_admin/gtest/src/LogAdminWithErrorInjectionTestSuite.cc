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

#include <gtest/gtest.h>

#include "asprintf_ei.h"
#include "celix/FrameworkFactory.h"
#include "celix/Constants.h"
#include "celix_log_admin.h"
#include "celix_properties_ei.h"
#include "celix_string_hash_map_ei.h"
#include "malloc_ei.h"

#include <celix_log_service.h>

class LogAdminWithErrorInjectionTestSuite : public ::testing::Test {
  public:
    LogAdminWithErrorInjectionTestSuite() {
        fw = celix::createFramework();

        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_ei_expect_asprintf(nullptr, 0, -1);
        celix_ei_expect_celix_stringHashMap_put(nullptr, 0, 0);
        celix_ei_expect_celix_properties_create(nullptr, 0, nullptr);
    }

    ~LogAdminWithErrorInjectionTestSuite() noexcept override {}

    std::shared_ptr<celix::Framework> fw{};
};

TEST_F(LogAdminWithErrorInjectionTestSuite, CreateWithErrorTest) {
    //Given an error injection for calloc
    celix_ei_expect_calloc((void*)celix_logAdmin_create, 0, nullptr);

    //When creating a log admin
    auto* admin = celix_logAdmin_create(nullptr);

    //Then the admin is nullptr
    ASSERT_EQ(nullptr, admin);
}

TEST_F(LogAdminWithErrorInjectionTestSuite, AddLogSvcWithErrorTest) {
    //Given a log admin
    auto* logAdmin = celix_logAdmin_create(fw->getFrameworkBundleContext()->getCBundleContext());
    ASSERT_NE(logAdmin, nullptr);

    //And an error injection for calloc is expected
    celix_ei_expect_asprintf((void*)celix_logAdmin_addLogSvcForName, 0, -1);

    //When adding a log service for a name
    celix_logAdmin_addLogSvcForName(logAdmin, "test");

    //Then the loggers count is not increased
    EXPECT_EQ(0, celix_logAdmin_nrOfLogServices(logAdmin, "test"));

    //When an error injection for calloc is expected
    celix_ei_expect_calloc( (void*)celix_logAdmin_addLogSvcForName, 0, nullptr);

    //And adding a log service for a name
    celix_logAdmin_addLogSvcForName(logAdmin, "test");

    //Then the loggers count is not increased
    EXPECT_EQ(0, celix_logAdmin_nrOfLogServices(logAdmin, "test"));

    //When an error injection for celix_stringHashMap_put is expected
    celix_ei_expect_celix_stringHashMap_put( (void*)celix_logAdmin_addLogSvcForName, 0, ENOMEM);

    //And adding a log service for a name
    celix_logAdmin_addLogSvcForName(logAdmin, "test");

    //Then the loggers count is not increased
    EXPECT_EQ(0, celix_logAdmin_nrOfLogServices(logAdmin, "test"));

    //When an error injection for celix_properties_create is expected
    celix_ei_expect_celix_properties_create( (void*)celix_logAdmin_addLogSvcForName, 0, nullptr);

    //And adding a log service for a name
    celix_logAdmin_addLogSvcForName(logAdmin, "test");

    //Then the loggers count is not increased
    EXPECT_EQ(0, celix_logAdmin_nrOfLogServices(logAdmin, "test"));

    celix_logAdmin_destroy(logAdmin);
}

TEST_F(LogAdminWithErrorInjectionTestSuite, AddLogSinkWithErrorTest) {
    //Given a log admin
    auto* logAdmin = celix_logAdmin_create(fw->getFrameworkBundleContext()->getCBundleContext());
    ASSERT_NE(logAdmin, nullptr);

    //And a service properties
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    celix_properties_set(props, "name", "test");
    celix_properties_setLong(props, celix::SERVICE_ID, 1L);

    //And an error injection for asprintf is expected
    celix_ei_expect_asprintf((void*)celix_logAdmin_addSink, 0, -1);

    //When adding a log sink
    celix_logAdmin_addSink(logAdmin, nullptr, props);

    //Then the sinks count is not increased
    EXPECT_EQ(0, celix_logAdmin_nrOfSinks(logAdmin, "test"));

    //When an error injection for calloc is expected
    celix_ei_expect_calloc( (void*)celix_logAdmin_addSink, 0, nullptr);

    //And adding a log sink
    celix_logAdmin_addSink(logAdmin, nullptr, props);

    //Then the sinks count is not increased
    EXPECT_EQ(0, celix_logAdmin_nrOfSinks(logAdmin, "test"));

    //When an error injection for celix_stringHashMap_put is expected
    celix_ei_expect_celix_stringHashMap_put( (void*)celix_logAdmin_addSink, 0, ENOMEM);

    //And adding a log sink
    celix_logAdmin_addSink(logAdmin, nullptr, props);

    //Then the sinks count is not increased
    EXPECT_EQ(0, celix_logAdmin_nrOfSinks(logAdmin, "test"));

    celix_logAdmin_destroy(logAdmin);
}
