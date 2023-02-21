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


#include "string.h"
#include <stdlib.h>
#include <string.h>

#include "CppUTest/TestHarness.h"
#include "CppUTest/TestHarness_c.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "celix_utils.h"

extern "C"
{
#include "utils.h"
}

int main(int argc, char** argv) {
    MemoryLeakWarningPlugin::turnOffNewDeleteOverloads();
    return RUN_ALL_TESTS(argc, argv);
}

TEST_GROUP(utils) {

    void setup(void) {
    }

    void teardown() {
    }
};

static char* my_strdup(const char* s){
    if(s==NULL){
        return NULL;
    }

    size_t len = strlen(s);

    char *d = (char*) calloc (len + 1,sizeof(char));

    if (d == NULL){
        return NULL;
    }

    strncpy (d,s,len);
    return d;
}

TEST(utils, stringHash) {
    char * toHash = my_strdup("abc");
    unsigned int hash;
    hash = utils_stringHash((void *) toHash);
    LONGS_EQUAL(193485963, hash);

    free(toHash);
    toHash = my_strdup("abc123def456ghi789jkl012mno345pqr678stu901vwx234yz");
    hash = utils_stringHash((void *) toHash);
    LONGS_EQUAL(1532304168, hash);

    free(toHash);
    toHash = my_strdup("abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz");
    hash = utils_stringHash((void *) toHash);
    LONGS_EQUAL(3721605959, hash);
    free(toHash);

    hash = utils_stringHash(NULL);
    LONGS_EQUAL(0, hash);
}

TEST(utils, stringEquals) {
    // Compare with equal strings
    char * org = my_strdup("abc");
    char * cmp = my_strdup("abc");

    int result = utils_stringEquals((void *) org, (void *) cmp);
    CHECK(result);

    // Compare with no equal strings
    free(cmp);
    cmp = my_strdup("abcd");

    result = utils_stringEquals((void *) org, (void *) cmp);
    CHECK_FALSE(result);

    // Compare with numeric strings
    free(org);
    free(cmp);
    org = my_strdup("123");
    cmp = my_strdup("123");

    result = utils_stringEquals((void *) org, (void *) cmp);
    CHECK(result);

    // Compare with long strings
    free(org);
    free(cmp);
    org = my_strdup("abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz");
    cmp = my_strdup("abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
            "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz");

    result = utils_stringEquals((void *) org, (void *) cmp);
    CHECK(result);

    free(org);
    free(cmp);
}

TEST(utils, string_ndup){
    // Compare with equal strings
    const char * org = "abc";
    char * cmp = NULL;

    cmp = string_ndup(org, 3);
    STRCMP_EQUAL(org, cmp);
    free(cmp);

    org = "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz";
    cmp = string_ndup(org, 50);
    STRCMP_EQUAL(org, cmp);
    free(cmp);

    cmp = string_ndup(org, 25);
    LONGS_EQUAL(25, strlen(cmp));
    free(cmp);
}

TEST(utils, stringTrim) {
    // Multiple whitespaces, before, after and in between
    char * toTrim = my_strdup(" a b c ");
    char * result = utils_stringTrim(toTrim);

    STRCMP_EQUAL("a b c", result);

    // No whitespaces
    free(toTrim);
    toTrim = my_strdup("abc");
    result = utils_stringTrim(toTrim);

    STRCMP_EQUAL("abc", result);

    // Only whitespace before
    free(toTrim);
    toTrim = my_strdup("               abc");
    result = utils_stringTrim(toTrim);

    STRCMP_EQUAL("abc", result);

    // Only whitespace after
    free(toTrim);
    toTrim = my_strdup("abc         ");
    result = utils_stringTrim(toTrim);

    STRCMP_EQUAL("abc", result);

    // Whitespace other than space (tab, cr..).
    free(toTrim);
    toTrim = my_strdup("\tabc  \n asdf  \n");
    result = utils_stringTrim(toTrim);

    STRCMP_EQUAL("abc  \n asdf", result);

    free(toTrim);

    char* trimmed = celix_utils_trim("  abc   ");
    STRCMP_EQUAL("abc", trimmed);
    free(trimmed);
}

TEST(utils, thread_equalsSelf){
    celix_thread thread = celixThread_self();
    bool get;

    LONGS_EQUAL(CELIX_SUCCESS, thread_equalsSelf(thread, &get));
    CHECK(get);

    thread.thread = (pthread_t) 0x42;
    LONGS_EQUAL(CELIX_SUCCESS, thread_equalsSelf(thread, &get));
    CHECK_FALSE(get);
}

