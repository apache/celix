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
/**
 * version_range_test.cpp
 *
 *  \date       Dec 18, 2012
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include "CppUTest/TestHarness.h"
#include "CppUTest/TestHarness_c.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "string.h"

extern "C"
{
#include "version_range_private.h"
#include "version_private.h"
}

int main(int argc, char** argv) {
    MemoryLeakWarningPlugin::turnOffNewDeleteOverloads();
    return RUN_ALL_TESTS(argc, argv);
}

static char* my_strdup(const char* s) {
    if (s == NULL) {
        return NULL;
    }

    size_t len = strlen(s);

    char *d = (char*) calloc(len + 1, sizeof(char));

    if (d == NULL) {
        return NULL;
    }

    strncpy(d, s, len);
    return d;
}

//----------------------TESTGROUP DEFINES----------------------

TEST_GROUP(version_range) {

    void setup(void) {
    }

    void teardown() {
    }
};

TEST(version_range, create) {
    celix_status_t status = CELIX_SUCCESS;
    version_range_pt range = NULL;
    version_pt low = (version_pt) calloc(1, sizeof(*low));
    version_pt high = (version_pt) calloc(1, sizeof(*high));

    status = versionRange_createVersionRange(low, false, high, true, &range);
    LONGS_EQUAL(CELIX_SUCCESS, status);
    CHECK_C((range != NULL));
    LONGS_EQUAL(true, range->isHighInclusive);
    LONGS_EQUAL(false, range->isLowInclusive);
    POINTERS_EQUAL(low, range->low);
    POINTERS_EQUAL(high, range->high);

    versionRange_destroy(range);
}

TEST(version_range, createInfinite) {
    celix_status_t status = CELIX_SUCCESS;
    version_range_pt range = NULL;
    version_pt version = (version_pt) calloc(1, sizeof(*version));
    version->major = 1;
    version->minor = 2;
    version->micro = 3;

    status = versionRange_createInfiniteVersionRange(&range);
    LONGS_EQUAL(CELIX_SUCCESS, status);
    CHECK_C(range != NULL);
    LONGS_EQUAL(true, range->isHighInclusive);
    LONGS_EQUAL(true, range->isLowInclusive);
    LONGS_EQUAL(range->low->major, 0);
    LONGS_EQUAL(range->low->minor, 0);
    LONGS_EQUAL(range->low->micro, 0);
    STRCMP_EQUAL(range->low->qualifier, "");
    POINTERS_EQUAL(NULL, range->high);

    versionRange_destroy(range);
    free(version);
}

TEST(version_range, isInRange) {
    bool result;
    version_pt version = (version_pt) calloc(1, sizeof(*version));
    version->major = 1;
    version->minor = 2;
    version->micro = 3;

    {
        version_range_pt range = NULL;

        version_pt low = (version_pt) calloc(1, sizeof(*low));
        low->major = 1;
        low->minor = 2;
        low->micro = 3;
        low->qualifier = NULL;

        version_pt high = (version_pt) calloc(1, sizeof(*high));
        high->major = 1;
        high->minor = 2;
        high->micro = 3;
        high->qualifier = NULL;

        LONGS_EQUAL(CELIX_SUCCESS, versionRange_createVersionRange(low, true, high, true, &range));
        LONGS_EQUAL(CELIX_SUCCESS, versionRange_isInRange(range, version, &result));
        LONGS_EQUAL(true, result);

        versionRange_destroy(range);
    }

    {
        version_range_pt range = NULL;

        version_pt low = (version_pt) calloc(1, sizeof(*low));
        low->major = 1;
        low->minor = 2;
        low->micro = 3;

        LONGS_EQUAL(CELIX_SUCCESS, versionRange_createVersionRange(low, true, NULL, true, &range));
        LONGS_EQUAL(CELIX_SUCCESS, versionRange_isInRange(range, version, &result));
        LONGS_EQUAL(true, result);

        versionRange_destroy(range);
    }

    {
        version_range_pt range = NULL;

        version_pt low = (version_pt) calloc(1, sizeof(*low));
        low->major = 1;
        low->minor = 2;
        low->micro = 3;

        version_pt high = (version_pt) calloc(1, sizeof(*high));
        high->major = 1;
        high->minor = 2;
        high->micro = 3;

        LONGS_EQUAL(CELIX_SUCCESS, versionRange_createVersionRange(low, false, high, true, &range));
        LONGS_EQUAL(CELIX_SUCCESS, versionRange_isInRange(range, version, &result));
        LONGS_EQUAL(false, result);

        versionRange_destroy(range);
    }

    {
        version_range_pt range = NULL;

        version_pt low = (version_pt) calloc(1, sizeof(*low));
        low->major = 1;
        low->minor = 2;
        low->micro = 3;

        version_pt high = (version_pt) calloc(1, sizeof(*high));
        high->major = 1;
        high->minor = 2;
        high->micro = 3;

        LONGS_EQUAL(CELIX_SUCCESS, versionRange_createVersionRange(low, true, high, false, &range));
        LONGS_EQUAL(CELIX_SUCCESS, versionRange_isInRange(range, version, &result));
        LONGS_EQUAL(false, result);

        versionRange_destroy(range);
    }

    {
        version_range_pt range = NULL;

        version_pt low = (version_pt) calloc(1, sizeof(*low));
        low->major = 1;
        low->minor = 2;
        low->micro = 3;

        version_pt high = (version_pt) calloc(1, sizeof(*high));
        high->major = 1;
        high->minor = 2;
        high->micro = 3;

        LONGS_EQUAL(CELIX_SUCCESS, versionRange_createVersionRange(low, false, high, false, &range));
        LONGS_EQUAL(CELIX_SUCCESS, versionRange_isInRange(range, version, &result));
        LONGS_EQUAL(false, result);

        versionRange_destroy(range);
    }

    free(version);
}

TEST(version_range, parse) {
    version_range_pt range = NULL;
    version_pt low = (version_pt) calloc(1, sizeof(*low));
    version_pt high = (version_pt) calloc(1, sizeof(*high));
    char * version = my_strdup("[1.2.3,7.8.9]");
    low->major = 1;
    low->minor = 2;
    low->micro = 3;

    high->major = 7;
    high->minor = 8;
    high->micro = 9;

    LONGS_EQUAL(CELIX_SUCCESS, versionRange_parse(version, &range));

    versionRange_destroy(range);
    free(version);
    version = my_strdup("[1.2.3");

    LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, versionRange_parse(version, &range));

    free(version);

    free(high);
    free(low);
}

TEST(version_range, createLdapFilterInclusiveBoth) {
    version_pt low = (version_pt) calloc(1, sizeof(*low));
    low->major = 1;
    low->minor = 2;
    low->micro = 3;
    low->qualifier = NULL;

    version_pt high = (version_pt) calloc(1, sizeof(*high));
    high->major = 1;
    high->minor = 2;
    high->micro = 3;
    high->qualifier = NULL;

    version_range_pt range = NULL;
    LONGS_EQUAL(CELIX_SUCCESS, versionRange_createVersionRange(low, true, high, true, &range));

    auto filter = versionRange_createLDAPFilter(range, "service.version");
    STRCMP_EQUAL(filter, "(&(service.version>=1.2.3)(service.version<=1.2.3))");

    versionRange_destroy(range);
    free(filter);
}

TEST(version_range, createLdapFilterInclusiveLow) {
    version_pt low = (version_pt) calloc(1, sizeof(*low));
    low->major = 1;
    low->minor = 2;
    low->micro = 3;
    low->qualifier = NULL;

    version_pt high = (version_pt) calloc(1, sizeof(*high));
    high->major = 1;
    high->minor = 2;
    high->micro = 3;
    high->qualifier = NULL;

    version_range_pt range = NULL;
    LONGS_EQUAL(CELIX_SUCCESS, versionRange_createVersionRange(low, false, high, true, &range));

    auto filter = versionRange_createLDAPFilter(range, "service.version");
    STRCMP_EQUAL(filter, "(&(service.version>1.2.3)(service.version<=1.2.3))");

    versionRange_destroy(range);
    free(filter);
}

TEST(version_range, createLdapFilterInclusiveHigh) {
    version_pt low = (version_pt) calloc(1, sizeof(*low));
    low->major = 1;
    low->minor = 2;
    low->micro = 3;
    low->qualifier = NULL;

    version_pt high = (version_pt) calloc(1, sizeof(*high));
    high->major = 1;
    high->minor = 2;
    high->micro = 3;
    high->qualifier = NULL;

    version_range_pt range = NULL;
    LONGS_EQUAL(CELIX_SUCCESS, versionRange_createVersionRange(low, true, high, false, &range));

    auto filter = versionRange_createLDAPFilter(range, "service.version");
    STRCMP_EQUAL(filter, "(&(service.version>=1.2.3)(service.version<1.2.3))");

    versionRange_destroy(range);
    free(filter);
}

TEST(version_range, createLdapFilterExclusiveBoth) {
    version_pt low = (version_pt) calloc(1, sizeof(*low));
    low->major = 1;
    low->minor = 2;
    low->micro = 3;
    low->qualifier = NULL;

    version_pt high = (version_pt) calloc(1, sizeof(*high));
    high->major = 1;
    high->minor = 2;
    high->micro = 3;
    high->qualifier = NULL;

    version_range_pt range = NULL;
    LONGS_EQUAL(CELIX_SUCCESS, versionRange_createVersionRange(low, false, high, false, &range));

    auto filter = versionRange_createLDAPFilter(range, "service.version");
    STRCMP_EQUAL(filter, "(&(service.version>1.2.3)(service.version<1.2.3))");

    versionRange_destroy(range);
    free(filter);
}

TEST(version_range, createLdapFilterInfinite) {
    version_pt low = (version_pt) calloc(1, sizeof(*low));
    low->major = 1;
    low->minor = 2;
    low->micro = 3;
    low->qualifier = NULL;

    version_range_pt range = NULL;
    LONGS_EQUAL(CELIX_SUCCESS, versionRange_createVersionRange(low, true, NULL, true, &range));

    auto filter = versionRange_createLDAPFilter(range, "service.version");
    STRCMP_EQUAL(filter, "(&(service.version>=1.2.3))");

    versionRange_destroy(range);
    free(filter);
}

TEST(version_range, createLdapFilterInPlaceInclusiveBoth) {
    version_pt low = (version_pt) calloc(1, sizeof(*low));
    low->major = 1;
    low->minor = 2;
    low->micro = 3;
    low->qualifier = NULL;

    version_pt high = (version_pt) calloc(1, sizeof(*high));
    high->major = 1;
    high->minor = 2;
    high->micro = 3;
    high->qualifier = NULL;

    version_range_pt range = NULL;
    LONGS_EQUAL(CELIX_SUCCESS, versionRange_createVersionRange(low, true, high, true, &range));
    char buffer[100];
    int bufferLen = sizeof(buffer) / sizeof(buffer[0]);

    LONGS_EQUAL(1, versionRange_createLDAPFilterInPlace(range, "service.version", buffer, bufferLen));

    STRCMP_EQUAL(buffer, "(&(service.version>=1.2.3)(service.version<=1.2.3))");

    versionRange_destroy(range);
}

TEST(version_range, createLdapFilterInPlaceInclusiveLow) {
    version_pt low = (version_pt) calloc(1, sizeof(*low));
    low->major = 1;
    low->minor = 2;
    low->micro = 3;
    low->qualifier = NULL;

    version_pt high = (version_pt) calloc(1, sizeof(*high));
    high->major = 1;
    high->minor = 2;
    high->micro = 3;
    high->qualifier = NULL;

    version_range_pt range = NULL;
    LONGS_EQUAL(CELIX_SUCCESS, versionRange_createVersionRange(low, true, high, false, &range));
    char buffer[100];
    int bufferLen = sizeof(buffer) / sizeof(buffer[0]);

    LONGS_EQUAL(1, versionRange_createLDAPFilterInPlace(range, "service.version", buffer, bufferLen));

    STRCMP_EQUAL(buffer, "(&(service.version>=1.2.3)(service.version<1.2.3))");

    versionRange_destroy(range);
}

TEST(version_range, createLdapFilterInPlaceInclusiveHigh) {
    version_pt low = (version_pt) calloc(1, sizeof(*low));
    low->major = 1;
    low->minor = 2;
    low->micro = 3;
    low->qualifier = NULL;

    version_pt high = (version_pt) calloc(1, sizeof(*high));
    high->major = 1;
    high->minor = 2;
    high->micro = 3;
    high->qualifier = NULL;

    version_range_pt range = NULL;
    LONGS_EQUAL(CELIX_SUCCESS, versionRange_createVersionRange(low, false, high, true, &range));
    char buffer[100];
    int bufferLen = sizeof(buffer) / sizeof(buffer[0]);

    LONGS_EQUAL(1, versionRange_createLDAPFilterInPlace(range, "service.version", buffer, bufferLen));

    STRCMP_EQUAL(buffer, "(&(service.version>1.2.3)(service.version<=1.2.3))");

    versionRange_destroy(range);
}

TEST(version_range, createLdapFilterInPlaceExclusiveBoth) {
    version_pt low = (version_pt) calloc(1, sizeof(*low));
    low->major = 1;
    low->minor = 2;
    low->micro = 3;
    low->qualifier = NULL;

    version_pt high = (version_pt) calloc(1, sizeof(*high));
    high->major = 1;
    high->minor = 2;
    high->micro = 3;
    high->qualifier = NULL;

    version_range_pt range = NULL;
    LONGS_EQUAL(CELIX_SUCCESS, versionRange_createVersionRange(low, false, high, false, &range));
    char buffer[100];
    int bufferLen = sizeof(buffer) / sizeof(buffer[0]);

    LONGS_EQUAL(1, versionRange_createLDAPFilterInPlace(range, "service.version", buffer, bufferLen));

    STRCMP_EQUAL(buffer, "(&(service.version>1.2.3)(service.version<1.2.3))");

    versionRange_destroy(range);
}

TEST(version_range, createLdapFilterInPlaceInfiniteHigh) {
    version_pt low = (version_pt) calloc(1, sizeof(*low));
    low->major = 1;
    low->minor = 2;
    low->micro = 3;
    low->qualifier = NULL;

    version_range_pt range = NULL;
    LONGS_EQUAL(CELIX_SUCCESS, versionRange_createVersionRange(low, false, NULL, false, &range));
    char buffer[100];
    int bufferLen = sizeof(buffer) / sizeof(buffer[0]);

    LONGS_EQUAL(1, versionRange_createLDAPFilterInPlace(range, "service.version", buffer, bufferLen));

    STRCMP_EQUAL(buffer, "(&(service.version>1.2.3))");

    versionRange_destroy(range);
}




