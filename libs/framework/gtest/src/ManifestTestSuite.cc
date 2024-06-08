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

#include "celix_manifest.h"
#include "celix_properties.h"

class ManifestTestSuite : public ::testing::Test {
};

TEST_F(ManifestTestSuite, CreateManifestTest) {
    celix_properties_t *properties = celix_properties_create();
    celix_properties_set(properties, "key", "value");
    celix_autoptr(celix_manifest_t) manifest = nullptr;
    celix_status_t status = celix_manifest_create(properties, &manifest);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_EQ(1, celix_properties_size(celix_manifest_getAttributes(manifest)));

    celix_autoptr(celix_manifest_t) manifest2 = nullptr;
    status = celix_manifest_create(nullptr, &manifest2);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_EQ(0, celix_properties_size(celix_manifest_getAttributes(manifest2)));

    celix_autoptr(celix_manifest_t) manifest3 = nullptr;
    status = celix_manifest_createFromFile("empty.properties", &manifest3);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_EQ(0, celix_properties_size(celix_manifest_getAttributes(manifest3)));
}

