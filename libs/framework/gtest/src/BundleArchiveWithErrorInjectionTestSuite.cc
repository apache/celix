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
#include "asprintf_ei.h"

//extern declarations for testing purposes. Note signatures are not correct, but that is not important for the test.
extern "C" celix_status_t celix_bundleArchive_create(void);
extern "C" celix_status_t manifest_create(void);
extern "C" celix_status_t celix_bundleRevision_create(void);

class BundleArchiveWithErrorInjectionTestSuite : public ::testing::Test {
public:
    BundleArchiveWithErrorInjectionTestSuite() {
        fw = celix::createFramework({
             {"CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "trace"},
             {CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, "true"}
        });
        ctx = fw->getFrameworkBundleContext();
    }

    ~BundleArchiveWithErrorInjectionTestSuite() override {
        teardownErrorInjectors();
        ctx.reset();
        fw.reset();
    }

    void teardownErrorInjectors() {
        celix_ei_expect_asprintf(nullptr, 0, 0);
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_ei_expect_malloc(nullptr, 0, nullptr);
        celix_ei_expect_celix_utils_strdup(nullptr, 0, nullptr);
        celix_ei_expect_celix_utils_createDirectory(nullptr, 0, CELIX_SUCCESS);
        celix_ei_expect_celix_utils_getLastModified(nullptr, 0, CELIX_SUCCESS);
        celix_ei_expect_celix_utils_deleteDirectory(nullptr, 0, CELIX_SUCCESS);
        celix_ei_expect_celix_utils_writeOrCreateString(nullptr, 0, nullptr);
        celix_ei_expect_celix_utils_extractZipData(nullptr, 0, CELIX_SUCCESS);
    }

    void installBundleAndExpectFailure() {
        //When I create bundle from a bundle zip by installing a bundle
        //Then the bundle install fails, and the bundle id is < 0
        long bndId = ctx->installBundle(SIMPLE_TEST_BUNDLE1_LOCATION);
        EXPECT_LT(bndId, 0);
    }

protected:
    std::shared_ptr<celix::Framework> fw{};
    std::shared_ptr<celix::BundleContext> ctx{};
};


TEST_F(BundleArchiveWithErrorInjectionTestSuite, BundleArchiveCreatedFailedTest) {
    teardownErrorInjectors();
    //Given a mocked calloc which returns NULL from a (indirect) call from bundleArchive_create
    celix_ei_expect_calloc((void*)celix_bundleArchive_create, 1, nullptr);
    installBundleAndExpectFailure();

    teardownErrorInjectors();
    //Given a mocked celix_utils_strdup which returns NULL from a (indirect) call from bundleArchive_create
    celix_ei_expect_celix_utils_strdup((void*)celix_bundleArchive_create, 1, nullptr);
    installBundleAndExpectFailure();

    teardownErrorInjectors();
    //Given a mocked malloc which returns NULL from a call from manifest_create
    celix_ei_expect_malloc((void*)manifest_create, 0, nullptr);
    installBundleAndExpectFailure();

    teardownErrorInjectors();
    //Given a mocked calloc which returns NULL from a call from bundleRevision_create
    celix_ei_expect_calloc((void*)celix_bundleRevision_create, 0, nullptr);
    installBundleAndExpectFailure();

    teardownErrorInjectors();
    //Given a mocked celix_utils_strdup which returns NULL from a call from bundleRevision_create
    celix_ei_expect_celix_utils_strdup((void*)celix_bundleRevision_create, 0, nullptr);
    installBundleAndExpectFailure();

    celix_ei_expect_celix_utils_strdup((void*)celix_bundleRevision_create, 0, nullptr, 2);
    installBundleAndExpectFailure();
}

TEST_F(BundleArchiveWithErrorInjectionTestSuite, BundleArchiveCreateCacheDirectoryFailedTest) {
    teardownErrorInjectors();
    //Given a mocked celix_utils_createDirectory which returns CELIX_FILE_IO_EXCEPTION from a (indirect) call from
    //bundleArchive_create
    celix_ei_expect_celix_utils_createDirectory((void*)celix_bundleArchive_create, 2, CELIX_FILE_IO_EXCEPTION);
    installBundleAndExpectFailure();

    teardownErrorInjectors();
    //Given a mocked asprintf which returns -1 from a (indirect) call from bundleArchive_create
    celix_ei_expect_asprintf((void*)celix_bundleArchive_create, 2, -1);
    installBundleAndExpectFailure();

    teardownErrorInjectors();
    //Given a mocked celix_utils_createDirectory which returns CELIX_FILE_IO_EXCEPTION from a second (indirect) call
    // from bundleArchive_create
    celix_ei_expect_celix_utils_createDirectory((void*)celix_bundleArchive_create, 2, CELIX_FILE_IO_EXCEPTION, 2);
    installBundleAndExpectFailure();

    teardownErrorInjectors();
    //Given a mocked asprintf which returns -1 from a (indirect) call from bundleArchive_create
    celix_ei_expect_asprintf((void*)celix_bundleArchive_create, 2, -1, 2);
    installBundleAndExpectFailure();

    teardownErrorInjectors();
    //Given a mocked celix_utils_createDirectory which returns CELIX_FILE_IO_EXCEPTION from a third (indirect) call
    // from bundleArchive_create
    celix_ei_expect_celix_utils_createDirectory((void*)celix_bundleArchive_create, 2, CELIX_FILE_IO_EXCEPTION, 3);
    installBundleAndExpectFailure();

    teardownErrorInjectors();
    //Given a mocked celix_utils_strdup which returns NULL from a (indirect) call from bundleArchive_create
    celix_ei_expect_celix_utils_strdup((void*)celix_bundleArchive_create, 2, nullptr);
    installBundleAndExpectFailure();

    teardownErrorInjectors();
    //Given a mocked celix_utils_strdup which returns NULL from a second (indirect) call from bundleArchive_create
    celix_ei_expect_celix_utils_strdup((void*)celix_bundleArchive_create, 2, nullptr, 2);
    installBundleAndExpectFailure();
}
