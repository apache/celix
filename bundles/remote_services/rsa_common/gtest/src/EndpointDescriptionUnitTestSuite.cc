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
extern "C" {
#include "endpoint_description.h"
}
#include "celix_constants.h"
#include "celix_properties.h"
#include "remote_constants.h"
#include "celix_framework.h"
#include "celix_framework_factory.h"
#include "malloc_ei.h"
#include "celix_utils_ei.h"
#include "celix_properties_ei.h"
#include <gtest/gtest.h>

class EndpointDescriptionUnitTestSuite : public ::testing::Test {
public:
    EndpointDescriptionUnitTestSuite() {
        auto* props = celix_properties_create();
        celix_properties_set(props, CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, "true");
        celix_properties_set(props, CELIX_FRAMEWORK_CACHE_DIR, ".rsa_common_test_cache");
        fw = std::shared_ptr<celix_framework_t>{celix_frameworkFactory_createFramework(props), [](auto* f) {celix_frameworkFactory_destroyFramework(f);}};

        properties = std::shared_ptr<celix_properties_t>(celix_properties_create(), [](celix_properties_t* p) {celix_properties_destroy(p);});
        celix_properties_set(properties.get(), CELIX_RSA_ENDPOINT_FRAMEWORK_UUID, "e7c2a8ab-79a8-40c5-a926-85ae64c9382a");
        celix_properties_set(properties.get(), CELIX_FRAMEWORK_SERVICE_NAME, "org.example.api.Foo");
        celix_properties_set(properties.get(), CELIX_RSA_ENDPOINT_ID, "983e0ad8-5de9-44a8-b336-b3dc79d364b5");
        celix_properties_set(properties.get(), CELIX_RSA_ENDPOINT_SERVICE_ID, "100");
    }

    ~EndpointDescriptionUnitTestSuite() override {
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_ei_expect_celix_utils_strdup(nullptr, 0, nullptr);
        celix_ei_expect_celix_properties_copy(nullptr, 0, nullptr);
    }

    std::shared_ptr<celix_properties_t> properties{};
    std::shared_ptr<celix_framework_t> fw{};
};

TEST_F(EndpointDescriptionUnitTestSuite, CreateEndpointDescription) {
    endpoint_description_t* endpointDescription = nullptr;
    celix_properties_t *props = celix_properties_copy(properties.get());
    auto status = endpointDescription_create(props, &endpointDescription);
    EXPECT_EQ(status, CELIX_SUCCESS);
    EXPECT_TRUE(endpointDescription != nullptr);
    endpointDescription_destroy(endpointDescription);
}

TEST_F(EndpointDescriptionUnitTestSuite, CreateEndpointDescriptionWithInvalidArgs) {
    endpoint_description_t* endpointDescription = nullptr;
    auto status = endpointDescription_create(nullptr, &endpointDescription);
    EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);


    celix_properties_t *props = celix_properties_copy(properties.get());
    status = endpointDescription_create(props, nullptr);
    EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);
    celix_properties_destroy(props);
}

TEST_F(EndpointDescriptionUnitTestSuite, CreateEndpointDescriptionWithENOMEM) {
    endpoint_description_t* endpointDescription = nullptr;
    celix_properties_t *props = celix_properties_copy(properties.get());
    celix_ei_expect_calloc((void*)endpointDescription_create, 0, nullptr);
    auto status = endpointDescription_create(props, &endpointDescription);
    EXPECT_EQ(status, CELIX_ENOMEM);
    celix_properties_destroy(props);
}

TEST_F(EndpointDescriptionUnitTestSuite, CreateEndpointDescriptionWithInvalidProperties1) {
    endpoint_description_t* endpointDescription = nullptr;
    celix_properties_t *props = celix_properties_copy(properties.get());
    celix_properties_unset(props, CELIX_RSA_ENDPOINT_FRAMEWORK_UUID);
    auto status = endpointDescription_create(props, &endpointDescription);
    EXPECT_EQ(status, CELIX_BUNDLE_EXCEPTION);

    celix_properties_destroy(props);
}

