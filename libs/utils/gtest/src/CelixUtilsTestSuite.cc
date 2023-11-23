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

#include "celix_stdlib_cleanup.h"
#include "celix_utils.h"
#include "utils.h"

class UtilsTestSuite : public ::testing::Test {
public:
};


TEST_F(UtilsTestSuite, CompareServiceIdsAndRankingTest){
    int ret;
    //service 1 is higher ranked and has a irrelevant ID, so result is -1 (smaller -> sorted before service 2)
    ret = celix_utils_compareServiceIdsAndRanking(2,2,1,1);
    EXPECT_EQ(-1, ret);

    //service 1 is equally ranked and has a lower ID. so result is -1 (smaller -> sorted before service 2)
    ret = celix_utils_compareServiceIdsAndRanking(1,1,2,1);
    EXPECT_EQ(-1, ret);

    //service 1 is equally ranked and has a higher ID, so result is 1 (larger -> sorted after service 2)
    ret = celix_utils_compareServiceIdsAndRanking(2,1,1,1);
    EXPECT_EQ(1, ret);

    //service 1 is lower ranked and has a irrelevant ID, so result is -1 (larger -> sorted after service 2)
    ret = celix_utils_compareServiceIdsAndRanking(1,1,2,2);
    EXPECT_EQ(1, ret);

    //service 1 is equal in ID and irrelevantly ranked
    //note ensure that also the call without celix_ is tested
    ret = celix_utils_compareServiceIdsAndRanking(1,1,1,1);
    EXPECT_EQ(0, ret);


}

TEST_F(UtilsTestSuite, ExtractLocalNameAndNamespaceTest) {
    const char *input = "lb";
    char* name = nullptr;
    char *ns = nullptr;
    celix_utils_extractLocalNameAndNamespaceFromFullyQualifiedName(input, "::", &name, &ns);
    EXPECT_STREQ("lb", name);
    EXPECT_TRUE(ns == nullptr);
    free(name);
    free(ns);

    input = "celix::lb";
    name = nullptr;
    ns = nullptr;
    celix_utils_extractLocalNameAndNamespaceFromFullyQualifiedName(input, "::", &name, &ns);
    EXPECT_STREQ("lb", name);
    EXPECT_STREQ("celix", ns);
    free(name);
    free(ns);

    input = "celix::extra::namespace::entries::lb";
    name = nullptr;
    ns = nullptr;
    celix_utils_extractLocalNameAndNamespaceFromFullyQualifiedName(input, "::", &name, &ns);
    EXPECT_STREQ("lb", name);
    EXPECT_STREQ("celix::extra::namespace::entries", ns);
    free(name);
    free(ns);

    input = "celix.extra.namespace.entries.lb";
    name = nullptr;
    ns = nullptr;
    celix_utils_extractLocalNameAndNamespaceFromFullyQualifiedName(input, ".", &name, &ns);
    EXPECT_STREQ("lb", name);
    EXPECT_STREQ("celix.extra.namespace.entries", ns);
    free(name);
    free(ns);

    //testing with non existing namespace
    input = "celix.extra.namespace.entries.lb";
    name = nullptr;
    ns = nullptr;
    celix_utils_extractLocalNameAndNamespaceFromFullyQualifiedName(input, "??", &name, &ns);
    EXPECT_STREQ("celix.extra.namespace.entries.lb", name);
    EXPECT_TRUE(ns == nullptr);
    free(name);
    free(ns);

    //wrong input check
    input = nullptr;
    name = nullptr;
    ns = nullptr;
    celix_utils_extractLocalNameAndNamespaceFromFullyQualifiedName(input, "??", &name, &ns);
    EXPECT_TRUE(name == nullptr);
    EXPECT_TRUE(ns == nullptr);
    free(name);
    free(ns);


    //empty namespace check
    input = "celix.extra.namespace.entries.lb";
    name = nullptr;
    ns = nullptr;
    celix_utils_extractLocalNameAndNamespaceFromFullyQualifiedName(input, "", &name, &ns);
    EXPECT_STREQ("celix.extra.namespace.entries.lb", name);
    EXPECT_TRUE(ns == nullptr);
    free(name);
    free(ns);
}

