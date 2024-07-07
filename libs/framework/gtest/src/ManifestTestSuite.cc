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

#include "celix_bundle_manifest.h"
#include "celix_err.h"
#include "celix_properties.h"
#include "celix_stdlib_cleanup.h"

class ManifestTestSuite : public ::testing::Test {
  public:
    ManifestTestSuite() { celix_err_resetErrors(); }

    static celix_properties_t* createAttributes(const char* manifestVersion,
                                                const char* bundleVersion,
                                                const char* bundleName,
                                                const char* symbolicName) {
        celix_properties_t* properties = celix_properties_create();
        celix_properties_set(properties, "CELIX_BUNDLE_MANIFEST_VERSION", manifestVersion);
        celix_properties_set(properties, "CELIX_BUNDLE_VERSION", bundleVersion);
        celix_properties_set(properties, "CELIX_BUNDLE_NAME", bundleName);
        celix_properties_set(properties, "CELIX_BUNDLE_SYMBOLIC_NAME", symbolicName);
        return properties;
    }
};

TEST_F(ManifestTestSuite, CreateManifestTest) {
    //Given a properties set with all the mandatory manifest attributes
    celix_properties_t *properties = celix_properties_create();
    celix_version_t* v = celix_version_create(2, 0, 0, nullptr);
    celix_properties_assignVersion(properties, "CELIX_BUNDLE_MANIFEST_VERSION", v);
    celix_properties_set(properties, "CELIX_BUNDLE_VERSION", "1.0.0");
    celix_properties_set(properties, "CELIX_BUNDLE_NAME", "my_bundle");
    celix_properties_set(properties, "CELIX_BUNDLE_SYMBOLIC_NAME", "celix_my_bundle");

    //When creating a manifest from the attributes
    celix_autoptr(celix_bundle_manifest_t) manifest = nullptr;
    celix_status_t status = celix_bundleManifest_create(properties, &manifest);

    //Then the creation is successful
    ASSERT_EQ(CELIX_SUCCESS, status);
    EXPECT_EQ(4, celix_properties_size(celix_bundleManifest_getAttributes(manifest)));
}

TEST_F(ManifestTestSuite, MissingOrInvalidMandatoryManifestAttributesTest) {
    //Given an empty properties set
    celix_properties_t *properties = celix_properties_create();

    //When creating a manifest from the attributes
    celix_autoptr(celix_bundle_manifest_t) manifest = nullptr;
    celix_status_t status = celix_bundleManifest_create(properties, &manifest);

    //Then the creation fails
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    //And 4 celix err log entries are logged (4 missing attributes)
    EXPECT_EQ(celix_err_getErrorCount(), 4);
    celix_err_printErrors(stdout, "Expect Errors: ", nullptr);

    //Given a properties set with not versions attributes for bundle/manifest version.
    properties = celix_properties_create();
    celix_properties_set(properties, "CELIX_BUNDLE_VERSION", "not a version");
    celix_properties_setBool(properties, "CELIX_BUNDLE_MANIFEST_VERSION", false);
    celix_properties_set(properties, "CELIX_BUNDLE_NAME", "my_bundle");
    celix_properties_set(properties, "CELIX_BUNDLE_SYMBOLIC_NAME", "celix_my_bundle");

    //When creating a manifest from the attributes
    celix_autoptr(celix_bundle_manifest_t) manifest2 = nullptr;
    status = celix_bundleManifest_create(properties, &manifest2);

    //Then the creation fails
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    //And 4 celix err log entries are logged (4x invalid versions)
    EXPECT_EQ(celix_err_getErrorCount(), 2);
    celix_err_printErrors(stdout, "Expect Errors: ", nullptr);

    //Given a properties with an unsupported manifest version
    properties = createAttributes("1.0.0" /* note must be 2.0.**/, "1.0.0", "my_bundle", "celix_my_bundle");

    //When creating a manifest from the attributes
    celix_autoptr(celix_bundle_manifest_t) manifest3 = nullptr;
    status = celix_bundleManifest_create(properties, &manifest3);

    //Then the creation fails
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    //And 1 celix err log entries is logged
    EXPECT_EQ(celix_err_getErrorCount(), 1);
    celix_err_printErrors(stdout, "Expect Errors: ", nullptr);

    //Given a properties set with an invalid private libraries list
    properties = createAttributes("2.0.2", "1.0.0", "my_bundle", "celix_my_bundle");
    celix_properties_setBool(properties, "CELIX_BUNDLE_PRIVATE_LIBRARIES", true);

    //When creating a manifest from the attributes
    celix_autoptr(celix_bundle_manifest_t) manifest4 = nullptr;
    status = celix_bundleManifest_create(properties, &manifest4);

    //Then the creation fails
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    //And 1 celix err log entries is logged
    EXPECT_EQ(celix_err_getErrorCount(), 1);
    celix_err_printErrors(stdout, "Expect Errors: ", nullptr);
}

