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
#include "celix_jansson_schema.h"
#include "celix_json_patch.h"
#include <gtest/gtest.h>
#include <cstring>

/* ── Patch Apply: add operations ──────────────────────────────────────── */

TEST(PatchApplyTest, AddRoot) {
    json_t* doc = json_loads("42", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);

    json_t* patch = json_array();
    json_t* op = json_object();
    json_object_set_new(op, "op", json_string("add"));
    json_object_set_new(op, "path", json_string(""));
    json_object_set_new(op, "value", json_integer(99));
    json_array_append_new(patch, op);

    json_t* result = celix_json_patch_apply(doc, patch);
    ASSERT_NE(nullptr, result);
    EXPECT_EQ(99, json_integer_value(result));

    json_decref(doc);
    json_decref(patch);
    json_decref(result);
}

TEST(PatchApplyTest, AddToObject) {
    json_t* doc = json_loads(R"({"a":1})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);

    json_t* patch = json_array();
    json_t* op = json_object();
    json_object_set_new(op, "op", json_string("add"));
    json_object_set_new(op, "path", json_string("/b"));
    json_object_set_new(op, "value", json_integer(2));
    json_array_append_new(patch, op);

    json_t* result = celix_json_patch_apply(doc, patch);
    ASSERT_NE(nullptr, result);
    json_t* expected = json_loads(R"({"a":1,"b":2})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, expected);
    EXPECT_TRUE(json_equal(result, expected));

    json_decref(doc);
    json_decref(patch);
    json_decref(result);
    json_decref(expected);
}

TEST(PatchApplyTest, AddNested) {
    json_t* doc = json_loads(R"({"x":{}})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);

    json_t* patch = json_array();
    json_t* op = json_object();
    json_object_set_new(op, "op", json_string("add"));
    json_object_set_new(op, "path", json_string("/x/y/z"));
    json_object_set_new(op, "value", json_string("deep"));
    json_array_append_new(patch, op);

    json_t* result = celix_json_patch_apply(doc, patch);
    ASSERT_NE(nullptr, result);
    json_t* expected = json_loads(R"({"x":{"y":{"z":"deep"}}})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, expected);
    EXPECT_TRUE(json_equal(result, expected));

    json_decref(doc);
    json_decref(patch);
    json_decref(result);
    json_decref(expected);
}

/* ── Patch Apply: replace operations ──────────────────────────────────── */

TEST(PatchApplyTest, ReplaceExisting) {
    json_t* doc = json_loads(R"({"a":1,"b":2})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);

    json_t* patch = json_array();
    json_t* op = json_object();
    json_object_set_new(op, "op", json_string("replace"));
    json_object_set_new(op, "path", json_string("/b"));
    json_object_set_new(op, "value", json_integer(99));
    json_array_append_new(patch, op);

    json_t* result = celix_json_patch_apply(doc, patch);
    ASSERT_NE(nullptr, result);
    json_t* expected = json_loads(R"({"a":1,"b":99})", JSON_DECODE_ANY, nullptr);
    EXPECT_TRUE(json_equal(result, expected));

    json_decref(doc);
    json_decref(patch);
    json_decref(result);
    json_decref(expected);
}

TEST(PatchApplyTest, ReplaceInArray) {
    json_t* doc = json_loads(R"({"arr":[10,20,30]})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);

    json_t* patch = json_array();
    json_t* op = json_object();
    json_object_set_new(op, "op", json_string("replace"));
    json_object_set_new(op, "path", json_string("/arr/1"));
    json_object_set_new(op, "value", json_integer(99));
    json_array_append_new(patch, op);

    json_t* result = celix_json_patch_apply(doc, patch);
    ASSERT_NE(nullptr, result);
    json_t* expected = json_loads(R"({"arr":[10,99,30]})", JSON_DECODE_ANY, nullptr);
    EXPECT_TRUE(json_equal(result, expected));

    json_decref(doc);
    json_decref(patch);
    json_decref(result);
    json_decref(expected);
}

TEST(PatchApplyTest, ReplaceRoot) {
    json_t* doc = json_string("hello");
    json_t* patch = json_array();
    json_t* op = json_object();
    json_object_set_new(op, "op", json_string("replace"));
    json_object_set_new(op, "path", json_string(""));
    json_object_set_new(op, "value", json_string("world"));
    json_array_append_new(patch, op);

    json_t* result = celix_json_patch_apply(doc, patch);
    ASSERT_NE(nullptr, result);
    EXPECT_STREQ("world", json_string_value(result));

    json_decref(doc);
    json_decref(patch);
    json_decref(result);
}

/* ── Patch Apply: remove operations ───────────────────────────────────── */

TEST(PatchApplyTest, RemoveKey) {
    json_t* doc = json_loads(R"({"a":1,"b":2,"c":3})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);

    json_t* patch = json_array();
    json_t* op = json_object();
    json_object_set_new(op, "op", json_string("remove"));
    json_object_set_new(op, "path", json_string("/b"));
    json_array_append_new(patch, op);

    json_t* result = celix_json_patch_apply(doc, patch);
    ASSERT_NE(nullptr, result);
    json_t* expected = json_loads(R"({"a":1,"c":3})", JSON_DECODE_ANY, nullptr);
    EXPECT_TRUE(json_equal(result, expected));

    json_decref(doc);
    json_decref(patch);
    json_decref(result);
    json_decref(expected);
}

TEST(PatchApplyTest, RemoveFromArray) {
    json_t* doc = json_loads(R"({"arr":[10,20,30]})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);

    json_t* patch = json_array();
    json_t* op = json_object();
    json_object_set_new(op, "op", json_string("remove"));
    json_object_set_new(op, "path", json_string("/arr/1"));
    json_array_append_new(patch, op);

    json_t* result = celix_json_patch_apply(doc, patch);
    ASSERT_NE(nullptr, result);
    json_t* expected = json_loads(R"({"arr":[10,30]})", JSON_DECODE_ANY, nullptr);
    EXPECT_TRUE(json_equal(result, expected));

    json_decref(doc);
    json_decref(patch);
    json_decref(result);
    json_decref(expected);
}

TEST(PatchApplyTest, RemoveRoot) {
    json_t* doc = json_integer(42);
    json_t* patch = json_array();
    json_t* op = json_object();
    json_object_set_new(op, "op", json_string("remove"));
    json_object_set_new(op, "path", json_string(""));
    json_array_append_new(patch, op);

    json_t* result = celix_json_patch_apply(doc, patch);
    ASSERT_NE(nullptr, result);
    EXPECT_TRUE(json_is_null(result));

    json_decref(doc);
    json_decref(patch);
    json_decref(result);
}

/* ── Patch Apply: multiple operations ──────────────────────────────────── */

TEST(PatchApplyTest, MultipleOperations) {
    json_t* doc = json_loads(R"({"a":1,"b":2})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);

    json_t* patch = json_array();

    /* Add /c */
    json_t* op1 = json_object();
    json_object_set_new(op1, "op", json_string("add"));
    json_object_set_new(op1, "path", json_string("/c"));
    json_object_set_new(op1, "value", json_integer(3));
    json_array_append_new(patch, op1);

    /* Replace /a */
    json_t* op2 = json_object();
    json_object_set_new(op2, "op", json_string("replace"));
    json_object_set_new(op2, "path", json_string("/a"));
    json_object_set_new(op2, "value", json_integer(99));
    json_array_append_new(patch, op2);

    /* Remove /b */
    json_t* op3 = json_object();
    json_object_set_new(op3, "op", json_string("remove"));
    json_object_set_new(op3, "path", json_string("/b"));
    json_array_append_new(patch, op3);

    json_t* result = celix_json_patch_apply(doc, patch);
    ASSERT_NE(nullptr, result);
    json_t* expected = json_loads(R"({"a":99,"c":3})", JSON_DECODE_ANY, nullptr);
    EXPECT_TRUE(json_equal(result, expected));

    json_decref(doc);
    json_decref(patch);
    json_decref(result);
    json_decref(expected);
}

TEST(PatchApplyTest, SequenceDependent) {
    json_t* doc = json_loads(R"({"a":[1,2,3]})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);

    json_t* patch = json_array();
    /* Remove /a/1 first (removes 2), then the array is [1,3] */
    json_t* op1 = json_object();
    json_object_set_new(op1, "op", json_string("remove"));
    json_object_set_new(op1, "path", json_string("/a/1"));
    json_array_append_new(patch, op1);

    /* Replace /a/1 (now points to 3) with 99 */
    json_t* op2 = json_object();
    json_object_set_new(op2, "op", json_string("replace"));
    json_object_set_new(op2, "path", json_string("/a/1"));
    json_object_set_new(op2, "value", json_integer(99));
    json_array_append_new(patch, op2);

    json_t* result = celix_json_patch_apply(doc, patch);
    ASSERT_NE(nullptr, result);
    json_t* expected = json_loads(R"({"a":[1,99]})", JSON_DECODE_ANY, nullptr);
    EXPECT_TRUE(json_equal(result, expected));

    json_decref(doc);
    json_decref(patch);
    json_decref(result);
    json_decref(expected);
}

/* ── Edge cases ────────────────────────────────────────────────────────── */

TEST(PatchApplyTest, NullDocReturnsNull) {
    json_t* patch = json_array();
    json_t* result = celix_json_patch_apply(nullptr, patch);
    EXPECT_EQ(nullptr, result);
    json_decref(patch);
}

TEST(PatchApplyTest, NullPatchReturnsNull) {
    json_t* doc = json_integer(1);
    json_t* result = celix_json_patch_apply(doc, nullptr);
    EXPECT_EQ(nullptr, result);
    json_decref(doc);
}

TEST(PatchApplyTest, NonArrayPatchReturnsNull) {
    json_t* doc = json_integer(1);
    json_t* patch = json_string("bad");
    json_t* result = celix_json_patch_apply(doc, patch);
    EXPECT_EQ(nullptr, result);
    json_decref(doc);
    json_decref(patch);
}

TEST(PatchApplyTest, EmptyPatchReturnsCopy) {
    json_t* doc = json_loads(R"({"a":1})", JSON_DECODE_ANY, nullptr);
    json_t* patch = json_array();

    json_t* result = celix_json_patch_apply(doc, patch);
    ASSERT_NE(nullptr, result);
    EXPECT_TRUE(json_equal(doc, result));
    EXPECT_NE(doc, result); /* must be a copy, not the same object */

    json_decref(doc);
    json_decref(patch);
    json_decref(result);
}

TEST(PatchApplyTest, UnknownOpSkipped) {
    json_t* doc = json_object();
    json_t* patch = json_array();
    json_t* op = json_object();
    json_object_set_new(op, "op", json_string("move"));
    json_object_set_new(op, "path", json_string("/a"));
    json_object_set_new(op, "value", json_integer(1));
    json_array_append_new(patch, op);

    json_t* result = celix_json_patch_apply(doc, patch);
    ASSERT_NE(nullptr, result);
    json_t* expected = json_object();
    EXPECT_TRUE(json_equal(result, expected));

    json_decref(doc);
    json_decref(patch);
    json_decref(result);
    json_decref(expected);
}

TEST(PatchApplyTest, MissingOpField) {
    json_t* doc = json_object();
    json_t* patch = json_array();
    json_t* op = json_object();
    json_object_set_new(op, "path", json_string("/a"));
    json_object_set_new(op, "value", json_integer(1));
    json_array_append_new(patch, op);

    json_t* result = celix_json_patch_apply(doc, patch);
    ASSERT_NE(nullptr, result);
    /* No op field → skipped */
    EXPECT_TRUE(json_equal(result, doc));

    json_decref(doc);
    json_decref(patch);
    json_decref(result);
}

TEST(PatchApplyTest, MissingPathField) {
    json_t* doc = json_loads(R"({"a":1})", JSON_DECODE_ANY, nullptr);
    json_t* patch = json_array();
    json_t* op = json_object();
    json_object_set_new(op, "op", json_string("remove"));
    json_array_append_new(patch, op);

    json_t* result = celix_json_patch_apply(doc, patch);
    ASSERT_NE(nullptr, result);
    EXPECT_TRUE(json_equal(result, doc));

    json_decref(doc);
    json_decref(patch);
    json_decref(result);
}

/* ── Array-specific edge cases ─────────────────────────────────────────── */

TEST(PatchApplyTest, AddToArrayIndex) {
    json_t* doc = json_loads(R"({"arr":[1,2]})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);

    json_t* patch = json_array();
    json_t* op = json_object();
    json_object_set_new(op, "op", json_string("add"));
    json_object_set_new(op, "path", json_string("/arr/1"));
    json_object_set_new(op, "value", json_integer(99));
    json_array_append_new(patch, op);

    json_t* result = celix_json_patch_apply(doc, patch);
    ASSERT_NE(nullptr, result);
    json_t* expected = json_loads(R"({"arr":[1,99,2]})", JSON_DECODE_ANY, nullptr);
    EXPECT_TRUE(json_equal(result, expected));

    json_decref(doc);
    json_decref(patch);
    json_decref(result);
    json_decref(expected);
}

TEST(PatchApplyTest, AddToArrayAppend) {
    json_t* doc = json_loads(R"({"arr":[1,2]})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);

    json_t* patch = json_array();
    json_t* op = json_object();
    json_object_set_new(op, "op", json_string("add"));
    json_object_set_new(op, "path", json_string("/arr/2"));
    json_object_set_new(op, "value", json_integer(3));
    json_array_append_new(patch, op);

    json_t* result = celix_json_patch_apply(doc, patch);
    ASSERT_NE(nullptr, result);
    json_t* expected = json_loads(R"({"arr":[1,2,3]})", JSON_DECODE_ANY, nullptr);
    EXPECT_TRUE(json_equal(result, expected));

    json_decref(doc);
    json_decref(patch);
    json_decref(result);
    json_decref(expected);
}

TEST(PatchApplyTest, ReplaceNonExistentInArray) {
    json_t* doc = json_loads(R"({"arr":[1]})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);

    json_t* patch = json_array();
    json_t* op = json_object();
    json_object_set_new(op, "op", json_string("replace"));
    json_object_set_new(op, "path", json_string("/arr/5"));
    json_object_set_new(op, "value", json_integer(99));
    json_array_append_new(patch, op);

    json_t* result = celix_json_patch_apply(doc, patch);
    ASSERT_NE(nullptr, result);
    /* Non-existent index → operation skipped, document unchanged */
    EXPECT_TRUE(json_equal(result, doc));

    json_decref(doc);
    json_decref(patch);
    json_decref(result);
}

/* ── Original unchanged ────────────────────────────────────────────────── */

TEST(PatchApplyTest, OriginalUnchanged) {
    json_t* doc = json_loads(R"({"a":1})", JSON_DECODE_ANY, nullptr);
    json_t* patch = json_array();
    json_t* op = json_object();
    json_object_set_new(op, "op", json_string("add"));
    json_object_set_new(op, "path", json_string("/b"));
    json_object_set_new(op, "value", json_integer(2));
    json_array_append_new(patch, op);

    json_t* result = celix_json_patch_apply(doc, patch);
    ASSERT_NE(nullptr, result);
    EXPECT_FALSE(json_equal(doc, result)); /* doc should be unchanged */

    /* doc still has only "a" */
    json_t* a = json_object_get(doc, "a");
    ASSERT_NE(nullptr, a);
    EXPECT_EQ(1, json_integer_value(a));
    EXPECT_EQ(nullptr, json_object_get(doc, "b"));

    json_decref(doc);
    json_decref(patch);
    json_decref(result);
}

/* ── Remove array element at non-numeric index ─────────────────────────── */

TEST(PatchApplyTest, RemoveNonNumericArrayIndex) {
    json_t* doc = json_loads(R"({"arr":[1,2,3]})", JSON_DECODE_ANY, nullptr);
    json_t* patch = json_array();
    json_t* op = json_object();
    json_object_set_new(op, "op", json_string("remove"));
    json_object_set_new(op, "path", json_string("/arr/one"));
    json_array_append_new(patch, op);

    json_t* result = celix_json_patch_apply(doc, patch);
    ASSERT_NE(nullptr, result);
    /* Non-numeric index → skip, doc unchanged */
    EXPECT_TRUE(json_equal(result, doc));

    json_decref(doc);
    json_decref(patch);
    json_decref(result);
}
