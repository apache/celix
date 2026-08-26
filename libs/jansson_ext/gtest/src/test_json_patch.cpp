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
#include "celix_json_patch.h"
#include <gtest/gtest.h>
#include <cstring>

/* ── celix_json_patch_add ────────────────────────────────────────────────────── */

TEST(JsonPatchTest, AddOperation) {
    json_t* patch = json_array();
    json_t* val = json_integer(42);

    EXPECT_EQ(0, celix_json_patch_add(patch, "/x", val));
    EXPECT_EQ(1u, json_array_size(patch));

    json_t* op = json_array_get(patch, 0);
    EXPECT_STREQ("add", json_string_value(json_object_get(op, "op")));
    EXPECT_STREQ("/x", json_string_value(json_object_get(op, "path")));
    EXPECT_EQ(42, json_integer_value(json_object_get(op, "value")));

    json_decref(patch);
}

/* ── celix_json_patch_replace ────────────────────────────────────────────────── */

TEST(JsonPatchTest, ReplaceOperation) {
    json_t* patch = json_array();
    json_t* val = json_string("newval");

    EXPECT_EQ(0, celix_json_patch_replace(patch, "/a/b", val));
    EXPECT_EQ(1u, json_array_size(patch));

    json_t* op = json_array_get(patch, 0);
    EXPECT_STREQ("replace", json_string_value(json_object_get(op, "op")));
    EXPECT_STREQ("/a/b", json_string_value(json_object_get(op, "path")));
    EXPECT_STREQ("newval", json_string_value(json_object_get(op, "value")));

    json_decref(patch);
}

/* ── celix_json_patch_remove ─────────────────────────────────────────────────── */

TEST(JsonPatchTest, RemoveOperation) {
    json_t* patch = json_array();

    EXPECT_EQ(0, celix_json_patch_remove(patch, "/old/key"));
    EXPECT_EQ(1u, json_array_size(patch));

    json_t* op = json_array_get(patch, 0);
    EXPECT_STREQ("remove", json_string_value(json_object_get(op, "op")));
    EXPECT_STREQ("/old/key", json_string_value(json_object_get(op, "path")));
    /* remove has no "value" field */
    EXPECT_EQ(nullptr, json_object_get(op, "value"));

    json_decref(patch);
}

/* ── celix_json_patch_truncate ───────────────────────────────────────────────── */

TEST(JsonPatchTest, Truncate) {
    json_t* patch = json_array();
    celix_json_patch_add(patch, "/a", json_integer(1));
    celix_json_patch_add(patch, "/b", json_integer(2));
    celix_json_patch_add(patch, "/c", json_integer(3));
    EXPECT_EQ(3u, json_array_size(patch));

    celix_json_patch_truncate(patch, 1);
    EXPECT_EQ(1u, json_array_size(patch));

    json_t* op = json_array_get(patch, 0);
    EXPECT_STREQ("/a", json_string_value(json_object_get(op, "path")));

    json_decref(patch);
}

/* ── Edge cases: NULL / non-array inputs ──────────────────────────────── */

TEST(JsonPatchTest, NullInputs) {
    /* Steal semantics: the value is consumed even when the call fails on the
     * NULL/non-array guard — the caller must not touch it afterwards.
     * Leak-freedom is checked by LSan in the ASan CI build. */
    EXPECT_NE(0, celix_json_patch_add(nullptr, "/x", json_integer(1)));
    EXPECT_NE(0, celix_json_patch_replace(nullptr, "/x", json_integer(1)));
    EXPECT_NE(0, celix_json_patch_remove(nullptr, "/x"));

    json_t* non_array = json_string("not_an_array");
    EXPECT_NE(0, celix_json_patch_add(non_array, "/x", json_integer(1)));
    EXPECT_NE(0, celix_json_patch_replace(non_array, "/x", json_integer(1)));
    EXPECT_NE(0, celix_json_patch_remove(non_array, "/x"));
    json_decref(non_array);

    /* truncate with null/non-array is no-op (doesn't crash) */
    celix_json_patch_truncate(nullptr, 0);
    json_t* obj = json_object();
    celix_json_patch_truncate(obj, 5);
    json_decref(obj);
}