TEST_F(UtilsTestSuite, IsStringnullptrOrEmptyTest) {
    bool empty = celix_utils_isStringNullOrEmpty(nullptr);
    EXPECT_TRUE(empty);
    empty = celix_utils_isStringNullOrEmpty("");
    EXPECT_TRUE(empty);
    empty = celix_utils_isStringNullOrEmpty(" ");
    EXPECT_FALSE(empty);
    empty = celix_utils_isStringNullOrEmpty("foo");
    EXPECT_FALSE(empty);
}

TEST_F(UtilsTestSuite, ContainsWhitespaceTest) {
    EXPECT_FALSE(celix_utils_containsWhitespace(nullptr));
    EXPECT_FALSE(celix_utils_containsWhitespace(""));
    EXPECT_TRUE(celix_utils_containsWhitespace(" "));
    EXPECT_FALSE(celix_utils_containsWhitespace("abcd"));
    EXPECT_TRUE(celix_utils_containsWhitespace("ab cd"));
    EXPECT_TRUE(celix_utils_containsWhitespace("ab\tcd"));
    EXPECT_TRUE(celix_utils_containsWhitespace("ab\ncd"));
    EXPECT_TRUE(celix_utils_containsWhitespace("abcd "));
}

TEST_F(UtilsTestSuite, MakeCIdentifierTest) {
    //When a string is already a c identifier the result is equal to the input
    char* id = celix_utils_makeCIdentifier("abcd");
    EXPECT_STREQ(id, "abcd");
    free(id);

    //When a string is already a c identifier the result is equal to the input
    id = celix_utils_makeCIdentifier("abcdABCD1234");
    EXPECT_STREQ(id, "abcdABCD1234");
    free(id);

    //When a string start with a digit a _ is prefixed
    id = celix_utils_makeCIdentifier("1234abcdABCD");
    EXPECT_STREQ(id, "_1234abcdABCD");
    free(id);

    //When a string contains non isalnum characters, those are replaced with _
    id = celix_utils_makeCIdentifier("$%ab \tcd^&");
    EXPECT_STREQ(id, "__ab__cd__");
    free(id);

    //When a string is empty or null, null will be returned
    EXPECT_EQ(celix_utils_makeCIdentifier(nullptr), nullptr);
    EXPECT_EQ(celix_utils_makeCIdentifier(""), nullptr);
}


TEST_F(UtilsTestSuite, StringHashTest) {


    unsigned long hash = celix_utils_stringHash("abc");
    EXPECT_EQ(193485963, hash);

    hash = celix_utils_stringHash("abc123def456ghi789jkl012mno345pqr678stu901vwx234yz");
    EXPECT_EQ(1532304168, hash);

    hash = celix_utils_stringHash(nullptr);
    EXPECT_EQ(0, hash);

    //test deprecated api
    hash = utils_stringHash("abc");
    EXPECT_EQ(193485963, hash);
}

TEST_F(UtilsTestSuite, StringEqualsTest) {
    //refactor StringEqualsTest to use gtest methods instead of cpputest
    EXPECT_TRUE(celix_utils_stringEquals("abc", "abc"));
    EXPECT_FALSE(celix_utils_stringEquals("abc", "def"));
    EXPECT_FALSE(celix_utils_stringEquals("abc", nullptr));
    EXPECT_FALSE(celix_utils_stringEquals(nullptr, "abc"));
    EXPECT_TRUE(celix_utils_stringEquals(nullptr, nullptr));
    EXPECT_TRUE(celix_utils_stringEquals("", ""));
    EXPECT_FALSE(celix_utils_stringEquals("", nullptr));

    //test deprecated api
    EXPECT_TRUE(utils_stringEquals("abc", "abc"));
}

TEST_F(UtilsTestSuite, IsNumericTest) {
    //test deprecated api

    // Check numeric string
    bool result;
    celix_status_t status = utils_isNumeric("42", &result);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_TRUE(result);

    // Check non numeric string
    status = utils_isNumeric("42b", &result);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_FALSE(result);
}

