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
#include <string>

#include "celix_stdio_cleanup.h"
#include "manifest.h"

class ManifestTestSuite : public ::testing::Test {
protected:
  manifest_pt manifest = nullptr;
  void SetUp() override {
      ASSERT_EQ(CELIX_SUCCESS, manifest_create(&manifest));
  }
  void TearDown() override {
      manifest_destroy(manifest);
  }
};

TEST_F(ManifestTestSuite, EmptyManifestTest) {
  const celix_properties_t* mainAttr = manifest_getMainAttributes(manifest);
  EXPECT_EQ(0, celix_properties_size(mainAttr));
  hash_map_pt entries = nullptr;
  EXPECT_EQ(CELIX_SUCCESS, manifest_getEntries(manifest, &entries));
  EXPECT_NE(nullptr, entries);
  EXPECT_EQ(0, hashMap_size(entries));
}

TEST_F(ManifestTestSuite, ReadManifestWithoutNameSectionsTest) {
    std::string content = "Manifest-Version: 1.0\n"
                          "DeploymentPackage-SymbolicName: com.third._3d\n"
                          "DeploymentPacakge-Version: 1.2.3.build22032005\n"
                          "DeploymentPackage-Copyright: ACME Inc. (c) 2003\n";
    celix_autoptr(FILE) manifestFile = tmpfile();
    EXPECT_EQ(content.size(), fwrite(content.c_str(), 1, content.size(), manifestFile));
    rewind(manifestFile);
    EXPECT_EQ(CELIX_SUCCESS, manifest_readFromStream(manifest, manifestFile));
    const celix_properties_t* mainAttr = manifest_getMainAttributes(manifest);
    EXPECT_EQ(4, celix_properties_size(mainAttr));
    EXPECT_STREQ("1.0", celix_properties_get(mainAttr, "Manifest-Version", nullptr));
    EXPECT_STREQ("com.third._3d", celix_properties_get(mainAttr, "DeploymentPackage-SymbolicName", nullptr));
    EXPECT_STREQ("1.2.3.build22032005", celix_properties_get(mainAttr, "DeploymentPacakge-Version", nullptr));
    EXPECT_STREQ("ACME Inc. (c) 2003", celix_properties_get(mainAttr, "DeploymentPackage-Copyright", nullptr));
    hash_map_pt entries = nullptr;
    EXPECT_EQ(CELIX_SUCCESS, manifest_getEntries(manifest, &entries));
    EXPECT_NE(nullptr, entries);
    EXPECT_EQ(0, hashMap_size(entries));
}

TEST_F(ManifestTestSuite, ReadManifestWithNameSectionsTest) {
    std::string content = "Manifest-Version: 1.0\n"
                          "DeploymentPackage-Icon: %icon\n"
                          "DeploymentPackage-SymbolicName: com.third._3d\n"
                          "DeploymentPacakge-Version: 1.2.3.build22032005\n"
                          "\n"
                          "Name: bundles/3dlib.jar\n"
                          "SHA1-Digest: MOez1l4gXHBo8ycYdAxstK3UvEg=\n"
                          "Bundle-SymbolicName: com.third._3d\n"
                          "Bundle-Version: 2.3.1\n"
                          "\n"
                          "Name: bundles/3dnative.jar\n"
                          "SHA1-Digest: N8Ow2UY4yjnHZv5zeq2I1Uv/+uE=\n"
                          "Bundle-SymbolicName: com.third._3d.native\n"
                          "Bundle-Version: 1.5.3\n"
                          "\n";
    celix_autoptr(FILE) manifestFile = tmpfile();
    EXPECT_EQ(content.size(), fwrite(content.c_str(), 1, content.size(), manifestFile));
    rewind(manifestFile);
    EXPECT_EQ(CELIX_SUCCESS, manifest_readFromStream(manifest, manifestFile));
    const celix_properties_t* mainAttr = manifest_getMainAttributes(manifest);
    EXPECT_EQ(4, celix_properties_size(mainAttr));
    EXPECT_STREQ("1.0", celix_properties_get(mainAttr, "Manifest-Version", nullptr));
    EXPECT_STREQ("%icon", celix_properties_get(mainAttr, "DeploymentPackage-Icon", nullptr));
    EXPECT_STREQ("com.third._3d", celix_properties_get(mainAttr, "DeploymentPackage-SymbolicName", nullptr));
    EXPECT_STREQ("1.2.3.build22032005", celix_properties_get(mainAttr, "DeploymentPacakge-Version", nullptr));
    hash_map_pt entries = nullptr;
    EXPECT_EQ(CELIX_SUCCESS, manifest_getEntries(manifest, &entries));
    EXPECT_NE(nullptr, entries);
    EXPECT_EQ(2, hashMap_size(entries));

    const celix_properties_t* entry1 = (const celix_properties_t*)hashMap_get(entries, "bundles/3dlib.jar");
    EXPECT_EQ(3, celix_properties_size(entry1));
    EXPECT_STREQ("MOez1l4gXHBo8ycYdAxstK3UvEg=", celix_properties_get(entry1, "SHA1-Digest", nullptr));
    EXPECT_STREQ("com.third._3d", celix_properties_get(entry1, "Bundle-SymbolicName", nullptr));
    EXPECT_STREQ("2.3.1", celix_properties_get(entry1, "Bundle-Version", nullptr));

    const celix_properties_t* entry2 = (const celix_properties_t*)hashMap_get(entries, "bundles/3dnative.jar");
    EXPECT_EQ(3, celix_properties_size(entry2));
    EXPECT_STREQ("N8Ow2UY4yjnHZv5zeq2I1Uv/+uE=", celix_properties_get(entry2, "SHA1-Digest", nullptr));
    EXPECT_STREQ("com.third._3d.native", celix_properties_get(entry2, "Bundle-SymbolicName", nullptr));
    EXPECT_STREQ("1.5.3", celix_properties_get(entry2, "Bundle-Version", nullptr));
}