/* ── Apply: add at array positions ────────────────────────────────────── */

TEST(JsonPatchTest, ApplyAddArrayBegin) {
    json_t* doc = json_loads("[10,20]", JSON_DECODE_ANY, nullptr);
    json_t* patch = json_array();
    celix_json_patch_add(patch, "/0", json_integer(5));

    json_t* result = celix_json_patch_apply(doc, patch);
    ASSERT_NE(nullptr, result);
    EXPECT_EQ(5, json_integer_value(json_array_get(result, 0)));
    EXPECT_EQ(10, json_integer_value(json_array_get(result, 1)));
    EXPECT_EQ(20, json_integer_value(json_array_get(result, 2)));

    json_decref(doc);
    json_decref(patch);
    json_decref(result);
}

TEST(JsonPatchTest, ApplyAddArrayMiddle) {
    json_t* doc = json_loads("[10,20]", JSON_DECODE_ANY, nullptr);
    json_t* patch = json_array();
    celix_json_patch_add(patch, "/1", json_integer(15));

    json_t* result = celix_json_patch_apply(doc, patch);
    ASSERT_NE(nullptr, result);
    EXPECT_EQ(10, json_integer_value(json_array_get(result, 0)));
    EXPECT_EQ(15, json_integer_value(json_array_get(result, 1)));
    EXPECT_EQ(20, json_integer_value(json_array_get(result, 2)));

    json_decref(doc);
    json_decref(patch);
    json_decref(result);
}

/* ── Apply: multiple ops in one patch ──────────────────────────────────── */

TEST(JsonPatchTest, ApplyMixedOps) {
    json_t* doc = json_loads(R"({"users":[{"name":"A"},{"name":"B"}]})", JSON_DECODE_ANY, nullptr);
    json_t* patch = json_array();
    celix_json_patch_remove(patch, "/users/1");
    celix_json_patch_replace(patch, "/users/0/name", json_string("Alpha"));
    celix_json_patch_add(patch, "/count", json_integer(1));

    json_t* result = celix_json_patch_apply(doc, patch);
    ASSERT_NE(nullptr, result);

    json_t* expected = json_loads(R"({"users":[{"name":"Alpha"}],"count":1})", JSON_DECODE_ANY, nullptr);
    EXPECT_TRUE(json_equal(result, expected));

    json_decref(doc);
    json_decref(patch);
    json_decref(result);
    json_decref(expected);
}

/* ── Apply: remove last array element ─────────────────────────────────── */

TEST(JsonPatchTest, ApplyRemoveLastElement) {
    json_t* doc = json_loads("[1,2,3]", JSON_DECODE_ANY, nullptr);
    json_t* patch = json_array();
    celix_json_patch_remove(patch, "/2");

    json_t* result = celix_json_patch_apply(doc, patch);
    ASSERT_NE(nullptr, result);
    EXPECT_EQ(2u, json_array_size(result));
    EXPECT_EQ(1, json_integer_value(json_array_get(result, 0)));
    EXPECT_EQ(2, json_integer_value(json_array_get(result, 1)));

    json_decref(doc);
    json_decref(patch);
    json_decref(result);
}

/* ── Apply: replace nested deep value ─────────────────────────────────── */

TEST(JsonPatchTest, ApplyReplaceDeep) {
    json_t* doc = json_loads(R"({"a":{"b":{"c":1}}})", JSON_DECODE_ANY, nullptr);
    json_t* patch = json_array();
    celix_json_patch_replace(patch, "/a/b/c", json_integer(99));

    json_t* result = celix_json_patch_apply(doc, patch);
    ASSERT_NE(nullptr, result);

    json_t* expected = json_loads(R"({"a":{"b":{"c":99}}})", JSON_DECODE_ANY, nullptr);
    EXPECT_TRUE(json_equal(result, expected));

    json_decref(doc);
    json_decref(patch);
    json_decref(result);
    json_decref(expected);
}

