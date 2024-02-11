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

#include "celix_version.h"
#include "celix_version_range.h" 
#include "celix_stdlib_cleanup.h"
#include "celix_version_range.h"
#include "version_private.h"

class VersionRangeTestSuite : public ::testing::Test {};

TEST_F(VersionRangeTestSuite, CreateTest) {
    celix_version_range_t* range = nullptr;
    celix_version_t* low = celix_version_createEmptyVersion();
    celix_version_t* high = celix_version_createEmptyVersion();
    range = celix_versionRange_createVersionRange(low, false, high, true);
    EXPECT_TRUE((range != nullptr));
    EXPECT_EQ(true, celix_versionRange_isHighInclusive(range));
    EXPECT_EQ(false, celix_versionRange_isLowInclusive(range));
    EXPECT_EQ(low, celix_versionRange_getLowVersion(range));
    EXPECT_EQ(high, celix_versionRange_getHighVersion(range));

    celix_versionRange_destroy(range);
}

TEST_F(VersionRangeTestSuite, CleanupTest) {
    celix_autoptr(celix_version_range_t) range = celix_versionRange_createInfiniteVersionRange();
}

TEST_F(VersionRangeTestSuite, CreateInfiniteTest) {
    celix_version_t* version = celix_version_create(1,2, 3, nullptr);
    celix_version_range_t* range = celix_versionRange_createInfiniteVersionRange();
    EXPECT_TRUE(range != nullptr);
    EXPECT_EQ(true, celix_versionRange_isHighInclusive(range));
    EXPECT_EQ(true, celix_versionRange_isLowInclusive(range));
    EXPECT_EQ(celix_version_getMajor(celix_versionRange_getLowVersion(range)), 0);
    EXPECT_EQ(celix_version_getMinor(celix_versionRange_getLowVersion(range)), 0);
    EXPECT_EQ(celix_version_getMicro(celix_versionRange_getLowVersion(range)), 0);
    EXPECT_EQ(std::string{celix_version_getQualifier(celix_versionRange_getLowVersion(range))}, std::string{""});
    EXPECT_EQ(nullptr, celix_versionRange_getHighVersion(range));

    celix_versionRange_destroy(range);
    celix_version_destroy(version);
}

TEST_F(VersionRangeTestSuite, IsInRangeTest) {
    celix_version_t* version = celix_version_create(1, 2, 3, nullptr);

    {
        celix_version_t* low = (celix_version_t*) calloc(1, sizeof(*low));
        low->major = 1;
        low->minor = 2;
        low->micro = 3;
        low->qualifier = nullptr;

        celix_version_t* high = (celix_version_t*) calloc(1, sizeof(*high));
        high->major = 1;
        high->minor = 2;
        high->micro = 3;
        high->qualifier = nullptr;

        celix_version_range_t* range = celix_versionRange_createVersionRange(low, true, high, true);
        EXPECT_TRUE(range != nullptr);
        EXPECT_TRUE(celix_versionRange_isInRange(range, version));
        celix_versionRange_destroy(range);
    }

    {
        celix_version_t* low = (celix_version_t*) calloc(1, sizeof(*low));
        low->major = 1;
        low->minor = 2;
        low->micro = 3;

        celix_version_range_t* range = celix_versionRange_createVersionRange(low, true, nullptr, true);
        EXPECT_TRUE(range != nullptr);
        EXPECT_TRUE(celix_versionRange_isInRange(range, version));
        celix_versionRange_destroy(range);
    }

    {
        celix_version_t* low = (celix_version_t*) calloc(1, sizeof(*low));
        low->major = 1;
        low->minor = 2;
        low->micro = 3;

        celix_version_t* high = (celix_version_t*) calloc(1, sizeof(*high));
        high->major = 1;
        high->minor = 2;
        high->micro = 3;

        celix_version_range_t* range = celix_versionRange_createVersionRange(low, false, high, true);
        EXPECT_TRUE(range != nullptr);
        EXPECT_FALSE(celix_versionRange_isInRange(range, version));

        celix_versionRange_destroy(range);
    }

    {
        celix_version_t* low = (celix_version_t*) calloc(1, sizeof(*low));
        low->major = 1;
        low->minor = 2;
        low->micro = 3;

        celix_version_t* high = (celix_version_t*) calloc(1, sizeof(*high));
        high->major = 1;
        high->minor = 2;
        high->micro = 3;

        celix_version_range_t* range = celix_versionRange_createVersionRange(low, true, high, false);
        EXPECT_TRUE(range != nullptr);
        EXPECT_FALSE(celix_versionRange_isInRange(range, version));

        celix_versionRange_destroy(range);
    }

    {
        celix_version_t* low = (celix_version_t*) calloc(1, sizeof(*low));
        low->major = 1;
        low->minor = 2;
        low->micro = 3;

        celix_version_t* high = (celix_version_t*) calloc(1, sizeof(*high));
        high->major = 1;
        high->minor = 2;
        high->micro = 3;

        celix_version_range_t* range = celix_versionRange_createVersionRange(low, false, high, false);
        EXPECT_TRUE(range != nullptr);
        EXPECT_FALSE(celix_versionRange_isInRange(range, version));

        celix_versionRange_destroy(range);
    }

    celix_version_destroy(version);
}

