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

#include "celix_ip_utils.h"
#include "celix_err.h"

#include "celix_utils_ei.h"
#include "ifaddrs_ei.h"
#include "malloc_ei.h"

class IpUtilsWithErrorInjectionTestSuite : public ::testing::Test {
public:
    IpUtilsWithErrorInjectionTestSuite() {
        celix_err_resetErrors();
    }

    ~IpUtilsWithErrorInjectionTestSuite() override {
        celix_ei_expect_getifaddrs(nullptr, 0, 0);
        celix_ei_expect_celix_utils_strdup(nullptr, 0, nullptr);
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_err_resetErrors();
    }
};

TEST_F(IpUtilsWithErrorInjectionTestSuite, FailToGetInterfaceAddressesTest) {
    celix_ei_expect_getifaddrs((void *)&celix_utils_findIpInSubnet, 0, -1);
    char* ipAddr = nullptr;
    auto status = celix_utils_findIpInSubnet("192.168.1.0/24", &ipAddr);
    EXPECT_EQ(EMFILE, status);
    EXPECT_EQ(ipAddr, nullptr);
    EXPECT_EQ(1, celix_err_getErrorCount());
}

TEST_F(IpUtilsWithErrorInjectionTestSuite, FailToDuplicateStringTest) {
    int errCount = 1;
    char* ipAddr = nullptr;

    //first call to celix_utils_strdup fails
    celix_ei_expect_celix_utils_strdup((void *) &celix_utils_findIpInSubnet, 0, nullptr);
    auto status = celix_utils_findIpInSubnet("192.168.1.0/24", &ipAddr);
    EXPECT_EQ(status, ENOMEM);
    EXPECT_EQ(ipAddr, nullptr);
    EXPECT_EQ(errCount++, celix_err_getErrorCount());

    //second call to celix_utils_strdup fails (in ifa -> ifa_next loop)
    celix_ei_expect_celix_utils_strdup((void *) &celix_utils_findIpInSubnet, 0, nullptr, 2);
    status = celix_utils_findIpInSubnet("127.0.0.1/24", &ipAddr);
    EXPECT_EQ(status, ENOMEM);
    EXPECT_EQ(ipAddr, nullptr);
    EXPECT_EQ(errCount++, celix_err_getErrorCount());

    celix_ei_expect_celix_utils_strdup((void *) &celix_utils_convertIpToUint, 0, nullptr);
    bool converted;
    auto ipAsUint = celix_utils_convertIpToUint("192.168.1.0", &converted);
    EXPECT_EQ(ipAsUint, 0);
    EXPECT_FALSE(converted);
    EXPECT_EQ(errno, ENOMEM);
    EXPECT_EQ(errCount++, celix_err_getErrorCount());
}

TEST_F(IpUtilsWithErrorInjectionTestSuite, FailToCalledTest) {
    celix_ei_expect_calloc((void*)celix_utils_convertUintToIp, 0, nullptr);
    auto ip = celix_utils_convertUintToIp(3232235840);
    EXPECT_EQ(ip, nullptr);
    EXPECT_EQ(errno, ENOMEM);
    EXPECT_EQ(1, celix_err_getErrorCount());
}
