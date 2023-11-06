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

#include "celix/FrameworkFactory.h"
#include "celix/BundleContext.h"
#include "celix_log_service.h"
#include "celix_log_constants.h"
#include "celix_log_helper.h"
#include "celix/LogHelper.h"
#include "celix_err.h"

class LogHelperTestSuite : public ::testing::Test {
public:
    LogHelperTestSuite() {
        celix::Properties properties{};
        properties.set(CELIX_FRAMEWORK_CACHE_DIR, ".cacheLogHelperTestSuite");
        properties.set(CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL_CONFIG_NAME, "debug");
        fw = celix::createFramework(properties);
        ctx = fw->getFrameworkBundleContext();
    }

    std::shared_ptr<celix::Framework> fw{};
    std::shared_ptr<celix::BundleContext> ctx{};
};



TEST_F(LogHelperTestSuite, LogToStdOut) {
    auto *helper = celix_logHelper_create(ctx->getCBundleContext(), "test::Log");
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
    auto *helper = celix_logHelper_create(ctx->getCBundleContext(), "test::Log");
    EXPECT_EQ(0, celix_logHelper_logCount(helper));

    std::atomic<size_t> logCount{0};
    celix_log_service_t logSvc;
    logSvc.handle = (void*)&logCount;
    logSvc.vlogDetails= [](void *handle, celix_log_level_e, const char*, const char*, int, const char *format, va_list formatArgs) {
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
    long svcId = celix_bundleContext_registerServiceWithOptions(ctx->getCBundleContext(), &opts);

    EXPECT_EQ(0, celix_logHelper_logCount(helper));
    EXPECT_EQ(0, logCount.load());
    celix_logHelper_trace(helper, "testing %i", 0); //not not active
    celix_logHelper_debug(helper, "testing %i", 1);
    celix_logHelper_info(helper, "testing %i", 2);
    celix_logHelper_warning(helper, "testing %i", 3);
    celix_logHelper_error(helper, "testing %i", 4);
    celix_logHelper_fatal(helper, "testing %i", 5);
    celix_logHelper_logDetails(helper, CELIX_LOG_LEVEL_ERROR, __FILE__, __FUNCTION__, __LINE__, "testing %i", 0);
    celix_logHelper_log(helper, CELIX_LOG_LEVEL_DISABLED, "testing %i", 0);
    EXPECT_EQ(6, celix_logHelper_logCount(helper));
    EXPECT_EQ(6, logCount.load());

    celix_bundleContext_unregisterService(ctx->getCBundleContext(), svcId);
    celix_logHelper_destroy(helper);
}

TEST_F(LogHelperTestSuite, LogTssErrors) {
    auto *helper = celix_logHelper_create(ctx->getCBundleContext(), "test::Log");
    EXPECT_EQ(0, celix_logHelper_logCount(helper));

    char *buf = nullptr;
    celix_log_service_t logSvc;
    logSvc.handle = (void*)&buf;
    logSvc.vlogDetails= [](void *handle, celix_log_level_e, const char*, const char*, int, const char *format, va_list formatArgs) {
        auto **b = static_cast<char **>(handle);
        vasprintf(b, format, formatArgs);
        EXPECT_NE(nullptr, *b);
    };
    auto* props = celix_properties_create();
    celix_properties_set(props, CELIX_LOG_SERVICE_PROPERTY_NAME, "test::Log");
    celix_service_registration_options_t opts{};
    opts.serviceName = CELIX_LOG_SERVICE_NAME;
    opts.serviceVersion = CELIX_LOG_SERVICE_VERSION;
    opts.properties = props;
    opts.svc = (void*)&logSvc;
    long svcId = celix_bundleContext_registerServiceWithOptions(ctx->getCBundleContext(), &opts);

    //celix error messages is empty
    celix_logHelper_logTssErrors(helper, CELIX_LOG_LEVEL_ERROR);
    EXPECT_EQ(0, celix_logHelper_logCount(helper));

    //Log celix error messages
    int logCnt = 0;
    for (logCnt = 0; logCnt < 5; ++logCnt) {
        celix_err_pushf("celix error message%d", logCnt);
    }

    //Print celix error messages
    celix_logHelper_logTssErrors(helper, CELIX_LOG_LEVEL_ERROR);
    EXPECT_EQ(1, celix_logHelper_logCount(helper));

    //Check celix error message format
    char *p = buf;
    char data[32]={0};
    sscanf(p,"%[^\n]",data);
    EXPECT_STREQ(data, "Detected tss errors:");
    p = p+strlen(data)+1;//skip '\n'
    while(logCnt--)
    {
        sscanf(p,"%[^\n]",data);
        char expected[64];
        sprintf(expected, "[TssErr] celix error message%d", logCnt);
        EXPECT_STREQ(data, expected);

        p = p+strlen(data)+1;//skip '\n'
    }
    free(buf);

    celix_bundleContext_unregisterService(ctx->getCBundleContext(), svcId);
    celix_logHelper_destroy(helper);
}

TEST_F(LogHelperTestSuite, TssErrorsAlwaysIncludeNulTerminated) {
    auto *helper = celix_logHelper_create(ctx->getCBundleContext(), "test::Log");
    EXPECT_EQ(0, celix_logHelper_logCount(helper));

    char *buf = nullptr;
    celix_log_service_t logSvc;
    logSvc.handle = (void*)&buf;
    logSvc.vlogDetails= [](void *handle, celix_log_level_e, const char*, const char*, int, const char *format, va_list formatArgs) {
        auto **b = static_cast<char **>(handle);
        vasprintf(b, format, formatArgs);
        EXPECT_NE(nullptr, *b);
    };
    auto* props = celix_properties_create();
    celix_properties_set(props, CELIX_LOG_SERVICE_PROPERTY_NAME, "test::Log");
    celix_service_registration_options_t opts{};
    opts.serviceName = CELIX_LOG_SERVICE_NAME;
    opts.serviceVersion = CELIX_LOG_SERVICE_VERSION;
    opts.properties = props;
    opts.svc = (void*)&logSvc;
    long svcId = celix_bundleContext_registerServiceWithOptions(ctx->getCBundleContext(), &opts);

    //celix error messages total length is greater than 512
    int logCnt = 0;
    for (logCnt = 0; logCnt < 64; ++logCnt) {
        celix_err_pushf("celix error message%d", logCnt);
    }

    //Print celix error messages
    celix_logHelper_logTssErrors(helper, CELIX_LOG_LEVEL_ERROR);
    EXPECT_LT(strlen(buf)-(sizeof("Detected tss errors:\n") - 1), 512);
    free(buf);

    celix_bundleContext_unregisterService(ctx->getCBundleContext(), svcId);
    celix_logHelper_destroy(helper);
}

TEST_F(LogHelperTestSuite, CxxLogHelper) {
    celix::LogHelper helper{ctx, "test name"};

    EXPECT_EQ(0, helper.count());

    helper.trace("testing %i", 0); //not not active
    helper.debug("testing %i", 1); //not not active
    helper.info("testing %i", 2); //not not active
    helper.warning("testing %i", 3); //not not active
    helper.error("testing %i", 4); //not not active
    helper.fatal("testing %i", 5); //not not active
    celix_err_pushf("celix error message");
    helper.logTssErrors(CELIX_LOG_LEVEL_ERROR);
    EXPECT_EQ(6, helper.count());
}

TEST_F(LogHelperTestSuite, CLogHelperAutoCleanup) {
    celix_autoptr(celix_log_helper_t) helper = celix_logHelper_create(ctx->getCBundleContext(), "test::Log");
    celix_logHelper_info(helper, "Auto cleanup test");
}