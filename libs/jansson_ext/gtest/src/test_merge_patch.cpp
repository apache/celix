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
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#include "celix_json_merge_patch.h"
#include <gtest/gtest.h>

/* ── RFC 7396 examples ─────────────────────────────────────────────────────
 * Section 1, Section 3 and Appendix A examples, transcribed from the
 * nlohmann/json merge_patch test suite (tests/src/unit-merge_patch.cpp).
 * Ex1..Ex15 correspond to RFC 7396 Appendix A.1..A.15; A16 is missing from
 * the nlohmann suite and is covered here as well. */

static void applyAndExpectEqual(const char* targetText, const char* patchText, const char* expectedText) {
    json_auto_t* target = json_loads(targetText, JSON_DECODE_ANY, nullptr);
    json_auto_t* patch = json_loads(patchText, JSON_DECODE_ANY, nullptr);
    json_auto_t* expected = json_loads(expectedText, JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, target);
    ASSERT_NE(nullptr, patch);
    ASSERT_NE(nullptr, expected);

    json_auto_t* result = celix_json_merge_patch(target, patch);
    ASSERT_NE(nullptr, result);
    EXPECT_TRUE(json_equal(result, expected));
}

TEST(JsonMergePatchTest, Section1) {
    applyAndExpectEqual(
        R"({"a":"b","c":{"d":"e","f":"g"}})",
        R"({"a":"z","c":{"f":null}})",
        R"({"a":"z","c":{"d":"e"}})");
}

TEST(JsonMergePatchTest, Section3) {
    applyAndExpectEqual(
        R"({"title":"Goodbye!","author":{"givenName":"John","familyName":"Doe"},"tags":["example","sample"],"content":"This will be unchanged"})",
        R"({"title":"Hello!","phoneNumber":"+01-123-456-7890","author":{"familyName":null},"tags":["example"]})",
        R"({"title":"Hello!","author":{"givenName":"John"},"tags":["example"],"content":"This will be unchanged","phoneNumber":"+01-123-456-7890"})");
}

TEST(JsonMergePatchTest, Ex1) {
    applyAndExpectEqual(R"({"a":"b"})", R"({"a":"c"})", R"({"a":"c"})");
}

TEST(JsonMergePatchTest, Ex2) {
    applyAndExpectEqual(R"({"a":"b"})", R"({"b":"c"})", R"({"a":"b","b":"c"})");
}

TEST(JsonMergePatchTest, Ex3) {
    applyAndExpectEqual(R"({"a":"b"})", R"({"a":null})", "{}");
}

TEST(JsonMergePatchTest, Ex4) {
    applyAndExpectEqual(R"({"a":"b","b":"c"})", R"({"a":null})", R"({"b":"c"})");
}

TEST(JsonMergePatchTest, Ex5) {
    applyAndExpectEqual(R"({"a":["b"]})", R"({"a":"c"})", R"({"a":"c"})");
}

TEST(JsonMergePatchTest, Ex6) {
    applyAndExpectEqual(R"({"a":"c"})", R"({"a":["b"]})", R"({"a":["b"]})");
}

TEST(JsonMergePatchTest, Ex7) {
    applyAndExpectEqual(R"({"a":{"b":"c"}})", R"({"a":{"b":"d","c":null}})", R"({"a":{"b":"d"}})");
}

TEST(JsonMergePatchTest, Ex8) {
    applyAndExpectEqual(R"({"a":[{"b":"c"}]})", R"({"a":[1]})", R"({"a":[1]})");
}

TEST(JsonMergePatchTest, Ex9) {
    applyAndExpectEqual(R"(["a","b"])", R"(["c","d"])", R"(["c","d"])");
}

TEST(JsonMergePatchTest, Ex10) {
    applyAndExpectEqual(R"({"a":"b"})", R"(["c"])", R"(["c"])");
}

TEST(JsonMergePatchTest, Ex11) {
    applyAndExpectEqual(R"({"a":"foo"})", "null", "null");
}

TEST(JsonMergePatchTest, Ex12) {
    applyAndExpectEqual(R"({"a":"foo"})", R"("bar")", R"("bar")");
}

TEST(JsonMergePatchTest, Ex13) {
    applyAndExpectEqual(R"({"e":null})", R"({"a":1})", R"({"e":null,"a":1})");
}

TEST(JsonMergePatchTest, Ex14) {
    applyAndExpectEqual("[1,2]", R"({"a":"b","c":null})", R"({"a":"b"})");
}

