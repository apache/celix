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

#include "celix/FrameworkFactory.h"
#include "celix_constants.h"
#include "celix_framework_utils.h"
#include "malloc_ei.h"

//including private headers, which should only be used for testing
#include "bundle_archive_private.h"

class BundleArchiveWithErrorInjectionTestSuite : public ::testing::Test {
public:
    BundleArchiveWithErrorInjectionTestSuite() = default;

    ~BundleArchiveWithErrorInjectionTestSuite() override {
        celix_ei_expect_calloc(nullptr, 0, nullptr);
    }

};
void* callocSomeMem();
void* callocSomeMem() {
    celix_ei_expect_calloc((void*)callocSomeMem, 0, nullptr);
    return calloc(1, 12);
}

TEST_F(BundleArchiveWithErrorInjectionTestSuite, WrapCallocTest) {
    //note this triggers, but not the calloc/malloc in libcelix_framework?
    void* mem = callocSomeMem();
    EXPECT_EQ(mem, nullptr);
}

TEST_F(BundleArchiveWithErrorInjectionTestSuite, BundleArchiveCreatedFailedTest) {
    auto fw = celix::createFramework({
         {"CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "trace"},
         {CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, "true"}
    });
    auto ctx = fw->getFrameworkBundleContext();

    //Given a mocked calloc which returns NULL from a call from bundleArchive_create
    celix_ei_expect_calloc((void*)bundleArchive_create, 1, nullptr);

    //When  I create bundle from a bundle zip by installing a bundle
    long bndId = ctx->installBundle(SIMPLE_TEST_BUNDLE1_LOCATION);
    EXPECT_LT(bndId, 0);
}
