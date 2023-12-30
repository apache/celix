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

#include "celix_err.h"
#include "celix_ip_utils.h"
#include "celix_stdlib_cleanup.h"

class IpUtilsTestSuite : public ::testing::Test {
public:
    IpUtilsTestSuite() {
        celix_err_resetErrors();
    }
    ~IpUtilsTestSuite() override {
        celix_err_printErrors(stderr, nullptr, nullptr);
    }
};

TEST_F(IpUtilsTestSuite, ipToUnsignedIntTest) {
    const char *ip = "192.168.1.64";
    uint32_t expected = 3232235840;
    uint32_t actual = celix_utils_convertIpToUint(ip, nullptr);
    EXPECT_EQ(expected, actual);

    bool converted;
    actual = celix_utils_convertIpToUint(ip, &converted);
    EXPECT_TRUE(converted);
    EXPECT_EQ(expected, actual);

    EXPECT_EQ(0, celix_utils_convertIpToUint("a.b.c.d", &converted));
    EXPECT_FALSE(converted);
    EXPECT_EQ(1, celix_err_getErrorCount());

    EXPECT_EQ(0, celix_utils_convertIpToUint("273.168.1.64", &converted));
    EXPECT_FALSE(converted);
    EXPECT_EQ(2, celix_err_getErrorCount());

    EXPECT_EQ(0, celix_utils_convertIpToUint("10.1.1.1.1", &converted));
    EXPECT_FALSE(converted);
    EXPECT_EQ(3, celix_err_getErrorCount());
}

TEST_F(IpUtilsTestSuite, unsignedIntToIpTest) {
    uint32_t ipAsUint = 3232235840;
    const char *expected = "192.168.1.64";
    char* ip = celix_utils_convertUintToIp(ipAsUint);
    EXPECT_STREQ(expected, ip);
    free(ip);

    ip = celix_utils_convertUintToIp(0);
    EXPECT_STREQ("0.0.0.0", ip);
    free(ip);

    ip = celix_utils_convertUintToIp(UINT32_MAX);
    EXPECT_STREQ("255.255.255.255", ip);
    free(ip);
}

TEST_F(IpUtilsTestSuite, prefixToBitmaskTest) {
    uint32_t bitmask = celix_utils_ipPrefixLengthToBitmask(27);
    EXPECT_EQ(4294967264, bitmask);

    bitmask = celix_utils_ipPrefixLengthToBitmask(0);
    EXPECT_EQ(0, bitmask);

    bitmask = celix_utils_ipPrefixLengthToBitmask(32);
    EXPECT_EQ(UINT32_MAX, bitmask);

    bitmask = celix_utils_ipPrefixLengthToBitmask(-1);
    EXPECT_EQ(0, bitmask);

    bitmask = celix_utils_ipPrefixLengthToBitmask(33);
    EXPECT_EQ(0, bitmask);
}

TEST_F(IpUtilsTestSuite, NetmaskToPrefixTest) {
    int prefix = celix_utils_ipNetmaskToPrefixLength("255.255.255.0");
    EXPECT_EQ(24, prefix);

    prefix = celix_utils_ipNetmaskToPrefixLength("0.0.0.0");
    EXPECT_EQ(0, prefix);

    prefix = celix_utils_ipNetmaskToPrefixLength("255.255.255.255");
    EXPECT_EQ(32, prefix);

    prefix = celix_utils_ipNetmaskToPrefixLength("255.0.0.0");
    EXPECT_EQ(8, prefix);

    EXPECT_EQ(-1, celix_utils_ipNetmaskToPrefixLength("a.b.c.d"));
    EXPECT_EQ(1, celix_err_getErrorCount());
}

TEST_F(IpUtilsTestSuite, FindIpInSubnetWithInvalidInputTest) {
    EXPECT_EQ(nullptr, celix_utils_findIpInSubnet("198.168.0.1")); // missing subnet
    EXPECT_EQ(1, celix_err_getErrorCount());

    EXPECT_EQ(nullptr, celix_utils_findIpInSubnet("198.168.0.1/abc")); // invalid subnet
    EXPECT_EQ(2, celix_err_getErrorCount());

    EXPECT_EQ(nullptr, celix_utils_findIpInSubnet("198.168.0.1/40")); // out of range subnet
    EXPECT_EQ(3, celix_err_getErrorCount());

    EXPECT_EQ(nullptr, celix_utils_findIpInSubnet("198.168.0.1/-1")); // out of range subnet
    EXPECT_EQ(4, celix_err_getErrorCount());

    EXPECT_EQ(nullptr, celix_utils_findIpInSubnet("a.b.c.d/8")); // invalid ip
    EXPECT_EQ(5, celix_err_getErrorCount());
}