/* ── Apply: add with numeric key in object path ───────────────────────── */

TEST(JsonPatchTest, ApplyAddNumericKey) {
    json_t* doc = json_object();
    json_t* patch = json_array();
    celix_json_patch_add(patch, "/123", json_string("numeric key"));

    json_t* result = celix_json_patch_apply(doc, patch);
    ASSERT_NE(nullptr, result);

    json_t* val = json_object_get(result, "123");
    ASSERT_NE(nullptr, val);
    EXPECT_STREQ("numeric key", json_string_value(val));

    json_decref(doc);
    json_decref(patch);
    json_decref(result);
}

/* ── Apply: ops with invalid path / missing value are skipped ─────────── */

TEST(JsonPatchTest, ApplyAddInvalidPath) {
    json_t* doc = json_loads(R"({})", JSON_DECODE_ANY, nullptr);
    /* path without leading '/' fails to parse as a JSON Pointer */
    json_t* patch = json_loads(R"([{"op":"add","path":"foo","value":5}])", JSON_DECODE_ANY, nullptr);

    json_t* result = celix_json_patch_apply(doc, patch);
    ASSERT_NE(nullptr, result);

    json_t* expected = json_loads(R"({})", JSON_DECODE_ANY, nullptr);
    EXPECT_TRUE(json_equal(result, expected));

    json_decref(doc);
    json_decref(patch);
    json_decref(result);
    json_decref(expected);
}

TEST(JsonPatchTest, ApplyMissingValue) {
    json_t* doc = json_loads(R"({})", JSON_DECODE_ANY, nullptr);
    /* add/replace ops without a "value" are skipped, later ops still apply */
    json_t* patch = json_loads(R"([{"op":"add","path":"/a"},{"op":"replace","path":"/b"},{"op":"add","path":"/c","value":1}])", JSON_DECODE_ANY, nullptr);

    json_t* result = celix_json_patch_apply(doc, patch);
    ASSERT_NE(nullptr, result);

    json_t* expected = json_loads(R"({"c":1})", JSON_DECODE_ANY, nullptr);
    EXPECT_TRUE(json_equal(result, expected));

    json_decref(doc);
    json_decref(patch);
    json_decref(result);
    json_decref(expected);
}

/* ── Apply: walk through array parents creating intermediate nodes ────── */

TEST(JsonPatchTest, ApplyAddThroughArrayCreatingNode) {
    json_t* doc = json_loads(R"({"arr":[]})", JSON_DECODE_ANY, nullptr);
    json_t* patch = json_array();
    celix_json_patch_add(patch, "/arr/2/x", json_integer(5));

    json_t* result = celix_json_patch_apply(doc, patch);
    ASSERT_NE(nullptr, result);

    /* gap filled with nulls, missing element created as object */
    json_t* expected = json_loads(R"({"arr":[null,null,{"x":5}]})", JSON_DECODE_ANY, nullptr);
    EXPECT_TRUE(json_equal(result, expected));

    json_decref(doc);
    json_decref(patch);
    json_decref(result);
    json_decref(expected);
}

TEST(JsonPatchTest, ApplyAddThroughArrayInvalidToken) {
    json_t* doc = json_loads(R"({"arr":[1,2]})", JSON_DECODE_ANY, nullptr);
    json_t* patch = json_array();
    celix_json_patch_add(patch, "/arr/foo/bar", json_integer(5));

    json_t* result = celix_json_patch_apply(doc, patch);
    ASSERT_NE(nullptr, result);

    /* non-numeric array token aborts the walk, op is dropped */
    json_t* expected = json_loads(R"({"arr":[1,2]})", JSON_DECODE_ANY, nullptr);
    EXPECT_TRUE(json_equal(result, expected));

    json_decref(doc);
    json_decref(patch);
    json_decref(result);
    json_decref(expected);
}

