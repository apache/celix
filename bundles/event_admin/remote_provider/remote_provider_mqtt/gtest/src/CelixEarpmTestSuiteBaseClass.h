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

#ifndef CELIX_EARPM_TEST_SUITE_BASE_CLASS_H
#define CELIX_EARPM_TEST_SUITE_BASE_CLASS_H

#include <mutex>
#include <condition_variable>
#include <vector>
#include <cstdio>
#include <algorithm>
#include <gtest/gtest.h>

#include "celix_bundle_context.h"
#include "celix_framework_factory.h"
#include "celix_constants.h"
#include "celix_log_constants.h"
#include "celix_log_service.h"
#include "celix_log_utils.h"


class CelixEarpmTestSuiteBaseClass : public ::testing::Test {
public:
    CelixEarpmTestSuiteBaseClass() = delete;
    explicit CelixEarpmTestSuiteBaseClass(const char* testCache, const char* logServiceName = "celix_earpm") {
        auto props = celix_properties_create();
        celix_properties_set(props, CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, "true");
        celix_properties_set(props, CELIX_FRAMEWORK_CACHE_DIR, testCache);
        celix_properties_set(props, CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL_CONFIG_NAME, "trace");
        auto fwPtr = celix_frameworkFactory_createFramework(props);
        fw = std::shared_ptr < celix_framework_t >
             {fwPtr, [](celix_framework_t *f) { celix_frameworkFactory_destroyFramework(f); }};
        ctx = std::shared_ptr < celix_bundle_context_t >
              {celix_framework_getFrameworkContext(fw.get()), [](celix_bundle_context_t *) {/*nop*/}};
        fwUUID = celix_bundleContext_getProperty(ctx.get(), CELIX_FRAMEWORK_UUID, "");
        logServiceName_ = logServiceName;
        logService.handle = this;
        logService.vlogDetails = [](void *handle, celix_log_level_e level, const char* file, const char* function,
                int line, const char* format, va_list formatArgs) {
            auto self = static_cast<CelixEarpmTestSuiteBaseClass *>(handle);
            char log[1024]{0};
            va_list formatArgsCopy;
            va_copy(formatArgsCopy, formatArgs);
            vsnprintf(log, sizeof(log), format, formatArgsCopy);
            celix_logUtils_vLogToStdoutDetails(self->logServiceName_.c_str(), level, file, function, line, format, formatArgs);
            std::lock_guard<std::mutex> lockGuard{self->logMessagesMutex};
            self->logMessages.emplace_back(log);
            self->logMessagesCond.notify_all();
        };
        celix_service_registration_options_t opts{};
        opts.svc = &logService;
        opts.serviceName = CELIX_LOG_SERVICE_NAME;
        opts.serviceVersion = CELIX_LOG_SERVICE_VERSION;
        opts.properties = celix_properties_create();
        EXPECT_NE(nullptr, opts.properties);
        celix_properties_set(opts.properties, CELIX_LOG_SERVICE_PROPERTY_NAME, logServiceName);
        logServiceId = celix_bundleContext_registerServiceWithOptions(ctx.get(), &opts);
        EXPECT_LE(0, logServiceId);
    }

    ~CelixEarpmTestSuiteBaseClass() override {
        celix_bundleContext_unregisterService(ctx.get(), logServiceId);
    }

    auto WaitForLogMessage(const std::string &msg, int timeoutInMs = 30000) {
        std::unique_lock<std::mutex> lock{logMessagesMutex};
        return logMessagesCond.wait_for(lock, std::chrono::milliseconds{timeoutInMs}, [&] {
            return std::find_if(logMessages.rbegin(), logMessages.rend(), [msg](const std::string &m) {
                return m.find(msg) != std::string::npos;
            }) != logMessages.rend();
        });
    }

    std::shared_ptr<celix_framework_t> fw{};
    std::shared_ptr<celix_bundle_context_t> ctx{};
    std::string fwUUID{};
    std::string logServiceName_{};
    celix_log_service_t logService{};
    long logServiceId{-1};
    std::vector<std::string> logMessages{};
    std::mutex logMessagesMutex{};
    std::condition_variable logMessagesCond{};
};

#endif //CELIX_EARPM_TEST_SUITE_BASE_CLASS_H