TEST_F(EndpointDescriptionUnitTestSuite, CreateEndpointDescriptionWithInvalidProperties2) {
    endpoint_description_t* endpointDescription = nullptr;
    celix_properties_t *props = celix_properties_copy(properties.get());
    celix_properties_unset(props, CELIX_FRAMEWORK_SERVICE_NAME);
    auto status = endpointDescription_create(props, &endpointDescription);
    EXPECT_EQ(status, CELIX_BUNDLE_EXCEPTION);
    celix_properties_destroy(props);
}

TEST_F(EndpointDescriptionUnitTestSuite, CreateEndpointDescriptionWithInvalidProperties3) {
    endpoint_description_t* endpointDescription = nullptr;
    celix_properties_t *props = celix_properties_copy(properties.get());
    celix_properties_unset(props, CELIX_RSA_ENDPOINT_ID);
    auto status = endpointDescription_create(props, &endpointDescription);
    EXPECT_EQ(status, CELIX_BUNDLE_EXCEPTION);
    celix_properties_destroy(props);
}

TEST_F(EndpointDescriptionUnitTestSuite, CreateEndpointDescriptionWithInvalidProperties4) {
    endpoint_description_t* endpointDescription = nullptr;
    celix_properties_t *props = celix_properties_copy(properties.get());
    celix_properties_unset(props, CELIX_RSA_ENDPOINT_SERVICE_ID);
    auto status = endpointDescription_create(props, &endpointDescription);
    EXPECT_EQ(status, CELIX_BUNDLE_EXCEPTION);
    celix_properties_destroy(props);
}


class CloneEndpointDescriptionUnitTestSuite : public EndpointDescriptionUnitTestSuite {
public:
    CloneEndpointDescriptionUnitTestSuite() {
        celix_properties_t *props = celix_properties_copy(properties.get());
        endpoint_description_t* epPtr = nullptr;
        auto status = endpointDescription_create(props, &epPtr);
        EXPECT_EQ(status, CELIX_SUCCESS);
        EXPECT_TRUE(epPtr != nullptr);
        ep = std::shared_ptr<endpoint_description_t>{epPtr, [](auto* p) {endpointDescription_destroy(p);}};
    }

    ~CloneEndpointDescriptionUnitTestSuite() override {

    }
    std::shared_ptr<endpoint_description_t> ep{};
};

TEST_F(CloneEndpointDescriptionUnitTestSuite, CloneEndpointDescription) {
    endpoint_description_t* endpointDescription = endpointDescription_clone(ep.get());
    EXPECT_TRUE(endpointDescription != nullptr);
    endpointDescription_destroy(endpointDescription);
}

TEST_F(CloneEndpointDescriptionUnitTestSuite, CloneEndpointDescriptionWithInvalidArgs) {
    endpoint_description_t* endpointDescription = endpointDescription_clone(nullptr);
    EXPECT_TRUE(endpointDescription == nullptr);
}

TEST_F(CloneEndpointDescriptionUnitTestSuite, CloneEndpointDescriptionWithENOMEM) {
    celix_ei_expect_calloc((void*)endpointDescription_clone, 0, nullptr);
    endpoint_description_t* endpointDescription = endpointDescription_clone(ep.get());
    EXPECT_TRUE(endpointDescription == nullptr);
}


TEST_F(CloneEndpointDescriptionUnitTestSuite, CloneEndpointDescriptionWithCopyPropertiesError) {
    celix_ei_expect_celix_properties_copy((void*)endpointDescription_clone, 0, nullptr);
    endpoint_description_t* endpointDescription = endpointDescription_clone(ep.get());
    EXPECT_TRUE(endpointDescription == nullptr);
}

TEST_F(CloneEndpointDescriptionUnitTestSuite, CloneEndpointDescriptionWithDupingServiceNameError) {
    celix_ei_expect_celix_utils_strdup((void*)endpointDescription_clone, 0, nullptr);
    endpoint_description_t* endpointDescription = endpointDescription_clone(ep.get());
    EXPECT_TRUE(endpointDescription == nullptr);
}