TEST(JsonPatchTest, ApplyAddBeyondArrayEnd) {
    json_t* doc = json_loads(R"({"arr":[10]})", JSON_DECODE_ANY, nullptr);
    json_t* patch = json_array();
    celix_json_patch_add(patch, "/arr/3", json_integer(5));

    json_t* result = celix_json_patch_apply(doc, patch);
    ASSERT_NE(nullptr, result);

    /* gap filled with nulls, value appended at the end */
    json_t* expected = json_loads(R"({"arr":[10,null,null,5]})", JSON_DECODE_ANY, nullptr);
    EXPECT_TRUE(json_equal(result, expected));

    json_decref(doc);
    json_decref(patch);
    json_decref(result);
    json_decref(expected);
}

TEST(JsonPatchTest, ApplyAddThroughScalarParent) {
    json_t* doc = json_loads(R"({"a":5})", JSON_DECODE_ANY, nullptr);
    json_t* patch = json_array();
    celix_json_patch_add(patch, "/a/b/c", json_integer(1));

    json_t* result = celix_json_patch_apply(doc, patch);
    ASSERT_NE(nullptr, result);

    /* scalar parent mid-walk → NULL → walk aborted, op is dropped */
    json_t* expected = json_loads(R"({"a":5})", JSON_DECODE_ANY, nullptr);
    EXPECT_TRUE(json_equal(result, expected));

    json_decref(doc);
    json_decref(patch);
    json_decref(result);
    json_decref(expected);
}

/* ── Apply: remove through array parents ──────────────────────────────── */

TEST(JsonPatchTest, ApplyRemoveThroughArray) {
    json_t* doc = json_loads(R"({"arr":[{"key":1},{"key":2}]})", JSON_DECODE_ANY, nullptr);
    json_t* patch = json_array();
    celix_json_patch_remove(patch, "/arr/0/key");

    json_t* result = celix_json_patch_apply(doc, patch);
    ASSERT_NE(nullptr, result);

    json_t* expected = json_loads(R"({"arr":[{},{"key":2}]})", JSON_DECODE_ANY, nullptr);
    EXPECT_TRUE(json_equal(result, expected));

    json_decref(doc);
    json_decref(patch);
    json_decref(result);
    json_decref(expected);
}

TEST(JsonPatchTest, ApplyRemoveThroughScalar) {
    json_t* doc = json_loads(R"({"a":5})", JSON_DECODE_ANY, nullptr);
    json_t* patch = json_array();
    celix_json_patch_remove(patch, "/a/b/c");

    json_t* result = celix_json_patch_apply(doc, patch);
    ASSERT_NE(nullptr, result);

    /* scalar parent mid-walk → NULL → walk aborted, op is dropped */
    json_t* expected = json_loads(R"({"a":5})", JSON_DECODE_ANY, nullptr);
    EXPECT_TRUE(json_equal(result, expected));

    json_decref(doc);
    json_decref(patch);
    json_decref(result);
    json_decref(expected);
}

TEST(JsonPatchTest, ApplyRemoveThroughArrayInvalidIndex) {
    json_t* doc = json_loads(R"({"arr":[{"key":1}]})", JSON_DECODE_ANY, nullptr);
    json_t* patch = json_array();
    celix_json_patch_remove(patch, "/arr/5/key");

    json_t* result = celix_json_patch_apply(doc, patch);
    ASSERT_NE(nullptr, result);

    /* out-of-range array index → parent NULL → op is dropped */
    json_t* expected = json_loads(R"({"arr":[{"key":1}]})", JSON_DECODE_ANY, nullptr);
    EXPECT_TRUE(json_equal(result, expected));

    json_decref(doc);
    json_decref(patch);
    json_decref(result);
    json_decref(expected);
}
