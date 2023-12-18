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

#include "dyn_common.h"
#include "celix_err.h"
#include <stdio.h>
#include <gtest/gtest.h>
#include <sys/queue.h>

class DynCommonTests : public ::testing::Test {
protected:
    char* result{nullptr};
    FILE* stream{nullptr};
    DynCommonTests() {
    }
    ~DynCommonTests() override {
        if (stream != nullptr) {
            fclose(stream);
        }
        if (result != nullptr) {
            free(result);
        }
    }
    // delete other constructors and assign operators
    DynCommonTests(DynCommonTests const&) = delete;
    DynCommonTests(DynCommonTests&&) = delete;
    DynCommonTests& operator=(DynCommonTests const&) = delete;
    DynCommonTests& operator=(DynCommonTests&&) = delete;
};

TEST_F(DynCommonTests, ParseValidName) {
    stream = fmemopen((void*)"valid_name", 10, "r");
    ASSERT_EQ(dynCommon_parseName(stream, &result), 0);
    ASSERT_STREQ(result, "valid_name");
}

TEST_F(DynCommonTests, ParseNameTillInvalidCharacter) {
    stream = fmemopen((void*)"invalid-name", 12, "r");
    ASSERT_EQ(dynCommon_parseName(stream, &result), 0);
    ASSERT_STREQ("invalid", result);
    ASSERT_EQ('-', fgetc(stream));
}

TEST_F(DynCommonTests, ParseNameWithExtraAllowableCharacters) {
    stream = fmemopen((void*)"invalid-@name", 13, "r");
    ASSERT_EQ(dynCommon_parseNameAlsoAccept(stream, "@-", &result), 0);
    ASSERT_STREQ("invalid-@name", result);
    ASSERT_EQ(EOF, fgetc(stream));

}

TEST_F(DynCommonTests, ParseEmptyName) {
    stream = fmemopen((void*)"", 1, "r");
    ASSERT_EQ(dynCommon_parseName(stream, &result), 1);
    ASSERT_EQ('\0', fgetc(stream));
}

TEST_F(DynCommonTests, EatCharValid) {
    stream = fmemopen((void*)"valid", 5, "r");
    ASSERT_EQ(dynCommon_eatChar(stream, 'v'), 0);
    ASSERT_EQ(dynCommon_eatChar(stream, 'a'), 0);
    ASSERT_EQ(dynCommon_eatChar(stream, 'l'), 0);
    ASSERT_EQ(dynCommon_eatChar(stream, 'i'), 0);
    ASSERT_EQ(dynCommon_eatChar(stream, 'd'), 0);
    ASSERT_FALSE(feof(stream));
    ASSERT_EQ(dynCommon_eatChar(stream, EOF), 0);
    ASSERT_TRUE(feof(stream));
}

TEST_F(DynCommonTests, EatCharFromEmptyString) {
    stream = fmemopen((void*)"", 1, "r");
    ASSERT_EQ(dynCommon_eatChar(stream, '\0'), 0);
    ASSERT_FALSE(feof(stream));
    ASSERT_EQ(dynCommon_eatChar(stream, EOF), 0);
    ASSERT_TRUE(feof(stream));
}

TEST_F(DynCommonTests, ParseNameValueSection) {
    stream = fmemopen((void*)"name1=value1\nname2=value2\n", 26, "r");
    struct namvals_head head;
    TAILQ_INIT(&head);
    ASSERT_EQ(dynCommon_parseNameValueSection(stream, &head), 0);

    struct namval_entry *entry = TAILQ_FIRST(&head);
    ASSERT_STREQ("name1", entry->name);
    ASSERT_STREQ("value1", entry->value);

    entry = TAILQ_NEXT(entry, entries);
    ASSERT_STREQ("name2", entry->name);
    ASSERT_STREQ("value2", entry->value);

    dynCommon_clearNamValHead(&head);
}

TEST_F(DynCommonTests, ParseNameValueSectionWithPairWithEmptyName) {
    stream = fmemopen((void*)"=value1\nname2=value2\n", 22, "r");
    struct namvals_head head;
    TAILQ_INIT(&head);
    ASSERT_EQ(dynCommon_parseNameValueSection(stream, &head), 1);
    ASSERT_STREQ("Parsed empty name", celix_err_popLastError());

    dynCommon_clearNamValHead(&head);
}

TEST_F(DynCommonTests, ParseNameValueSectionWithPairWithEmptyValue) {
    stream = fmemopen((void*)"name1=\nname2=value2\n", 22, "r");
    struct namvals_head head;
    TAILQ_INIT(&head);
    ASSERT_EQ(dynCommon_parseNameValueSection(stream, &head), 1);
    ASSERT_STREQ("Parsed empty name", celix_err_popLastError());

    dynCommon_clearNamValHead(&head);
}

TEST_F(DynCommonTests, ParseNameValueSectionWithPairMissingEquality) {
    stream = fmemopen((void*)"name1 value1\nname2=value2\n", 22, "r");
    struct namvals_head head;
    TAILQ_INIT(&head);
    ASSERT_EQ(dynCommon_parseNameValueSection(stream, &head), 1);
    ASSERT_STREQ("Error parsing, expected token '=' got ' ' at position 6", celix_err_popLastError());

    dynCommon_clearNamValHead(&head);
}

TEST_F(DynCommonTests, ParseNameValueSectionWithPairMissingNewline) {
    stream = fmemopen((void*)"name1=value1 name2=value2\n", 26, "r");
    struct namvals_head head;
    TAILQ_INIT(&head);
    ASSERT_EQ(dynCommon_parseNameValueSection(stream, &head), 1);
    ASSERT_STREQ("Error parsing, expected token '\n' got ' ' at position 13", celix_err_popLastError());

    dynCommon_clearNamValHead(&head);
}