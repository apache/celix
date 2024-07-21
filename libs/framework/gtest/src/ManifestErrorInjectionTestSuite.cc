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
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 */

#include <gtest/gtest.h>

#include "celix_err.h"
#include "celix_properties_ei.h"
#include "malloc_ei.h"
#include "celix_bundle_manifest.h"

class ManifestErrorInjectionTestSuite : public ::testing::Test {
public:
    ManifestErrorInjectionTestSuite() = default;

    ~ManifestErrorInjectionTestSuite() override {
        teardownErrorInjectors();
    }

    static void teardownErrorInjectors() {
        celix_ei_expect_calloc(nullptr, 0, nullptr);
    }
};

TEST_F(ManifestErrorInjectionTestSuite, NoMemoryForManifestCreateTest) {
    celix_bundle_manifest_t* manifest = nullptr;
    celix_ei_expect_calloc((void*)celix_bundleManifest_create, 0, nullptr);
    celix_properties_t* attributes = celix_properties_create();
    ASSERT_NE(nullptr, attributes);
    celix_status_t status = celix_bundleManifest_create(attributes, &manifest);
    EXPECT_EQ(CELIX_ENOMEM, status);
    EXPECT_EQ(nullptr, manifest);
    celix_err_printErrors(stdout, "Errors are expected[", "]\n");
}
