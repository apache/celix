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
#include <string>
#include <thread>
#include <celix_log_sink.h>
#include <celix_log_control.h>

#include "celix_bundle_context.h"
#include "celix_framework_factory.h"
#include "celix_log_service.h"
#include "celix_log_constants.h"
#include "celix_constants.h"

class SyslogWriterTestSuite : public ::testing::Test {
public:
    SyslogWriterTestSuite() {
        auto* properties = celix_properties_create();
        celix_properties_set(properties, CELIX_FRAMEWORK_CACHE_DIR, ".cacheSyslogWriterTestSuite");
        celix_properties_set(properties, CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL_CONFIG_NAME, "trace");


        auto* fwPtr = celix_frameworkFactory_createFramework(properties);
        auto* ctxPtr = celix_framework_getFrameworkContext(fwPtr);
        fw = std::shared_ptr<celix_framework_t>{fwPtr, [](celix_framework_t* f) {celix_frameworkFactory_destroyFramework(f);}};
        ctx = std::shared_ptr<celix_bundle_context_t>{ctxPtr, [](celix_bundle_context_t*){/*nop*/}};

        long bndId1 = celix_bundleContext_installBundle(ctx.get(), LOG_ADMIN_BUNDLE, true);
        EXPECT_TRUE(bndId1 >= 0);

        long bndId2 = celix_bundleContext_installBundle(ctx.get(), SYSLOG_WRITER_BUNDLE, true);
        EXPECT_TRUE(bndId2 >= 0);
    }


    long bndId = -1L;
    std::shared_ptr<celix_framework_t> fw{nullptr};
    std::shared_ptr<celix_bundle_context_t> ctx{nullptr};
};

TEST_F(SyslogWriterTestSuite, StartStop) {
    auto *list = celix_bundleContext_listBundles(ctx.get());
    EXPECT_EQ(2, celix_arrayList_size(list));
    celix_arrayList_destroy(list);
}

TEST_F(SyslogWriterTestSuite, LogToSysLog) {
    {
        celix_service_use_options_t opts{};
        opts.filter.serviceName = CELIX_LOG_CONTROL_NAME;
        opts.filter.versionRange = CELIX_LOG_CONTROL_USE_RANGE;
        opts.callbackHandle = nullptr;
        opts.use = [](void*, void *svc) {
            auto *lc = static_cast<celix_log_control_t*>(svc);
            EXPECT_EQ(1, lc->nrOfSinks(lc->handle, nullptr));
        };
        bool called = celix_bundleContext_useServiceWithOptions(ctx.get(), &opts);
        EXPECT_TRUE(called);
    }
    {
        celix_service_use_options_t opts{};
        opts.filter.serviceName = CELIX_LOG_SERVICE_NAME;
        opts.filter.versionRange = CELIX_LOG_SERVICE_USE_RANGE;
        opts.callbackHandle = nullptr;
        opts.use = [](void*, void *svc) {
            auto *ls = static_cast<celix_log_service_t*>(svc);
            ls->trace(ls->handle, "test %i %i %i", 1, 2, 3);
            ls->debug(ls->handle, "test %i %i %i", 1, 2, 3);
            ls->info(ls->handle, "test %i %i %i", 1, 2, 3);
            ls->warning(ls->handle, "test %i %i %i", 1, 2, 3);
            ls->error(ls->handle, "test %i %i %i", 1, 2, 3);

            //a very long log message
            std::string buf(2047, 'A');
            ls->fatal(ls->handle, "%s", buf.c_str());
        };
        bool called = celix_bundleContext_useServiceWithOptions(ctx.get(), &opts);
        EXPECT_TRUE(called);
    }

}

