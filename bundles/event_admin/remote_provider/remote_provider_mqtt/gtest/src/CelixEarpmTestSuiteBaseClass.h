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

#include "celix_bundle_context.h"
#include "celix_framework_factory.h"
#include "celix_constants.h"
#include "celix_log_constants.h"
#include <gtest/gtest.h>

class CelixEarpmTestSuiteBaseClass : public ::testing::Test {
public:
    CelixEarpmTestSuiteBaseClass() = delete;
    explicit CelixEarpmTestSuiteBaseClass(const char* testCache) {
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
    }

    virtual ~CelixEarpmTestSuiteBaseClass() = default;

    std::shared_ptr<celix_framework_t> fw{};
    std::shared_ptr<celix_bundle_context_t> ctx{};
    std::string fwUUID{};
};

#endif //CELIX_EARPM_TEST_SUITE_BASE_CLASS_H