TEST(utils, isNumeric) {
    // Check numeric string
    char * toCheck = my_strdup("42");

    bool result;
    celix_status_t status = utils_isNumeric(toCheck, &result);
    LONGS_EQUAL(CELIX_SUCCESS, status);
    CHECK_C(result);

    // Check non numeric string
    free(toCheck);
    toCheck = my_strdup("42b");
    status = utils_isNumeric(toCheck, &result);
    LONGS_EQUAL(CELIX_SUCCESS, status);
    CHECK_C(!result);

    free(toCheck);
}

TEST(utils, compareServiceIdsAndRanking){
    int ret;
    //service 1 is higher ranked and has a irrelevant ID, so result is -1 (smaller -> sorted before service 2)
    ret = celix_utils_compareServiceIdsAndRanking(2,2,1,1);
    LONGS_EQUAL(-1, ret);

    //service 1 is equally ranked and has a lower ID. so result is -1 (smaller -> sorted before service 2)
    ret = celix_utils_compareServiceIdsAndRanking(1,1,2,1);
    LONGS_EQUAL(-1, ret);

    //service 1 is equally ranked and has a higher ID, so result is 1 (larger -> sorted after service 2)
    ret = celix_utils_compareServiceIdsAndRanking(2,1,1,1);
    LONGS_EQUAL(1, ret);

    //service 1 is lower ranked and has a irrelevant ID, so result is -1 (larger -> sorted after service 2)
    ret = celix_utils_compareServiceIdsAndRanking(1,1,2,2);
    LONGS_EQUAL(1, ret);

    //service 1 is equal in ID and irrelevantly ranked
    //note ensure that also the call without celix_ is tested
    ret = utils_compareServiceIdsAndRanking(1,1,1,1);
    LONGS_EQUAL(0, ret);


}

TEST(utils, extractLocalNameAndNamespaceTest) {
    const char *input = "lb";
    char* name = NULL;
    char *ns = NULL;
    celix_utils_extractLocalNameAndNamespaceFromFullyQualifiedName(input, "::", &name, &ns);
    CHECK_EQUAL_C_STRING("lb", name);
    CHECK_TRUE(ns == NULL);
    free(name);
    free(ns);

    input = "celix::lb";
    name = NULL;
    ns = NULL;
    celix_utils_extractLocalNameAndNamespaceFromFullyQualifiedName(input, "::", &name, &ns);
    CHECK_EQUAL_C_STRING("lb", name);
    CHECK_EQUAL_C_STRING("celix", ns);
    free(name);
    free(ns);

    input = "celix::extra::namespace::entries::lb";
    name = NULL;
    ns = NULL;
    celix_utils_extractLocalNameAndNamespaceFromFullyQualifiedName(input, "::", &name, &ns);
    CHECK_EQUAL_C_STRING("lb", name);
    CHECK_EQUAL_C_STRING("celix::extra::namespace::entries", ns);
    free(name);
    free(ns);

    input = "celix.extra.namespace.entries.lb";
    name = NULL;
    ns = NULL;
    celix_utils_extractLocalNameAndNamespaceFromFullyQualifiedName(input, ".", &name, &ns);
    CHECK_EQUAL_C_STRING("lb", name);
    CHECK_EQUAL_C_STRING("celix.extra.namespace.entries", ns);
    free(name);
    free(ns);

    //testing with non existing namespace
    input = "celix.extra.namespace.entries.lb";
    name = NULL;
    ns = NULL;
    celix_utils_extractLocalNameAndNamespaceFromFullyQualifiedName(input, "??", &name, &ns);
    CHECK_EQUAL_C_STRING("celix.extra.namespace.entries.lb", name);
    CHECK_TRUE(ns == NULL);
    free(name);
    free(ns);

    //wrong input check
    input = NULL;
    name = NULL;
    ns = NULL;
    celix_utils_extractLocalNameAndNamespaceFromFullyQualifiedName(input, "??", &name, &ns);
    CHECK_TRUE(name == NULL);
    CHECK_TRUE(ns == NULL);
    free(name);
    free(ns);


    //empty namespace check
    input = "celix.extra.namespace.entries.lb";
    name = NULL;
    ns = NULL;
    celix_utils_extractLocalNameAndNamespaceFromFullyQualifiedName(input, "", &name, &ns);
    CHECK_EQUAL_C_STRING("celix.extra.namespace.entries.lb", name);
    CHECK_TRUE(ns == NULL);
    free(name);
    free(ns);
}

TEST(utils, isStringNullOrEmpty) {
    bool empty = celix_utils_isStringNullOrEmpty(nullptr);
    CHECK_TRUE(empty);
    empty = celix_utils_isStringNullOrEmpty("");
    CHECK_TRUE(empty);
    empty = celix_utils_isStringNullOrEmpty(" ");
    CHECK_FALSE(empty);
    empty = celix_utils_isStringNullOrEmpty("foo");
    CHECK_FALSE(empty);
}