TEST_F(UtilsTestSuite, ThreadEqualSelfTest) {
    celix_thread thread = celixThread_self();
    bool equal;

    celix_status_t status = thread_equalsSelf(thread, &equal);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_TRUE(equal);


    thread.thread = (pthread_t) 0x42;
    status = thread_equalsSelf(thread, &equal);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_FALSE(equal);
}

TEST_F(UtilsTestSuite, StringTrimTest) {
    // Multiple whitespaces, before, after and in between
    auto* trimmed = celix_utils_trim(" a b c ");
    EXPECT_STREQ("a b c", trimmed);
    free(trimmed);

    // No whitespaces
    trimmed = celix_utils_trim("abc");
    EXPECT_STREQ("abc", trimmed);
    free(trimmed);

    // Only whitespace before
    trimmed = celix_utils_trim(" \tabc");
    EXPECT_STREQ("abc", trimmed);
    free(trimmed);

    // Only whitespace after
    trimmed = celix_utils_trim("abc \t");
    EXPECT_STREQ("abc", trimmed);
    free(trimmed);

    // Whitespace other than space (tab, cr..).
    trimmed = celix_utils_trim("\tabc  \n asdf  \n");
    EXPECT_STREQ("abc  \n asdf", trimmed);
    free(trimmed);

    // Empty string
    trimmed = celix_utils_trim("  abc   ");
    EXPECT_STREQ("abc", trimmed);
    free(trimmed);
}

TEST_F(UtilsTestSuite, StringNBdupTest){
    // test deprecated api

    // Compare with equal strings
    const char * org = "abc";
    char * cmp = nullptr;

    cmp = string_ndup(org, 3);
    EXPECT_STREQ(org, cmp);
    free(cmp);

    org = "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz";
    cmp = string_ndup(org, 50);
    EXPECT_STREQ(org, cmp);
    free(cmp);

    cmp = string_ndup(org, 25);
    EXPECT_EQ(25, strlen(cmp));
    free(cmp);
}

TEST_F(UtilsTestSuite, WriteOrCreateStringTest) {
    //Buffer big enough, write to stack buffer
    char buffer[16];
    char* out = celix_utils_writeOrCreateString(buffer, sizeof(buffer), "abc");
    EXPECT_EQ(buffer, out);
    EXPECT_STREQ("abc", out);
    celix_utils_freeStringIfNotEqual(buffer, out);

    //Buffer not big enough, malloc new string
    out = celix_utils_writeOrCreateString(buffer, sizeof(buffer), "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz");
    EXPECT_NE(buffer, out);
    EXPECT_STREQ("abc123def456ghi789jkl012mno345pqr678stu901vwx234yz", out);
    celix_utils_freeStringIfNotEqual(buffer, out);

    //same test, but with long, int and short format args
    //Buffer big enough, write to stack buffer
    char buffer2[64];
    char* out2 = celix_utils_writeOrCreateString(buffer2, sizeof(buffer2), "long %ld, int %d, short %hd", 1234567890L, 123456789, (short)12345);
    EXPECT_EQ(buffer2, out2);
    EXPECT_STREQ("long 1234567890, int 123456789, short 12345", out2);
    celix_utils_freeStringIfNotEqual(buffer2, out2);

    //Buffer not big enough, malloc new string
    out2 = celix_utils_writeOrCreateString(buffer2, sizeof(buffer2), "long %ld, int %d, short %hd. abc123def456ghi789jkl012mno345pqr678stu901vwx234yz", 1234567890123456789L, 123456789, (short)12345);
    EXPECT_NE(buffer2, out2);
    EXPECT_STREQ("long 1234567890123456789, int 123456789, short 12345. abc123def456ghi789jkl012mno345pqr678stu901vwx234yz", out2);
    celix_utils_freeStringIfNotEqual(buffer2, out2);
}

TEST_F(UtilsTestSuite, StrDupAndStrLenTest) {
    celix_autofree char* str = celix_utils_strdup("abc");
    ASSERT_NE(nullptr, str);
    EXPECT_STREQ("abc", str);

    size_t len = celix_utils_strlen(str);
    EXPECT_EQ(3, len);

    EXPECT_EQ(nullptr, celix_utils_strdup(nullptr));
}
