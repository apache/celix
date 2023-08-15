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
#include <stdio.h>

#include "celix_err.h"
#include "celix_properties_ei.h"
#include "celix_stdio_cleanup.h"
#include "hmap_ei.h"
#include "malloc_ei.h"
#include "manifest.h"

class ManifestErrorInjectionTestSuite : public ::testing::Test {
public:
    ManifestErrorInjectionTestSuite() {
    }

    ~ManifestErrorInjectionTestSuite() override {
        teardownErrorInjectors();
    }
    void teardownErrorInjectors() {
        celix_ei_expect_malloc(nullptr, 0, nullptr);
        celix_ei_expect_celix_properties_create(nullptr, 0, nullptr);
        celix_ei_expect_celix_properties_copy(nullptr, 0, nullptr);
        celix_ei_expect_hashMap_create(nullptr, 0, nullptr);
    }
};

TEST_F(ManifestErrorInjectionTestSuite, NoMemoryForManifestCreateTest) {
    manifest_pt manifest = nullptr;
    celix_ei_expect_malloc((void*)manifest_create, 0, nullptr);
    celix_status_t status = manifest_create(&manifest);
    EXPECT_EQ(CELIX_ENOMEM, status);
    EXPECT_EQ(nullptr, manifest);
    celix_err_printErrors(stdout, "Errors are expected[", "]\n");
    teardownErrorInjectors();

    manifest = nullptr;
    celix_ei_expect_celix_properties_create((void*)manifest_create, 0, nullptr);
    status = manifest_create(&manifest);
    EXPECT_EQ(CELIX_ENOMEM, status);
    EXPECT_EQ(nullptr, manifest);
    celix_err_printErrors(stdout, "Errors are expected[", "]\n");
    teardownErrorInjectors();

    manifest = nullptr;
    celix_ei_expect_hashMap_create((void*)manifest_create, 0, nullptr);
    status = manifest_create(&manifest);
    EXPECT_EQ(CELIX_ENOMEM, status);
    EXPECT_EQ(nullptr, manifest);
    celix_err_printErrors(stdout, "Errors are expected[", "]\n");
    teardownErrorInjectors();
}

TEST_F(ManifestErrorInjectionTestSuite, NoMemoryForManifestCloneTest) {
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
    celix_auto(manifest_pt) manifest = nullptr;
    manifest_create(&manifest);
    EXPECT_EQ(CELIX_SUCCESS, manifest_readFromStream(manifest, manifestFile));
    celix_ei_expect_malloc((void*)manifest_create, 0, nullptr);
    EXPECT_EQ(nullptr, manifest_clone(manifest));
    teardownErrorInjectors();

    celix_ei_expect_celix_properties_copy((void*)manifest_clone, 0, nullptr);
    EXPECT_EQ(nullptr, manifest_clone(manifest));
}