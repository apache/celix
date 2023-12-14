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
#include <stdio.h>

#include "celix_err.h"
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
        celix_err_resetErrors();
    }
    void CheckPropertiesEqual(const celix_properties_t* prop1, const celix_properties_t* prop2) {
        EXPECT_EQ(celix_properties_size(prop1), celix_properties_size(prop2));
        CELIX_PROPERTIES_ITERATE(prop1, iter) {
            EXPECT_STREQ(celix_properties_get(prop1, iter.key, nullptr), celix_properties_get(prop2, iter.key, nullptr));
        }
    }
    void CheckManifestEqual(const manifest_pt manifest1, const manifest_pt manifest2) {
        CheckPropertiesEqual(manifest_getMainAttributes(manifest1), manifest_getMainAttributes(manifest2));
        hash_map_pt entries1 = nullptr;
        EXPECT_EQ(CELIX_SUCCESS, manifest_getEntries(manifest1, &entries1));
        hash_map_pt entries2 = nullptr;
        EXPECT_EQ(CELIX_SUCCESS, manifest_getEntries(manifest2, &entries2));
        EXPECT_EQ(hashMap_size(entries1), hashMap_size(entries2));
        hash_map_iterator_t iter = hashMapIterator_construct(entries1);
        while (hashMapIterator_hasNext(&iter)) {
            hash_map_entry_pt entry = hashMapIterator_nextEntry(&iter);
            celix_properties_t* value1 = (celix_properties_t*)hashMapEntry_getValue(entry);
            celix_properties_t* value2 = (celix_properties_t*)hashMap_get(entries2, hashMapEntry_getKey(entry));
            CheckPropertiesEqual(value1, value2);
        }
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
    celix_autoptr(FILE) manifestFile = fmemopen((void*)content.c_str(), content.size(), "r");
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

TEST_F(ManifestTestSuite, ReadManifestWithCarriageReturnTest) {
    std::string content = "Manifest-Version: 1.0\r\n"
                          "DeploymentPackage-SymbolicName: com.third._3d\r\n"
                          "DeploymentPacakge-Version: 1.2.3.build22032005\r\n"
                          "DeploymentPackage-Copyright: ACME Inc. (c) 2003\r\n";
    celix_autoptr(FILE) manifestFile = fmemopen((void*)content.c_str(), content.size(), "r");
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

TEST_F(ManifestTestSuite, ReadManifestWithLineContinuationTest) {
    std::string content = "Manifest-Version: 1.0\n"
                          "DeploymentPackage-SymbolicName: com.third._3d\n"
                          " x\n" /* line continuation */
                          "DeploymentPacakge-Version: 1.2.3.build22032005\n"
                          "DeploymentPackage-Copyright: ACME Inc. (c) 2003\n";
    celix_autoptr(FILE) manifestFile = fmemopen((void*)content.c_str(), content.size(), "r");
    EXPECT_EQ(CELIX_SUCCESS, manifest_readFromStream(manifest, manifestFile));
    const celix_properties_t* mainAttr = manifest_getMainAttributes(manifest);
    EXPECT_EQ(4, celix_properties_size(mainAttr));
    EXPECT_STREQ("1.0", celix_properties_get(mainAttr, "Manifest-Version", nullptr));
    EXPECT_STREQ("com.third._3dx", celix_properties_get(mainAttr, "DeploymentPackage-SymbolicName", nullptr));
    EXPECT_STREQ("1.2.3.build22032005", celix_properties_get(mainAttr, "DeploymentPacakge-Version", nullptr));
    EXPECT_STREQ("ACME Inc. (c) 2003", celix_properties_get(mainAttr, "DeploymentPackage-Copyright", nullptr));
    hash_map_pt entries = nullptr;
    EXPECT_EQ(CELIX_SUCCESS, manifest_getEntries(manifest, &entries));
    EXPECT_NE(nullptr, entries);
    EXPECT_EQ(0, hashMap_size(entries));
}

TEST_F(ManifestTestSuite, ReadManifestWithoutNewlineInLastLineTest) {
    std::string content = "Manifest-Version: 1.0\n"
                          "DeploymentPackage-SymbolicName: com.third._3d\n"
                          "DeploymentPacakge-Version: 1.2.3.build22032005\n"
                          "DeploymentPackage-Copyright: ACME Inc. (c) 2003";
    celix_autoptr(FILE) manifestFile = fmemopen((void*)content.c_str(), content.size(), "r");
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
                          "\n"
                          "Name: bundles/3dnative1.jar\n"
                          "SHA1-Digest: N8Ow2UY4yjnHZv5zeq2I1Uv/+uF=\n"
                          "Bundle-SymbolicName: com.third._3d.native1\n"
                          "Bundle-Version: 1.5.4\n"
                          "\n";
    celix_autoptr(FILE) manifestFile = fmemopen((void*)content.c_str(), content.size(), "r");
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
    EXPECT_EQ(3, hashMap_size(entries));

    const celix_properties_t* entry1 = (const celix_properties_t*)hashMap_get(entries, "bundles/3dlib.jar");
    EXPECT_EQ(4, celix_properties_size(entry1));
    EXPECT_STREQ("MOez1l4gXHBo8ycYdAxstK3UvEg=", celix_properties_get(entry1, "SHA1-Digest", nullptr));
    EXPECT_STREQ("com.third._3d", celix_properties_get(entry1, "Bundle-SymbolicName", nullptr));
    EXPECT_STREQ("2.3.1", celix_properties_get(entry1, "Bundle-Version", nullptr));

    const celix_properties_t* entry2 = (const celix_properties_t*)hashMap_get(entries, "bundles/3dnative.jar");
    EXPECT_EQ(4, celix_properties_size(entry2));
    EXPECT_STREQ("N8Ow2UY4yjnHZv5zeq2I1Uv/+uE=", celix_properties_get(entry2, "SHA1-Digest", nullptr));
    EXPECT_STREQ("com.third._3d.native", celix_properties_get(entry2, "Bundle-SymbolicName", nullptr));
    EXPECT_STREQ("1.5.3", celix_properties_get(entry2, "Bundle-Version", nullptr));
}

TEST_F(ManifestTestSuite, CleanupTest) {
    celix_auto(manifest_pt) manifest2 = nullptr;
    EXPECT_EQ(CELIX_SUCCESS, manifest_create(&manifest2));
    celix_auto(manifest_pt) manifest3 = nullptr;
    EXPECT_EQ(CELIX_SUCCESS, manifest_create(&manifest3));
    EXPECT_EQ(CELIX_SUCCESS, manifest_destroy(celix_steal_ptr(manifest3)));
}

TEST_F(ManifestTestSuite, CloneTest) {
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
    celix_autoptr(FILE) manifestFile = fmemopen((void*)content.c_str(), content.size(), "r");
    EXPECT_EQ(CELIX_SUCCESS, manifest_readFromStream(manifest, manifestFile));
    celix_auto(manifest_pt) clone = manifest_clone(manifest);
    CheckManifestEqual(manifest, clone);
}

TEST_F(ManifestTestSuite, CreateFromNonexistingFile) {
    celix_auto(manifest_pt) manifest2 = nullptr;
    EXPECT_EQ(ENOENT, manifest_createFromFile("nonexisting", &manifest2));
    EXPECT_EQ(nullptr, manifest2);
    celix_err_printErrors(stdout, "Errors are expected[", "]\n");
}

TEST_F(ManifestTestSuite, ManifestMissingSpaceSeparatorTest) {
    std::string content = "Manifest-Version: 1.0\n"
                          "NoSeparator:%icon\n"
                          "DeploymentPackage-SymbolicName: com.third._3d\n"
                          "DeploymentPacakge-Version: 1.2.3.build22032005\n"
                          "\n";
    celix_autoptr(FILE) manifestFile = fmemopen((void*)content.c_str(), content.size(), "r");
    EXPECT_EQ(CELIX_INVALID_SYNTAX, manifest_readFromStream(manifest, manifestFile));
    celix_err_printErrors(stdout, "Errors are expected[", "]\n");
}

TEST_F(ManifestTestSuite, ManifestMissingAttributeNameTest) {
    std::string content = "Manifest-Version: 1.0\n"
                          "%icon\n"
                          "DeploymentPackage-SymbolicName: com.third._3d\n"
                          "DeploymentPacakge-Version: 1.2.3.build22032005\n"
                          "\n";
    celix_autoptr(FILE) manifestFile = fmemopen((void*)content.c_str(), content.size(), "r");
    EXPECT_EQ(CELIX_INVALID_SYNTAX, manifest_readFromStream(manifest, manifestFile));
    celix_err_printErrors(stdout, "Errors are expected[", "]\n");
}

TEST_F(ManifestTestSuite, ManifestWithDuplicatedAttributeNameTest) {
    std::string content = "Manifest-Version: 1.0\n"
                          "DeploymentPackage-Icon: %icon\n"
                          "DeploymentPackage-Icon: %icon\n"
                          "DeploymentPackage-SymbolicName: com.third._3d\n"
                          "DeploymentPacakge-Version: 1.2.3.build22032005\n"
                          "\n";
    celix_autoptr(FILE) manifestFile = fmemopen((void*)content.c_str(), content.size(), "r");
    EXPECT_EQ(CELIX_INVALID_SYNTAX, manifest_readFromStream(manifest, manifestFile));
    celix_err_printErrors(stdout, "Errors are expected[", "]\n");
}

TEST_F(ManifestTestSuite, ManifestMissingNameAttributeTest) {
    std::string content = "Manifest-Version: 1.0\n"
                          "DeploymentPackage-Icon: %icon\n"
                          "DeploymentPackage-SymbolicName: com.third._3d\n"
                          "DeploymentPacakge-Version: 1.2.3.build22032005\n"
                          "\n"
                          "SHA1-Digest: MOez1l4gXHBo8ycYdAxstK3UvEg=\n"
                          "Bundle-SymbolicName: com.third._3d\n"
                          "Bundle-Version: 2.3.1\n"
                          "\n"
                          "Name: bundles/3dnative.jar\n"
                          "SHA1-Digest: N8Ow2UY4yjnHZv5zeq2I1Uv/+uE=\n"
                          "Bundle-SymbolicName: com.third._3d.native\n"
                          "Bundle-Version: 1.5.3\n"
                          "\n";
    celix_autoptr(FILE) manifestFile = fmemopen((void*)content.c_str(), content.size(), "r");
    EXPECT_EQ(CELIX_INVALID_SYNTAX, manifest_readFromStream(manifest, manifestFile));
    celix_err_printErrors(stdout, "Errors are expected[", "]\n");
}

TEST_F(ManifestTestSuite, ManifestMissingVersionTest) {
    std::string content = /*"Manifest-Version: 1.0\n"*/
                          "DeploymentPackage-Icon: %icon\n"
                          "DeploymentPackage-SymbolicName: com.third._3d\n"
                          "DeploymentPacakge-Version: 1.2.3.build22032005\n"
                          "\n";
    celix_autoptr(FILE) manifestFile = fmemopen((void*)content.c_str(), content.size(), "r");
    EXPECT_EQ(CELIX_INVALID_SYNTAX, manifest_readFromStream(manifest, manifestFile));
    celix_err_printErrors(stdout, "Errors are expected[", "]\n");
}

TEST_F(ManifestTestSuite, ReadEmptyManifestTest) {
    std::string content = "\n";
    celix_autoptr(FILE) manifestFile = fmemopen((void*)content.c_str(), content.size(), "r");
    EXPECT_EQ(CELIX_INVALID_SYNTAX, manifest_readFromStream(manifest, manifestFile));
    celix_err_printErrors(stdout, "Errors are expected[", "]\n");
}
