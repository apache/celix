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

#include "celix_rsa_utils.h"
#include "celix_properties.h"
#include "celix_constants.h"

class RsaUtilsTestSuite : public ::testing::Test {
 public:
   RsaUtilsTestSuite() = default;
   ~RsaUtilsTestSuite() override = default;
};

TEST_F(RsaUtilsTestSuite, CreateServicePropertiesFromEndpointPropertiesTest) {
   celix_autoptr(celix_properties_t) endpointProperties = celix_properties_create();
   celix_properties_set(endpointProperties, CELIX_FRAMEWORK_SERVICE_RANKING, "10");
   celix_properties_set(endpointProperties, CELIX_FRAMEWORK_SERVICE_VERSION, "1.0.0");

   celix_autoptr(celix_properties_t) serviceProperties = nullptr;
   celix_status_t status =
       celix_rsaUtils_createServicePropertiesFromEndpointProperties(endpointProperties, &serviceProperties);
    ASSERT_EQ(status, CELIX_SUCCESS);
    ASSERT_TRUE(serviceProperties != nullptr);

    const auto* entry = celix_properties_getEntry(serviceProperties, CELIX_FRAMEWORK_SERVICE_RANKING);
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->valueType, CELIX_PROPERTIES_VALUE_TYPE_LONG);
    EXPECT_EQ(entry->typed.longValue, 10L);

    entry = celix_properties_getEntry(serviceProperties, CELIX_FRAMEWORK_SERVICE_VERSION);
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->valueType, CELIX_PROPERTIES_VALUE_TYPE_VERSION);
    EXPECT_EQ(1, celix_version_getMajor(entry->typed.versionValue));
    EXPECT_EQ(0, celix_version_getMinor(entry->typed.versionValue));
    EXPECT_EQ(0, celix_version_getMicro(entry->typed.versionValue));
    EXPECT_STREQ("", celix_version_getQualifier(entry->typed.versionValue));
}

TEST_F(RsaUtilsTestSuite, CreateServicePropertiesFromEndpointPropertiesWithInvalidVersionTest) {
    celix_autoptr(celix_properties_t) endpointProperties = celix_properties_create();
    celix_properties_set(endpointProperties, CELIX_FRAMEWORK_SERVICE_RANKING, "10");
    celix_properties_set(endpointProperties, CELIX_FRAMEWORK_SERVICE_VERSION, "invalid");

    celix_autoptr(celix_properties_t) serviceProperties = nullptr;
    celix_status_t status =
            celix_rsaUtils_createServicePropertiesFromEndpointProperties(endpointProperties, &serviceProperties);
    ASSERT_EQ(status, CELIX_ILLEGAL_ARGUMENT);
    ASSERT_TRUE(serviceProperties == nullptr);
}

TEST_F(RsaUtilsTestSuite, CreateServicePropertiesFromEndpointPropertiesWithNullArgTest) {
    // NULL argument will result in an empty service properties set
    celix_autoptr(celix_properties_t) serviceProperties = nullptr;
    celix_status_t status =
            celix_rsaUtils_createServicePropertiesFromEndpointProperties(nullptr, &serviceProperties);
    ASSERT_EQ(status, CELIX_SUCCESS);
    ASSERT_NE(nullptr, serviceProperties);
    EXPECT_EQ(0, celix_properties_size(serviceProperties));
}
