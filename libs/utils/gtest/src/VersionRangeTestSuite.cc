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

#include "version.h"
#include "version_range.h" //NOTE testing celix_version_range through the deprecated version_range api.
#include "celix_stdlib_cleanup.h"
#include "celix_version_range.h"
#include "version_private.h"

class VersionRangeTestSuite : public ::testing::Test {};

TEST_F(VersionRangeTestSuite, create) {
    celix_status_t status = CELIX_SUCCESS;
    version_range_pt range = nullptr;
    version_pt low = celix_version_createEmptyVersion();
    version_pt high = celix_version_createEmptyVersion();

    status = versionRange_createVersionRange(low, false, high, true, &range);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_TRUE((range != nullptr));
    EXPECT_EQ(true, celix_versionRange_isHighInclusive(range));
    EXPECT_EQ(false, celix_versionRange_isLowInclusive(range));
    EXPECT_EQ(low, celix_versionRange_getLowVersion(range));
    EXPECT_EQ(high, celix_versionRange_getHighVersion(range));

    versionRange_destroy(range);
}

TEST_F(VersionRangeTestSuite, cleanup) {
    celix_autoptr(celix_version_range_t) range = celix_versionRange_createInfiniteVersionRange();
}

TEST_F(VersionRangeTestSuite, createInfinite) {
    celix_status_t status = CELIX_SUCCESS;
    version_range_pt range = nullptr;
    version_pt version = celix_version_create(1,2, 3, nullptr);

    status = versionRange_createInfiniteVersionRange(&range);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_TRUE(range != nullptr);
    EXPECT_EQ(true, celix_versionRange_isHighInclusive(range));
    EXPECT_EQ(true, celix_versionRange_isLowInclusive(range));
    EXPECT_EQ(celix_version_getMajor(celix_versionRange_getLowVersion(range)), 0);
    EXPECT_EQ(celix_version_getMinor(celix_versionRange_getLowVersion(range)), 0);
    EXPECT_EQ(celix_version_getMicro(celix_versionRange_getLowVersion(range)), 0);
    EXPECT_EQ(std::string{celix_version_getQualifier(celix_versionRange_getLowVersion(range))}, std::string{""});
    EXPECT_EQ(nullptr, celix_versionRange_getHighVersion(range));

    versionRange_destroy(range);
    celix_version_destroy(version);
}