TEST_F(VersionRangeTestSuite, ParseTest) {
    const char* version = "[1.2.3,7.8.9]";
    celix_version_range_t* range = celix_versionRange_parse(version);
    EXPECT_TRUE(range != nullptr);
    celix_versionRange_destroy(range);

    version = "[1.2.3";
    EXPECT_TRUE(celix_versionRange_parse(version) == nullptr);

    EXPECT_TRUE(celix_versionRange_parse("[2.2.3,a.b.c)") == nullptr);
}

TEST_F(VersionRangeTestSuite, CreateLdapFilterInclusiveBothTest) {
    celix_version_t* low = (celix_version_t*) calloc(1, sizeof(*low));
    low->major = 1;
    low->minor = 2;
    low->micro = 3;
    low->qualifier = nullptr;

    celix_version_t* high = (celix_version_t*) calloc(1, sizeof(*high));
    high->major = 1;
    high->minor = 2;
    high->micro = 3;
    high->qualifier = nullptr;

    celix_version_range_t* range = celix_versionRange_createVersionRange(low, true, high, true);
    EXPECT_TRUE(range != nullptr);

    auto filter = celix_versionRange_createLDAPFilter(range, "service.version");
    EXPECT_EQ(std::string{filter}, std::string{"(&(service.version>=1.2.3)(service.version<=1.2.3))"});

    celix_versionRange_destroy(range);
    free(filter);
}

TEST_F(VersionRangeTestSuite, CreateLdapFilterInclusiveLowTest) {
    celix_version_t* low = (celix_version_t*) calloc(1, sizeof(*low));
    low->major = 1;
    low->minor = 2;
    low->micro = 3;
    low->qualifier = nullptr;

    celix_version_t* high = (celix_version_t*) calloc(1, sizeof(*high));
    high->major = 1;
    high->minor = 2;
    high->micro = 3;
    high->qualifier = nullptr;

    celix_version_range_t* range = celix_versionRange_createVersionRange(low, false, high, true);
    EXPECT_TRUE(range != nullptr);

    auto filter = celix_versionRange_createLDAPFilter(range, "service.version");
    EXPECT_STREQ(filter, "(&(service.version>1.2.3)(service.version<=1.2.3))");

    celix_versionRange_destroy(range);
    free(filter);
}

TEST_F(VersionRangeTestSuite, CreateLdapFilterInclusiveHighTest) {
    celix_version_t* low = (celix_version_t*) calloc(1, sizeof(*low));
    low->major = 1;
    low->minor = 2;
    low->micro = 3;
    low->qualifier = nullptr;

    celix_version_t* high = (celix_version_t*) calloc(1, sizeof(*high));
    high->major = 1;
    high->minor = 2;
    high->micro = 3;
    high->qualifier = nullptr;

    celix_version_range_t* range = celix_versionRange_createVersionRange(low, true, high, false);
    EXPECT_TRUE(range != nullptr);

    auto filter = celix_versionRange_createLDAPFilter(range, "service.version");
    EXPECT_STREQ(filter, "(&(service.version>=1.2.3)(service.version<1.2.3))");

    celix_versionRange_destroy(range);
    free(filter);
}

TEST_F(VersionRangeTestSuite, CreateLdapFilterExclusiveBothTest) {
    celix_version_t* low = (celix_version_t*) calloc(1, sizeof(*low));
    low->major = 1;
    low->minor = 2;
    low->micro = 3;
    low->qualifier = nullptr;

    celix_version_t* high = (celix_version_t*) calloc(1, sizeof(*high));
    high->major = 1;
    high->minor = 2;
    high->micro = 3;
    high->qualifier = nullptr;

    celix_version_range_t* range = celix_versionRange_createVersionRange(low, false, high, false);
    EXPECT_TRUE(range != nullptr);

    auto filter = celix_versionRange_createLDAPFilter(range, "service.version");
    EXPECT_STREQ(filter, "(&(service.version>1.2.3)(service.version<1.2.3))");

    celix_versionRange_destroy(range);
    free(filter);
}

TEST_F(VersionRangeTestSuite, CreateLdapFilterInfiniteTest) {
    celix_version_t* low = (celix_version_t*) calloc(1, sizeof(*low));
    low->major = 1;
    low->minor = 2;
    low->micro = 3;
    low->qualifier = nullptr;

    celix_version_range_t* range = celix_versionRange_createVersionRange(low, true, nullptr, true);

    auto filter = celix_versionRange_createLDAPFilter(range, "service.version");
    EXPECT_STREQ(filter, "(&(service.version>=1.2.3))");

    celix_versionRange_destroy(range);
    free(filter);
}

