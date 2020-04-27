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
#include <atomic>

#include "celix_log_service.h"
#include "celix_log_constants.h"
#include "celix_api.h"
#include "celix_log_helper.h"

class LogHelperTestSuite : public ::testing::Test {
public:
    LogHelperTestSuite() {
        auto* properties = celix_properties_create();
        celix_properties_set(properties, "org.osgi.framework.storage", ".cacheLogHelperTestSuite");
        celix_properties_set(properties, CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL_CONFIG_NAME, "debug");


        auto* fwPtr = celix_frameworkFactory_createFramework(properties);
        auto* ctxPtr = celix_framework_getFrameworkContext(fwPtr);
        fw = std::shared_ptr<celix_framework_t>{fwPtr, [](celix_framework_t* f) {celix_frameworkFactory_destroyFramework(f);}};
        ctx = std::shared_ptr<celix_bundle_context_t>{ctxPtr, [](celix_bundle_context_t*){/*nop*/}};
    }

    std::shared_ptr<celix_framework_t> fw{nullptr};
    std::shared_ptr<celix_bundle_context_t> ctx{nullptr};
};



TEST_F(LogHelperTestSuite, LogToStdOut) {
    auto *helper = celix_logHelper_create(ctx.get(), "test::Log");
    EXPECT_EQ(0, celix_logHelper_logCount(helper));

    celix_logHelper_trace(helper, "testing %i", 0); //not not active
    celix_logHelper_debug(helper, "testing %i", 1);
    celix_logHelper_info(helper, "testing %i", 2);
    celix_logHelper_warning(helper, "testing %i", 3);
    celix_logHelper_error(helper, "testing %i", 4);
    celix_logHelper_fatal(helper, "testing %i", 5);
    EXPECT_EQ(5, celix_logHelper_logCount(helper));

    celix_logHelper_destroy(helper);
}

TEST_F(LogHelperTestSuite, LogToLogSvc) {
    auto *helper = celix_logHelper_create(ctx.get(), "test::Log");
    EXPECT_EQ(0, celix_logHelper_logCount(helper));

    std::atomic<size_t> logCount{0};
    celix_log_service_t logSvc;
    logSvc.handle = (void*)&logCount;
    logSvc.vlog = [](void *handle, celix_log_level_e, const char *format, va_list formatArgs) {
        auto* c = static_cast<std::atomic<size_t>*>(handle);
        c->fetch_add(1);
        vfprintf(stderr, format, formatArgs);
        fprintf(stderr, "\n");
    };

    auto* props = celix_properties_create();
    celix_properties_set(props, CELIX_LOG_SERVICE_PROPERTY_NAME, "test::Log");
    celix_service_registration_options_t opts{};
    opts.serviceName = CELIX_LOG_SERVICE_NAME;
    opts.serviceVersion = CELIX_LOG_SERVICE_VERSION;
    opts.properties = props;
    opts.svc = (void*)&logSvc;
    long svcId = celix_bundleContext_registerServiceWithOptions(ctx.get(), &opts);

    EXPECT_EQ(0, celix_logHelper_logCount(helper));
    EXPECT_EQ(0, logCount.load());
    celix_logHelper_trace(helper, "testing %i", 0); //not not active
    celix_logHelper_debug(helper, "testing %i", 1);
    celix_logHelper_info(helper, "testing %i", 2);
    celix_logHelper_warning(helper, "testing %i", 3);
    celix_logHelper_error(helper, "testing %i", 4);
    celix_logHelper_fatal(helper, "testing %i", 5);
    celix_logHelper_log(helper, CELIX_LOG_LEVEL_DISABLED, "testing %i", 0);
    celix_logHelper_vlog(helper, CELIX_LOG_LEVEL_DISABLED, "test", nullptr);
    EXPECT_EQ(5, celix_logHelper_logCount(helper));
    EXPECT_EQ(5, logCount.load());

    celix_bundleContext_unregisterService(ctx.get(), svcId);
    celix_logHelper_destroy(helper);
}