TEST_F(ManifestTestSuite, InvalidBundleSymbolicNameTest) {
        //Given a properties set with an invalid bundle symbolic name
        celix_properties_t *properties = createAttributes("2.0.0", "1.0.0", "my_bundle", "celix_my_bundle$$$$");

        //When creating a manifest from the attributes
        celix_autoptr(celix_bundle_manifest_t) manifest = nullptr;
        celix_status_t status = celix_bundleManifest_create(properties, &manifest);

        //Then the creation fails
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

        //And 1 celix err log entries is logged
        EXPECT_EQ(celix_err_getErrorCount(), 1);
        celix_err_printErrors(stdout, "Expect Errors: ", nullptr);
}


TEST_F(ManifestTestSuite, GetBuiltinAttributes) {
        //Given a properties set with all the mandatory and manifest attributes
        celix_properties_t *properties = createAttributes("2.0.0", "1.0.0", "my_bundle", "celix_my_bundle");

        //When creating a manifest from the attributes
        celix_autoptr(celix_bundle_manifest_t) manifest = nullptr;
        celix_status_t status = celix_bundleManifest_create(properties, &manifest);

        //Then the creation is successful
        EXPECT_EQ(CELIX_SUCCESS, status);

        //And the manifest contains the mandatory attributes
        EXPECT_EQ(4, celix_properties_size(celix_bundleManifest_getAttributes(manifest)));
        EXPECT_STREQ("my_bundle", celix_bundleManifest_getBundleName(manifest));
        EXPECT_STREQ("celix_my_bundle", celix_bundleManifest_getSymbolicName(manifest));

        const auto* mv = celix_bundleManifest_getManifestVersion(manifest);
        const auto* bv = celix_bundleManifest_getBundleVersion(manifest);
        ASSERT_NE(nullptr, mv);
        ASSERT_NE(nullptr, bv);

        celix_autofree char* manifestVersion = celix_version_toString(mv);
        celix_autofree char* bundleVersion = celix_version_toString(bv);
        EXPECT_STREQ("2.0.0", manifestVersion);
        EXPECT_STREQ("1.0.0", bundleVersion);

        //And the manifest does not contain optional attributes
        EXPECT_EQ(nullptr, celix_bundleManifest_getBundleActivatorLibrary(manifest));
        EXPECT_EQ(nullptr, celix_bundleManifest_getBundlePrivateLibraries(manifest));
        EXPECT_EQ(nullptr, celix_bundleManifest_getBundleGroup(manifest));

        //Given a properties set with all the mandatory and optional attributes
        properties = createAttributes("2.0.0", "1.0.0", "my_bundle", "celix_my_bundle");
        celix_properties_set(properties, "CELIX_BUNDLE_ACTIVATOR_LIBRARY", "my_activator");
        celix_properties_set(properties, "CELIX_BUNDLE_PRIVATE_LIBRARIES", "lib1,lib2");
        celix_properties_set(properties, "CELIX_BUNDLE_GROUP", "my_group");

        //When creating a manifest from the attributes
        celix_autoptr(celix_bundle_manifest_t) manifest2 = nullptr;
        status = celix_bundleManifest_create(properties, &manifest2);

        //Then the creation is successful
        celix_err_printErrors(stdout, "Expect Errors: ", nullptr);
        ASSERT_EQ(CELIX_SUCCESS, status);

        //And the manifest contains the optional attributes
        EXPECT_STREQ("my_activator", celix_bundleManifest_getBundleActivatorLibrary(manifest2));
        const celix_array_list_t* privateLibraries = celix_bundleManifest_getBundlePrivateLibraries(manifest2);
        ASSERT_NE(nullptr, privateLibraries);
        EXPECT_EQ(2, celix_arrayList_size(privateLibraries));
        EXPECT_STREQ("lib1", celix_arrayList_getString(privateLibraries, 0));
        EXPECT_STREQ("lib2", celix_arrayList_getString(privateLibraries, 1));
        EXPECT_STREQ("my_group", celix_bundleManifest_getBundleGroup(manifest2));
}
