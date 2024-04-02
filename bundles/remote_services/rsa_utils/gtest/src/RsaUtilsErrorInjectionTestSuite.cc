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
#include "celix_properties_ei.h"
#include "celix_err.h"

class RsaUtilsErrorInjectionTestSuite : public ::testing::Test {
 public:
   RsaUtilsErrorInjectionTestSuite() {
       celix_ei_expect_celix_properties_copy(nullptr, 0, nullptr);
       celix_err_printErrors(stderr, nullptr, nullptr);
   }
   ~RsaUtilsErrorInjectionTestSuite() override = default;
};

TEST_F(RsaUtilsErrorInjectionTestSuite, PropertiesCopyFailureTest) {
    // Given an error injection for celix_properties_copy
    celix_ei_expect_celix_properties_copy((void*)celix_rsaUtils_createServicePropertiesFromEndpointProperties, 0, nullptr);

    // And an endpointProperties
    celix_autoptr(celix_properties_t) endpointProperties = celix_properties_create();
    EXPECT_NE(endpointProperties, nullptr);

    // When calling celix_rsaUtils_createServicePropertiesFromEndpointProperties
    celix_properties_t* serviceProperties = nullptr;
    celix_status_t status =
        celix_rsaUtils_createServicePropertiesFromEndpointProperties(endpointProperties, &serviceProperties);

    // Then the status is CELIX_ENOMEM
    EXPECT_EQ(status, CELIX_ENOMEM);

    // And the serviceProperties is nullptr
    EXPECT_EQ(serviceProperties, nullptr);
}
