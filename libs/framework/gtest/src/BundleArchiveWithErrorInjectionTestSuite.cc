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
#include "celix_utils_ei.h"

//including private headers, which should only be used for testing
#include "bundle_archive_private.h"
#include "bundle_revision_private.h"

class BundleArchiveWithErrorInjectionTestSuite : public ::testing::Test {
public:
    BundleArchiveWithErrorInjectionTestSuite() = default;

    ~BundleArchiveWithErrorInjectionTestSuite() override {
        teardownErrorInjectors();
    }

    void teardownErrorInjectors() {
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_ei_expect_malloc(nullptr, 0, nullptr);
        celix_ei_expect_celix_utils_strdup(nullptr, 0, nullptr);
    }

    void installBundleAndExpectFailure(celix::BundleContext& ctx) {
        long bndId = ctx.installBundle(SIMPLE_TEST_BUNDLE1_LOCATION);
        EXPECT_LT(bndId, 0);
    }
};


TEST_F(BundleArchiveWithErrorInjectionTestSuite, BundleArchiveCreatedFailedTest) {
    auto fw = celix::createFramework({
         {"CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "trace"},
         {CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, "true"}
    });
    auto ctx = fw->getFrameworkBundleContext();

    teardownErrorInjectors();
    //Given a mocked calloc which returns NULL from a (indirect) call from bundleArchive_create
    celix_ei_expect_calloc((void*)bundleArchive_create, 1, nullptr);
    //When I create bundle from a bundle zip by installing a bundle
    //Then the bundle install fails, and the bundle id is < 0
    installBundleAndExpectFailure(*ctx);


    teardownErrorInjectors();
    //Given a mocked celix_utils_strdup which returns NULL from a (indirect) call from bundleArchive_create
    celix_ei_expect_celix_utils_strdup((void*)bundleArchive_create, 1, nullptr);
    //When I create bundle from a bundle zip by installing a bundle
    //Then the bundle install fails, and the bundle id is < 0
    installBundleAndExpectFailure(*ctx);

    teardownErrorInjectors();
    //Given a mocked malloc which returns NULL from a call from manifest_create
    celix_ei_expect_malloc((void*)manifest_create, 0, nullptr);
    //When I create bundle from a bundle zip by installing a bundle
    //Then the bundle install fails, and the bundle id is < 0
    installBundleAndExpectFailure(*ctx);

    teardownErrorInjectors();
    //Given a mocked calloc which returns NULL from a call from bundleRevision_create
    celix_ei_expect_calloc((void*)bundleRevision_create, 0, nullptr);
    //When I create bundle from a bundle zip by installing a bundle
    //Then the bundle install fails, and the bundle id is < 0
    installBundleAndExpectFailure(*ctx);

    teardownErrorInjectors();
    //Given a mocked celix_utils_strdup which returns NULL from a call from bundleRevision_create
    celix_ei_expect_celix_utils_strdup((void*)bundleRevision_create, 0, nullptr);
    //When I create bundle from a bundle zip by installing a bundle
    //Then the bundle install fails, and the bundle id is < 0
    installBundleAndExpectFailure(*ctx);

    //TODO inject errors for celix_bundleArchive_createCacheDirectory
}
