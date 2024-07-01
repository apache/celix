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
#include "celix_err.h"

class ManifestTestSuite : public ::testing::Test {
  public:
    ManifestTestSuite() { celix_err_resetErrors(); }
};

TEST_F(ManifestTestSuite, CreateManifestTest) {
    //Given a properties set with all the mandatory manifest attributes
    celix_properties_t *properties = celix_properties_create();
    celix_version_t* v = celix_version_create(2, 0, 0, nullptr);
    celix_properties_assignVersion(properties, "CELIX_MANIFEST_VERSION", v);
    celix_properties_set(properties, "CELIX_BUNDLE_VERSION", "1.0.0");
    celix_properties_set(properties, "CELIX_BUNDLE_NAME", "my_bundle");
    celix_properties_set(properties, "CELIX_BUNDLE_SYMBOLIC_NAME", "celix_my_bundle");

    //When creating a manifest from the attributes
    celix_autoptr(celix_manifest_t) manifest = nullptr;
    celix_status_t status = celix_manifest_create(properties, &manifest);

    //Then the creation is successful
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_EQ(4, celix_properties_size(celix_manifest_getAttributes(manifest)));
}

TEST_F(ManifestTestSuite, MissingOrInvalidMandatoryManifestAttributesTest) {
    //Given an empty properties set
    celix_properties_t *properties = celix_properties_create();

    //When creating a manifest from the attributes
    celix_autoptr(celix_manifest_t) manifest = nullptr;
    celix_status_t status = celix_manifest_create(properties, &manifest);

    //Then the creation fails
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    //And 4 celix err log entries are logged (4 missing attributes)
    EXPECT_EQ(celix_err_getErrorCount(), 4);
    celix_err_printErrors(stdout, "Expect Errors: ", nullptr);

    //Given a properties set with not versions attributes for bundle/manifest version.
    properties = celix_properties_create();
    celix_properties_set(properties, "CELIX_BUNDLE_VERSION", "not a version");
    celix_properties_setBool(properties, "CELIX_MANIFEST_VERSION", false);
    celix_properties_set(properties, "CELIX_BUNDLE_NAME", "my_bundle");
    celix_properties_set(properties, "CELIX_BUNDLE_SYMBOLIC_NAME", "celix_my_bundle");

    //When creating a manifest from the attributes
    celix_autoptr(celix_manifest_t) manifest2 = nullptr;
    status = celix_manifest_create(properties, &manifest2);

    //Then the creation fails
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    //And 4 celix err log entries are logged (4x invalid versions)
    EXPECT_EQ(celix_err_getErrorCount(), 2);
    celix_err_printErrors(stdout, "Expect Errors: ", nullptr);

    //Given a properties with an unsupported manifest version
    properties = celix_properties_create();
    celix_properties_set(properties, "CELIX_MANIFEST_VERSION", "1.0.0"); //note must be 2.0.*
    celix_properties_set(properties, "CELIX_BUNDLE_VERSION", "1.0.0");
    celix_properties_set(properties, "CELIX_BUNDLE_NAME", "my_bundle");
    celix_properties_set(properties, "CELIX_BUNDLE_SYMBOLIC_NAME", "celix_my_bundle");

    //When creating a manifest from the attributes
    celix_autoptr(celix_manifest_t) manifest3 = nullptr;
    status = celix_manifest_create(properties, &manifest2);

    //Then the creation fails
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    //And 1 celix err log entries is logged
    EXPECT_EQ(celix_err_getErrorCount(), 1);
    celix_err_printErrors(stdout, "Expect Errors: ", nullptr);

    //Given a properties set with an invalid private libraries list
    properties = celix_properties_create();
    celix_properties_set(properties, "CELIX_MANIFEST_VERSION", "2.0.2");
    celix_properties_set(properties, "CELIX_BUNDLE_VERSION", "1.0.0");
    celix_properties_set(properties, "CELIX_BUNDLE_NAME", "my_bundle");
    celix_properties_set(properties, "CELIX_BUNDLE_SYMBOLIC_NAME", "celix_my_bundle");
    celix_properties_setBool(properties, "CELIX_BUNDLE_PRIVATE_LIBRARIES", true);

    //When creating a manifest from the attributes
    celix_autoptr(celix_manifest_t) manifest4 = nullptr;
    status = celix_manifest_create(properties, &manifest2);

    //Then the creation fails
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    //And 1 celix err log entries is logged
    EXPECT_EQ(celix_err_getErrorCount(), 1);
    celix_err_printErrors(stdout, "Expect Errors: ", nullptr);
}

//TODO check if the mandatory and optional attributes are set and can be retreived with getters
