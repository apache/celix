/*
 Licensed to the Apache Software Foundation (ASF) under one
 or more contributor license agreements.  See the NOTICE file
 distributed with this work for additional information
 regarding copyright ownership.  The ASF licenses this file
 to you under the Apache License, Version 2.0 (the
 "License"); you may not use this file except in compliance
 with the License.  You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing,
  software distributed under the License is distributed on an
  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
  KIND, either express or implied.  See the License for the
  specific language governing permissions and limitations
  under the License.
 */

#include <gtest/gtest.h>

#include <errno.h>

#include "ifaddrs_ei.h"
#include "celix_ip_utils.h"
#include "celix_utils_ei.h"

class IpUtilsWithErrorInjectionTestSuite : public ::testing::Test {
public:
    IpUtilsWithErrorInjectionTestSuite() = default;
    ~IpUtilsWithErrorInjectionTestSuite() override {
        celix_ei_expect_getifaddrs(nullptr, 0, 0);
        celix_ei_expect_celix_utils_strdup(nullptr, 0, nullptr);
    }
};

TEST_F(IpUtilsWithErrorInjectionTestSuite, failToGetInterfaceAddresses) {
    celix_ei_expect_getifaddrs((void *)&celix_utils_findIpInSubnet, 0, -1);
    auto ipAddresses = celix_utils_findIpInSubnet("192.168.1.0/24");
    EXPECT_EQ(ipAddresses, nullptr);
    EXPECT_EQ(errno, EMFILE);
}

TEST_F(IpUtilsWithErrorInjectionTestSuite, failToDuplicateString) {
    celix_ei_expect_celix_utils_strdup((void *) &celix_utils_findIpInSubnet, 0, nullptr);
    auto ipAddresses = celix_utils_findIpInSubnet("192.168.1.0/24");
    EXPECT_EQ(ipAddresses, nullptr);
    EXPECT_EQ(errno, ENOMEM);
}