TEST_F(VersionRangeTestSuite, isInRange) {
    bool result;
    version_pt version = celix_version_create(1, 2, 3, nullptr);

    {
        version_range_pt range = nullptr;

        version_pt low = (version_pt) calloc(1, sizeof(*low));
        low->major = 1;
        low->minor = 2;
        low->micro = 3;
        low->qualifier = nullptr;

        version_pt high = (version_pt) calloc(1, sizeof(*high));
        high->major = 1;
        high->minor = 2;
        high->micro = 3;
        high->qualifier = nullptr;

        EXPECT_EQ(CELIX_SUCCESS, versionRange_createVersionRange(low, true, high, true, &range));
        EXPECT_EQ(CELIX_SUCCESS, versionRange_isInRange(range, version, &result));
        EXPECT_EQ(true, result);

        versionRange_destroy(range);
    }

    {
        version_range_pt range = nullptr;

        version_pt low = (version_pt) calloc(1, sizeof(*low));
        low->major = 1;
        low->minor = 2;
        low->micro = 3;

        EXPECT_EQ(CELIX_SUCCESS, versionRange_createVersionRange(low, true, nullptr, true, &range));
        EXPECT_EQ(CELIX_SUCCESS, versionRange_isInRange(range, version, &result));
        EXPECT_EQ(true, result);

        versionRange_destroy(range);
    }

    {
        version_range_pt range = nullptr;

        version_pt low = (version_pt) calloc(1, sizeof(*low));
        low->major = 1;
        low->minor = 2;
        low->micro = 3;

        version_pt high = (version_pt) calloc(1, sizeof(*high));
        high->major = 1;
        high->minor = 2;
        high->micro = 3;

        EXPECT_EQ(CELIX_SUCCESS, versionRange_createVersionRange(low, false, high, true, &range));
        EXPECT_EQ(CELIX_SUCCESS, versionRange_isInRange(range, version, &result));
        EXPECT_EQ(false, result);

        versionRange_destroy(range);
    }

    {
        version_range_pt range = nullptr;

        version_pt low = (version_pt) calloc(1, sizeof(*low));
        low->major = 1;
        low->minor = 2;
        low->micro = 3;

        version_pt high = (version_pt) calloc(1, sizeof(*high));
        high->major = 1;
        high->minor = 2;
        high->micro = 3;

        EXPECT_EQ(CELIX_SUCCESS, versionRange_createVersionRange(low, true, high, false, &range));
        EXPECT_EQ(CELIX_SUCCESS, versionRange_isInRange(range, version, &result));
        EXPECT_EQ(false, result);

        versionRange_destroy(range);
    }

    {
        version_range_pt range = nullptr;

        version_pt low = (version_pt) calloc(1, sizeof(*low));
        low->major = 1;
        low->minor = 2;
        low->micro = 3;

        version_pt high = (version_pt) calloc(1, sizeof(*high));
        high->major = 1;
        high->minor = 2;
        high->micro = 3;

        EXPECT_EQ(CELIX_SUCCESS, versionRange_createVersionRange(low, false, high, false, &range));
        EXPECT_EQ(CELIX_SUCCESS, versionRange_isInRange(range, version, &result));
        EXPECT_EQ(false, result);

        versionRange_destroy(range);
    }

    celix_version_destroy(version);
}

TEST_F(VersionRangeTestSuite, parse) {
    version_range_pt range = nullptr;
    char * version = strdup("[1.2.3,7.8.9]");

    EXPECT_EQ(CELIX_SUCCESS, versionRange_parse(version, &range));

    versionRange_destroy(range);
    free(version);
    version = strdup("[1.2.3");

    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, versionRange_parse(version, &range));

    free(version);

    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, versionRange_parse("[2.2.3,a.b.c)", &range));
}

TEST_F(VersionRangeTestSuite, createLdapFilterInclusiveBoth) {
    version_pt low = (version_pt) calloc(1, sizeof(*low));
    low->major = 1;
    low->minor = 2;
    low->micro = 3;
    low->qualifier = nullptr;

    version_pt high = (version_pt) calloc(1, sizeof(*high));
    high->major = 1;
    high->minor = 2;
    high->micro = 3;
    high->qualifier = nullptr;

    version_range_pt range = nullptr;
    EXPECT_EQ(CELIX_SUCCESS, versionRange_createVersionRange(low, true, high, true, &range));

    auto filter = versionRange_createLDAPFilter(range, "service.version");
    EXPECT_EQ(std::string{filter}, std::string{"(&(service.version>=1.2.3)(service.version<=1.2.3))"});

    versionRange_destroy(range);
    free(filter);
}

TEST_F(VersionRangeTestSuite, createLdapFilterInclusiveLow) {
    version_pt low = (version_pt) calloc(1, sizeof(*low));
    low->major = 1;
    low->minor = 2;
    low->micro = 3;
    low->qualifier = nullptr;

    version_pt high = (version_pt) calloc(1, sizeof(*high));
    high->major = 1;
    high->minor = 2;
    high->micro = 3;
    high->qualifier = nullptr;

    version_range_pt range = nullptr;
    EXPECT_EQ(CELIX_SUCCESS, versionRange_createVersionRange(low, false, high, true, &range));

    auto filter = versionRange_createLDAPFilter(range, "service.version");
    EXPECT_STREQ(filter, "(&(service.version>1.2.3)(service.version<=1.2.3))");

    versionRange_destroy(range);
    free(filter);
}