TEST(JsonMergePatchTest, Ex15) {
    applyAndExpectEqual("{}", R"({"a":{"bb":{"ccc":null}}})", R"({"a":{"bb":{}}})");
}

TEST(JsonMergePatchTest, A16) {
    /* RFC 7396 Appendix A.16 (absent from the nlohmann suite) */
    applyAndExpectEqual(R"({"a":{"b":{}}})", R"({"a":{"b":null}})", R"({"a":{}})");
}

/* ── Edge cases ──────────────────────────────────────────────────────────── */

TEST(JsonMergePatchTest, NullArgs) {
    json_auto_t* doc = json_loads(R"({"a":1})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);

    EXPECT_EQ(nullptr, celix_json_merge_patch(nullptr, doc));
    EXPECT_EQ(nullptr, celix_json_merge_patch(doc, nullptr));
    EXPECT_EQ(nullptr, celix_json_merge_patch(nullptr, nullptr));
}

TEST(JsonMergePatchTest, EmptyPatch) {
    applyAndExpectEqual(R"({"a":1})", "{}", R"({"a":1})");
    /* an object patch converts a non-object target to {} even with no members */
    applyAndExpectEqual("[1,2]", "{}", "{}");
}

TEST(JsonMergePatchTest, AddMember) {
    applyAndExpectEqual("{}", R"({"a":"b"})", R"({"a":"b"})");
}

TEST(JsonMergePatchTest, ScalarAndBoolPatch) {
    applyAndExpectEqual(R"({"a":1})", "true", "true");
    applyAndExpectEqual(R"({"a":1})", "42", "42");
}

TEST(JsonMergePatchTest, DeepNesting) {
    applyAndExpectEqual(
        R"({"a":{"b":{"c":1},"x":[1,{"y":2}]}})",
        R"({"a":{"b":{"d":2},"x":[9]}})",
        R"({"a":{"b":{"c":1,"d":2},"x":[9]}})");
}

TEST(JsonMergePatchTest, InputsUnmodifiedAndNewDocument) {
    json_auto_t* target = json_loads(R"({"a":{"b":1},"c":[1,2]})", JSON_DECODE_ANY, nullptr);
    json_auto_t* patch = json_loads(R"({"a":{"b":2}})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, target);
    ASSERT_NE(nullptr, patch);
    json_auto_t* targetCopy = json_deep_copy(target);
    json_auto_t* patchCopy = json_deep_copy(patch);
    ASSERT_NE(nullptr, targetCopy);
    ASSERT_NE(nullptr, patchCopy);

    json_auto_t* result = celix_json_merge_patch(target, patch);
    ASSERT_NE(nullptr, result);

    /* a new document is returned and neither input is modified */
    EXPECT_NE(result, target);
    EXPECT_NE(result, patch);
    EXPECT_TRUE(json_equal(target, targetCopy));
    EXPECT_TRUE(json_equal(patch, patchCopy));
}

TEST(JsonMergePatchTest, SelfMerge) {
    /* self-merge with no null members is the identity */
    json_auto_t* doc = json_loads(R"({"a":{"b":1},"c":[1,2]})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);
    json_auto_t* copy = json_deep_copy(doc);
    ASSERT_NE(nullptr, copy);

    json_auto_t* result = celix_json_merge_patch(doc, doc);
    ASSERT_NE(nullptr, result);
    EXPECT_TRUE(json_equal(result, copy));
}

TEST(JsonMergePatchTest, PatchIsTargetSubobject) {
    /* the patch may alias a sub-object of the target: RFC 7396 treats the
     * patch as a document of its own, so its members are merged at the
     * top level */
    json_auto_t* target = json_loads(R"({"a":{"x":1,"y":2}})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, target);
    json_t* sub = json_object_get(target, "a");
    ASSERT_NE(nullptr, sub);
    json_auto_t* targetCopy = json_deep_copy(target);
    ASSERT_NE(nullptr, targetCopy);

    json_auto_t* result = celix_json_merge_patch(target, sub);
    ASSERT_NE(nullptr, result);
    json_auto_t* expected = json_loads(R"({"a":{"x":1,"y":2},"x":1,"y":2})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, expected);
    EXPECT_TRUE(json_equal(result, expected));
    /* the target itself is untouched */
    EXPECT_TRUE(json_equal(target, targetCopy));
}
