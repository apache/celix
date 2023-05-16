/*
 Licensed to the Apache Software Foundation (ASF) under one
 or more contributor license agreements.  See the NOTICE file
 distributed with this work for additional information
 regarding copyright ownership.  The ASF licenses this file
 to you under the Apache License, Version 2.0 (the
 "License"); you may not use this file except in compliance
 with the License.  You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing,
  software distributed under the License is distributed on an
  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
  KIND, either express or implied.  See the License for the
  specific language governing permissions and limitations
  under the License.
 */

#include "asprintf_ei.h"
#include "celix_constants.h"
#include "celix_bundle_cache.h"
#include "celix_properties.h"
#include "celix_log.h"
#include "framework_private.h"
#include "gtest/gtest.h"
#include "celix_hash_map_ei.h"
#include "malloc_ei.h"

class CelixBundleCacheErrorInjectionTestSuite : public ::testing::Test {
public:
    CelixBundleCacheErrorInjectionTestSuite() {
        fw.configurationMap = celix_properties_create();
        fw.logger = celix_frameworkLogger_create(CELIX_LOG_LEVEL_TRACE);
    }

    ~CelixBundleCacheErrorInjectionTestSuite() override {
        celix_ei_expect_asprintf(nullptr, 0, -1);
        celix_ei_expect_celix_stringHashMap_create(nullptr, 0, nullptr);
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_frameworkLogger_destroy(fw.logger);
        celix_properties_destroy(fw.configurationMap);
    }
    struct celix_framework fw{};
};

TEST_F(CelixBundleCacheErrorInjectionTestSuite, CacheCreateErrorTest) {
    celix_bundle_cache_t *cache = nullptr;
    celix_ei_expect_calloc((void *) celix_bundleCache_create, 0, nullptr);
    EXPECT_EQ(CELIX_ENOMEM, celix_bundleCache_create(&fw, &cache));
    EXPECT_EQ(nullptr, cache);
    celix_ei_expect_celix_stringHashMap_create((void *) celix_bundleCache_create, 0, nullptr);
    EXPECT_EQ(CELIX_ENOMEM, celix_bundleCache_create(&fw, &cache));
    EXPECT_EQ(nullptr, cache);
    celix_properties_setBool(fw.configurationMap, CELIX_FRAMEWORK_CACHE_USE_TMP_DIR, true);
    celix_ei_expect_asprintf((void *) celix_bundleCache_create, 0, -1);
    EXPECT_EQ(CELIX_ENOMEM, celix_bundleCache_create(&fw, &cache));
    EXPECT_EQ(nullptr, cache);
    celix_properties_setBool(fw.configurationMap, CELIX_FRAMEWORK_CACHE_USE_TMP_DIR, false);
}