TEST_F(VersionRangeTestSuite, createLdapFilterInclusiveHigh) {
    version_pt low = (version_pt) calloc(1, sizeof(*low));
    low->major = 1;
    low->minor = 2;
    low->micro = 3;
    low->qualifier = nullptr;

    version_pt high = (version_pt) calloc(1, sizeof(*high));
    high->major = 1;
    high->minor = 2;
    high->micro = 3;
    high->qualifier = nullptr;

    version_range_pt range = nullptr;
    EXPECT_EQ(CELIX_SUCCESS, versionRange_createVersionRange(low, true, high, false, &range));

    auto filter = versionRange_createLDAPFilter(range, "service.version");
    EXPECT_STREQ(filter, "(&(service.version>=1.2.3)(service.version<1.2.3))");

    versionRange_destroy(range);
    free(filter);
}

TEST_F(VersionRangeTestSuite, createLdapFilterExclusiveBoth) {
    version_pt low = (version_pt) calloc(1, sizeof(*low));
    low->major = 1;
    low->minor = 2;
    low->micro = 3;
    low->qualifier = nullptr;

    version_pt high = (version_pt) calloc(1, sizeof(*high));
    high->major = 1;
    high->minor = 2;
    high->micro = 3;
    high->qualifier = nullptr;

    version_range_pt range = nullptr;
    EXPECT_EQ(CELIX_SUCCESS, versionRange_createVersionRange(low, false, high, false, &range));

    auto filter = versionRange_createLDAPFilter(range, "service.version");
    EXPECT_STREQ(filter, "(&(service.version>1.2.3)(service.version<1.2.3))");

    versionRange_destroy(range);
    free(filter);
}

TEST_F(VersionRangeTestSuite, createLdapFilterInfinite) {
    version_pt low = (version_pt) calloc(1, sizeof(*low));
    low->major = 1;
    low->minor = 2;
    low->micro = 3;
    low->qualifier = nullptr;

    version_range_pt range = nullptr;
    EXPECT_EQ(CELIX_SUCCESS, versionRange_createVersionRange(low, true, nullptr, true, &range));

    auto filter = versionRange_createLDAPFilter(range, "service.version");
    EXPECT_STREQ(filter, "(&(service.version>=1.2.3))");

    versionRange_destroy(range);
    free(filter);
}

TEST_F(VersionRangeTestSuite, createLdapFilterInPlaceInclusiveBoth) {
    version_pt low = (version_pt) calloc(1, sizeof(*low));
    low->major = 1;
    low->minor = 2;
    low->micro = 3;
    low->qualifier = nullptr;

    version_pt high = (version_pt) calloc(1, sizeof(*high));
    high->major = 1;
    high->minor = 2;
    high->micro = 3;
    high->qualifier = nullptr;

    version_range_pt range = nullptr;
    EXPECT_EQ(CELIX_SUCCESS, versionRange_createVersionRange(low, true, high, true, &range));
    char buffer[100];
    int bufferLen = sizeof(buffer) / sizeof(buffer[0]);

    EXPECT_EQ(1, versionRange_createLDAPFilterInPlace(range, "service.version", buffer, bufferLen));

    EXPECT_STREQ(buffer, "(&(service.version>=1.2.3)(service.version<=1.2.3))");

    versionRange_destroy(range);
}

TEST_F(VersionRangeTestSuite, createLdapFilterInPlaceInclusiveLow) {
    version_pt low = (version_pt) calloc(1, sizeof(*low));
    low->major = 1;
    low->minor = 2;
    low->micro = 3;
    low->qualifier = nullptr;

    version_pt high = (version_pt) calloc(1, sizeof(*high));
    high->major = 1;
    high->minor = 2;
    high->micro = 3;
    high->qualifier = nullptr;

    version_range_pt range = nullptr;
    EXPECT_EQ(CELIX_SUCCESS, versionRange_createVersionRange(low, true, high, false, &range));
    char buffer[100];
    int bufferLen = sizeof(buffer) / sizeof(buffer[0]);

    EXPECT_EQ(1, versionRange_createLDAPFilterInPlace(range, "service.version", buffer, bufferLen));

    EXPECT_STREQ(buffer, "(&(service.version>=1.2.3)(service.version<1.2.3))");

    versionRange_destroy(range);
}