TEST_F(VersionRangeTestSuite, CreateLdapFilterInPlaceInclusiveBothTest) {
    celix_version_t* low = (celix_version_t*) calloc(1, sizeof(*low));
    low->major = 1;
    low->minor = 2;
    low->micro = 3;
    low->qualifier = nullptr;

    celix_version_t* high = (celix_version_t*) calloc(1, sizeof(*high));
    high->major = 1;
    high->minor = 2;
    high->micro = 3;
    high->qualifier = nullptr;

    celix_version_range_t* range = celix_versionRange_createVersionRange(low, true, high, true);
    char buffer[100];
    int bufferLen = sizeof(buffer) / sizeof(buffer[0]);

    EXPECT_EQ(1, celix_versionRange_createLDAPFilterInPlace(range, "service.version", buffer, bufferLen));

    EXPECT_STREQ(buffer, "(&(service.version>=1.2.3)(service.version<=1.2.3))");

    celix_versionRange_destroy(range);
}

TEST_F(VersionRangeTestSuite, CreateLdapFilterInPlaceInclusiveLowTest) {
    celix_version_t* low = (celix_version_t*) calloc(1, sizeof(*low));
    low->major = 1;
    low->minor = 2;
    low->micro = 3;
    low->qualifier = nullptr;

    celix_version_t* high = (celix_version_t*) calloc(1, sizeof(*high));
    high->major = 1;
    high->minor = 2;
    high->micro = 3;
    high->qualifier = nullptr;

    celix_version_range_t* range = celix_versionRange_createVersionRange(low, true, high, false);
    EXPECT_TRUE(range != nullptr);
    char buffer[100];
    int bufferLen = sizeof(buffer) / sizeof(buffer[0]);

    EXPECT_EQ(1, celix_versionRange_createLDAPFilterInPlace(range, "service.version", buffer, bufferLen));

    EXPECT_STREQ(buffer, "(&(service.version>=1.2.3)(service.version<1.2.3))");

    celix_versionRange_destroy(range);
}

TEST_F(VersionRangeTestSuite, CreateLdapFilterInPlaceInclusiveHighTest) {
    celix_version_t* low = (celix_version_t*) calloc(1, sizeof(*low));
    low->major = 1;
    low->minor = 2;
    low->micro = 3;
    low->qualifier = nullptr;

    celix_version_t* high = (celix_version_t*) calloc(1, sizeof(*high));
    high->major = 1;
    high->minor = 2;
    high->micro = 3;
    high->qualifier = nullptr;

    celix_version_range_t* range = celix_versionRange_createVersionRange(low, false, high, true);
    EXPECT_TRUE(range != nullptr);
    char buffer[100];
    int bufferLen = sizeof(buffer) / sizeof(buffer[0]);

    EXPECT_EQ(1, celix_versionRange_createLDAPFilterInPlace(range, "service.version", buffer, bufferLen));

    EXPECT_STREQ(buffer, "(&(service.version>1.2.3)(service.version<=1.2.3))");

    celix_versionRange_destroy(range);
}

TEST_F(VersionRangeTestSuite, CreateLdapFilterInPlaceExclusiveBothTest) {
    celix_version_t* low = (celix_version_t*) calloc(1, sizeof(*low));
    low->major = 1;
    low->minor = 2;
    low->micro = 3;
    low->qualifier = nullptr;

    celix_version_t* high = (celix_version_t*) calloc(1, sizeof(*high));
    high->major = 1;
    high->minor = 2;
    high->micro = 3;
    high->qualifier = nullptr;

    celix_version_range_t* range = celix_versionRange_createVersionRange(low, false, high, false);
    EXPECT_TRUE(range != nullptr);
    char buffer[100];
    int bufferLen = sizeof(buffer) / sizeof(buffer[0]);

    EXPECT_EQ(1, celix_versionRange_createLDAPFilterInPlace(range, "service.version", buffer, bufferLen));

    EXPECT_STREQ(buffer, "(&(service.version>1.2.3)(service.version<1.2.3))");

    celix_versionRange_destroy(range);
}

TEST_F(VersionRangeTestSuite, CreateLdapFilterInPlaceInfiniteHighTest) {
    celix_version_t* low = (celix_version_t*) calloc(1, sizeof(*low));
    low->major = 1;
    low->minor = 2;
    low->micro = 3;
    low->qualifier = nullptr;

    celix_version_range_t* range = celix_versionRange_createVersionRange(low, false, nullptr, false);
    char buffer[100];
    int bufferLen = sizeof(buffer) / sizeof(buffer[0]);

    EXPECT_EQ(1, celix_versionRange_createLDAPFilterInPlace(range, "service.version", buffer, bufferLen));

    EXPECT_STREQ(buffer, "(&(service.version>1.2.3))");

    celix_versionRange_destroy(range);
}

TEST_F(VersionRangeTestSuite, CreateLdapFilterWithQualifierTest) {
    celix_autoptr(celix_version_t) low = celix_version_create(1, 2, 2, "0");
    celix_autoptr(celix_version_t) high = celix_version_create(1, 2, 2, "0");

    celix_autoptr(celix_version_range_t) range = celix_versionRange_createVersionRange(celix_steal_ptr(low), true,
                                                                                       celix_steal_ptr(high), true);
    celix_autofree char* filter = celix_versionRange_createLDAPFilter(range, "service.version");
    EXPECT_STREQ(filter, "(&(service.version>=1.2.2.0)(service.version<=1.2.2.0))");
}