TEST_F(VersionRangeTestSuite, createLdapFilterInPlaceInclusiveHigh) {
    version_pt low = (version_pt) calloc(1, sizeof(*low));
    low->major = 1;
    low->minor = 2;
    low->micro = 3;
    low->qualifier = nullptr;

    version_pt high = (version_pt) calloc(1, sizeof(*high));
    high->major = 1;
    high->minor = 2;
    high->micro = 3;
    high->qualifier = nullptr;

    version_range_pt range = nullptr;
    EXPECT_EQ(CELIX_SUCCESS, versionRange_createVersionRange(low, false, high, true, &range));
    char buffer[100];
    int bufferLen = sizeof(buffer) / sizeof(buffer[0]);

    EXPECT_EQ(1, versionRange_createLDAPFilterInPlace(range, "service.version", buffer, bufferLen));

    EXPECT_STREQ(buffer, "(&(service.version>1.2.3)(service.version<=1.2.3))");

    versionRange_destroy(range);
}

TEST_F(VersionRangeTestSuite, createLdapFilterInPlaceExclusiveBoth) {
    version_pt low = (version_pt) calloc(1, sizeof(*low));
    low->major = 1;
    low->minor = 2;
    low->micro = 3;
    low->qualifier = nullptr;

    version_pt high = (version_pt) calloc(1, sizeof(*high));
    high->major = 1;
    high->minor = 2;
    high->micro = 3;
    high->qualifier = nullptr;

    version_range_pt range = nullptr;
    EXPECT_EQ(CELIX_SUCCESS, versionRange_createVersionRange(low, false, high, false, &range));
    char buffer[100];
    int bufferLen = sizeof(buffer) / sizeof(buffer[0]);

    EXPECT_EQ(1, versionRange_createLDAPFilterInPlace(range, "service.version", buffer, bufferLen));

    EXPECT_STREQ(buffer, "(&(service.version>1.2.3)(service.version<1.2.3))");

    versionRange_destroy(range);
}

TEST_F(VersionRangeTestSuite, createLdapFilterInPlaceInfiniteHigh) {
    version_pt low = (version_pt) calloc(1, sizeof(*low));
    low->major = 1;
    low->minor = 2;
    low->micro = 3;
    low->qualifier = nullptr;

    version_range_pt range = nullptr;
    EXPECT_EQ(CELIX_SUCCESS, versionRange_createVersionRange(low, false, nullptr, false, &range));
    char buffer[100];
    int bufferLen = sizeof(buffer) / sizeof(buffer[0]);

    EXPECT_EQ(1, versionRange_createLDAPFilterInPlace(range, "service.version", buffer, bufferLen));

    EXPECT_STREQ(buffer, "(&(service.version>1.2.3))");

    versionRange_destroy(range);
}

TEST_F(VersionRangeTestSuite, createLdapFilterWithQualifier) {
    celix_autoptr(celix_version_t) low = celix_version_create(1, 2, 2, "0");
    celix_autoptr(celix_version_t) high = celix_version_create(1, 2, 2, "0");

    celix_autoptr(celix_version_range_t) range = celix_versionRange_createVersionRange(celix_steal_ptr(low), true,
                                                                                       celix_steal_ptr(high), true);
    celix_autofree char* filter = celix_versionRange_createLDAPFilter(range, "service.version");
    EXPECT_STREQ(filter, "(&(service.version>=1.2.2.0)(service.version<=1.2.2.0))");
}
