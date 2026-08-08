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
#include "celix_jansson_pointer.h"
#include <gtest/gtest.h>
#include <cstring>
#include <string>

/* ── Construction ──────────────────────────────────────────────────────── */

TEST(PointerTest, NewEmpty) {
    celix_json_pointer_t* p = celix_json_pointer_create("");
    ASSERT_NE(nullptr, p);
    EXPECT_EQ(0u, celix_json_pointer_depth(p));
    char* s = celix_json_pointer_to_string(p);
    EXPECT_STREQ("/", s);
    free(s);
    celix_json_pointer_destroy(p);
}

TEST(PointerTest, NullPointer) {
    celix_json_pointer_t* p = celix_json_pointer_create(NULL);
    ASSERT_NE(nullptr, p);
    EXPECT_EQ(0u, celix_json_pointer_depth(p));
    celix_json_pointer_destroy(p);
}

TEST(PointerTest, SimplePath) {
    celix_json_pointer_t* p = celix_json_pointer_create("/foo/bar");
    ASSERT_NE(nullptr, p);
    EXPECT_EQ(2u, celix_json_pointer_depth(p));
    EXPECT_STREQ("foo", celix_json_pointer_token(p, 0));
    EXPECT_STREQ("bar", celix_json_pointer_token(p, 1));
    EXPECT_EQ(nullptr, celix_json_pointer_token(p, 2));
    celix_json_pointer_destroy(p);
}

TEST(PointerTest, ArrayIndex) {
    celix_json_pointer_t* p = celix_json_pointer_create("/store/book/0/title");
    ASSERT_NE(nullptr, p);
    EXPECT_EQ(4u, celix_json_pointer_depth(p));
    EXPECT_STREQ("store", celix_json_pointer_token(p, 0));
    EXPECT_STREQ("book", celix_json_pointer_token(p, 1));
    EXPECT_STREQ("0", celix_json_pointer_token(p, 2));
    EXPECT_STREQ("title", celix_json_pointer_token(p, 3));
    celix_json_pointer_destroy(p);
}

TEST(PointerTest, MustStartWithSlash) {
    celix_json_pointer_t* p = celix_json_pointer_create("foo/bar");
    EXPECT_EQ(nullptr, p) << "Pointer without leading '/' should fail";
}

/* ── Escaping ──────────────────────────────────────────────────────────── */

TEST(PointerTest, EscapeTilde) {
    celix_json_pointer_t* p = celix_json_pointer_create("/~0foo/~1bar");
    ASSERT_NE(nullptr, p);
    EXPECT_STREQ("~foo", celix_json_pointer_token(p, 0));
    EXPECT_STREQ("/bar", celix_json_pointer_token(p, 1));
    celix_json_pointer_destroy(p);
}

TEST(PointerTest, EscapeUtilities) {
    char* escaped = celix_json_pointer_escape("a/b~c");
    EXPECT_STREQ("a~1b~0c", escaped);
    free(escaped);

    char* unescaped = celix_json_pointer_unescape("a~1b~0c");
    EXPECT_STREQ("a/b~c", unescaped);
    free(unescaped);
}

/* ── Round-trip ────────────────────────────────────────────────────────── */

TEST(PointerTest, RoundTrip) {
    const char* orig = "/foo~0bar/baz~1qux/x/y/z";
    celix_json_pointer_t* p = celix_json_pointer_create(orig);
    ASSERT_NE(nullptr, p);

    char* serialized = celix_json_pointer_to_string(p);
    EXPECT_STREQ(orig, serialized);

    /* Parse the serialized form again — should be identical */
    celix_json_pointer_t* p2 = celix_json_pointer_create(serialized);
    ASSERT_NE(nullptr, p2);
    EXPECT_EQ(celix_json_pointer_depth(p), celix_json_pointer_depth(p2));
    EXPECT_TRUE(celix_json_pointer_equals(p, p2));

    free(serialized);
    celix_json_pointer_destroy(p);
    celix_json_pointer_destroy(p2);
}

/* ── Mutation ──────────────────────────────────────────────────────────── */

TEST(PointerTest, PushAndPop) {
    celix_json_pointer_t* p = celix_json_pointer_create("/a");
    ASSERT_NE(nullptr, p);
    EXPECT_EQ(1u, celix_json_pointer_depth(p));

    celix_json_pointer_push(p, "b");
    EXPECT_EQ(2u, celix_json_pointer_depth(p));
    EXPECT_STREQ("b", celix_json_pointer_token(p, 1));

    celix_json_pointer_push(p, "c");
    EXPECT_EQ(3u, celix_json_pointer_depth(p));

    celix_json_pointer_pop(p);
    EXPECT_EQ(2u, celix_json_pointer_depth(p));
    EXPECT_STREQ("b", celix_json_pointer_token(p, 1));

    celix_json_pointer_pop(p);
    EXPECT_EQ(1u, celix_json_pointer_depth(p));

    celix_json_pointer_pop(p);
    celix_json_pointer_pop(p); /* extra pop should be safe */
    EXPECT_EQ(0u, celix_json_pointer_depth(p));

    celix_json_pointer_destroy(p);
}

/* ── Document resolution ───────────────────────────────────────────────── */

TEST(PointerTest, ResolveObject) {
    json_t* doc = json_loads(R"({"foo":{"bar":[1,2,3]}})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);

    celix_json_pointer_t* p = celix_json_pointer_create("/foo/bar/0");
    ASSERT_NE(nullptr, p);

    json_t* val = celix_json_pointer_get(doc, p);
    ASSERT_NE(nullptr, val);
    EXPECT_TRUE(json_is_integer(val));
    EXPECT_EQ(1, json_integer_value(val));

    celix_json_pointer_destroy(p);
    json_decref(doc);
}

TEST(PointerTest, ResolveNotFound) {
    json_t* doc = json_loads(R"({"a":1})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);

    celix_json_pointer_t* p = celix_json_pointer_create("/b");
    ASSERT_NE(nullptr, p);
    EXPECT_EQ(nullptr, celix_json_pointer_get(doc, p));

    celix_json_pointer_destroy(p);
    json_decref(doc);
}

TEST(PointerTest, RootPointer) {
    json_t* doc = json_loads("42", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);

    celix_json_pointer_t* p = celix_json_pointer_create("");
    ASSERT_NE(nullptr, p);

    json_t* val = celix_json_pointer_get(doc, p);
    ASSERT_NE(nullptr, val);
    EXPECT_TRUE(json_equal(doc, val));

    celix_json_pointer_destroy(p);
    json_decref(doc);
}

/* ── Set ───────────────────────────────────────────────────────────────── */

TEST(PointerTest, SetSimple) {
    json_t* doc = json_object();
    celix_json_pointer_t* p = celix_json_pointer_create("/a/b");
    ASSERT_NE(nullptr, p);

    EXPECT_EQ(0, celix_json_pointer_set(doc, p, json_integer(42)));

    /* Verify */
    json_t* v = celix_json_pointer_get(doc, p);
    ASSERT_NE(nullptr, v);
    EXPECT_EQ(42, json_integer_value(v));

    char* s = json_dumps(doc, JSON_COMPACT);
    EXPECT_STREQ("{\"a\":{\"b\":42}}", s);
    free(s);

    celix_json_pointer_destroy(p);
    json_decref(doc);
}

TEST(PointerTest, SetArrayIndex) {
    json_t* doc = json_object();
    celix_json_pointer_t* p = celix_json_pointer_create("/arr/2");
    ASSERT_NE(nullptr, p);

    EXPECT_EQ(0, celix_json_pointer_set(doc, p, json_string("hello")));

    char* s = json_dumps(doc, JSON_COMPACT);
    EXPECT_STREQ("{\"arr\":[null,null,\"hello\"]}", s);
    free(s);

    celix_json_pointer_destroy(p);
    json_decref(doc);
}

TEST(PointerTest, SetArrayAppend) {
    json_t* doc = json_loads(R"({"a":[1,2]})", JSON_DECODE_ANY, nullptr);
    celix_json_pointer_t* p = celix_json_pointer_create("/a/-");
    ASSERT_NE(nullptr, p);

    EXPECT_EQ(0, celix_json_pointer_set(doc, p, json_integer(3)));

    char* s = json_dumps(doc, JSON_COMPACT);
    EXPECT_STREQ("{\"a\":[1,2,3]}", s);
    free(s);

    celix_json_pointer_destroy(p);
    json_decref(doc);
}

TEST(PointerTest, SetArrayIndexNullValue) {
    json_t* doc = json_loads("[1,2]", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);
    celix_json_pointer_t* p = celix_json_pointer_create("/0");
    ASSERT_NE(nullptr, p);

    /* A NULL value cannot be inserted into an array; fails without modifying the document */
    EXPECT_EQ(-1, celix_json_pointer_set(doc, p, nullptr));

    char* s = json_dumps(doc, JSON_COMPACT);
    EXPECT_STREQ("[1,2]", s);
    free(s);

    celix_json_pointer_destroy(p);
    json_decref(doc);
}

TEST(PointerTest, SetArraySelfReferenceFails) {
    json_t* doc = json_loads("[1,2]", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);
    celix_json_pointer_t* p = celix_json_pointer_create("/0");
    ASSERT_NE(nullptr, p);
    json_incref(doc); /* set consumes a reference on failure — keep doc alive */

    /* An array cannot be inserted into itself; fails without modifying the document */
    EXPECT_EQ(-1, celix_json_pointer_set(doc, p, doc));

    char* s = json_dumps(doc, JSON_COMPACT);
    EXPECT_STREQ("[1,2]", s);
    free(s);

    celix_json_pointer_destroy(p);
    json_decref(doc);
}

/* ── Remove ────────────────────────────────────────────────────────────── */

TEST(PointerTest, RemoveKey) {
    json_t* doc = json_loads(R"({"a":1,"b":2})", JSON_DECODE_ANY, nullptr);
    celix_json_pointer_t* p = celix_json_pointer_create("/b");
    ASSERT_NE(nullptr, p);

    EXPECT_EQ(0, celix_json_pointer_remove(doc, p));

    char* s = json_dumps(doc, JSON_COMPACT);
    EXPECT_STREQ("{\"a\":1}", s);
    free(s);

    celix_json_pointer_destroy(p);
    json_decref(doc);
}

TEST(PointerTest, RemoveArrayElement) {
    json_t* doc = json_loads(R"({"arr":[1,2,3]})", JSON_DECODE_ANY, nullptr);
    celix_json_pointer_t* p = celix_json_pointer_create("/arr/1");
    ASSERT_NE(nullptr, p);

    EXPECT_EQ(0, celix_json_pointer_remove(doc, p));

    char* s = json_dumps(doc, JSON_COMPACT);
    EXPECT_STREQ("{\"arr\":[1,3]}", s);
    free(s);

    celix_json_pointer_destroy(p);
    json_decref(doc);
}

/* ── Parent ────────────────────────────────────────────────────────────── */

TEST(PointerTest, Parent) {
    celix_json_pointer_t* p = celix_json_pointer_create("/a/b/c");
    ASSERT_NE(nullptr, p);

    celix_json_pointer_t parent;
    memset(&parent, 0, sizeof(parent));
    celix_json_pointer_t* pp = celix_json_pointer_parent(p, &parent);
    ASSERT_NE(nullptr, pp);

    char* s = celix_json_pointer_to_string(pp);
    EXPECT_STREQ("/a/b", s);
    free(s);

    /* Parent of parent */
    celix_json_pointer_t grandparent;
    memset(&grandparent, 0, sizeof(grandparent));
    celix_json_pointer_t* gp = celix_json_pointer_parent(pp, &grandparent);
    ASSERT_NE(nullptr, gp);

    s = celix_json_pointer_to_string(gp);
    EXPECT_STREQ("/a", s);
    free(s);

    /* Parent of single token → root (empty) */
    celix_json_pointer_t root;
    memset(&root, 0, sizeof(root));
    celix_json_pointer_t* rp = celix_json_pointer_parent(gp, &root);
    ASSERT_NE(nullptr, rp);
    EXPECT_EQ(0u, celix_json_pointer_depth(rp)); /* empty pointer is the root */

    celix_json_pointer_clear(&parent);
    celix_json_pointer_clear(&grandparent);
    celix_json_pointer_destroy(p);
}

/* ── Concat ────────────────────────────────────────────────────────────── */

TEST(PointerTest, Concat) {
    celix_json_pointer_t* p = celix_json_pointer_create("/a");
    celix_json_pointer_t* suffix = celix_json_pointer_create("/b/c");

    ASSERT_EQ(0, celix_json_pointer_concat(p, suffix));

    char* s = celix_json_pointer_to_string(p);
    EXPECT_STREQ("/a/b/c", s);
    free(s);

    celix_json_pointer_destroy(p);
    celix_json_pointer_destroy(suffix);
}

/* ── Comparison ────────────────────────────────────────────────────────── */

TEST(PointerTest, Compare) {
    celix_json_pointer_t* a = celix_json_pointer_create("/a/b");
    celix_json_pointer_t* b = celix_json_pointer_create("/a/b");
    celix_json_pointer_t* c = celix_json_pointer_create("/a/c");

    EXPECT_TRUE(celix_json_pointer_equals(a, b));
    EXPECT_FALSE(celix_json_pointer_equals(a, c));
    EXPECT_FALSE(celix_json_pointer_equals(a, nullptr));

    celix_json_pointer_destroy(a);
    celix_json_pointer_destroy(b);
    celix_json_pointer_destroy(c);
}

/* ── Copy ──────────────────────────────────────────────────────────────── */

TEST(PointerTest, Copy) {
    celix_json_pointer_t* orig = celix_json_pointer_create("/foo/bar");
    celix_json_pointer_t* copy = celix_json_pointer_copy(orig);

    EXPECT_EQ(celix_json_pointer_depth(orig), celix_json_pointer_depth(copy));
    EXPECT_TRUE(celix_json_pointer_equals(orig, copy));

    /* Mutate copy — original should be unaffected */
    celix_json_pointer_pop(copy);
    EXPECT_EQ(2u, celix_json_pointer_depth(orig));
    EXPECT_EQ(1u, celix_json_pointer_depth(copy));

    celix_json_pointer_destroy(orig);
    celix_json_pointer_destroy(copy);
}

/* ── Stack allocation ──────────────────────────────────────────────────── */

TEST(PointerTest, StackAllocated) {
    celix_json_pointer_t ptr;
    memset(&ptr, 0, sizeof(ptr));

    EXPECT_EQ(0, celix_json_pointer_init(&ptr, "/a/b"));
    EXPECT_EQ(2u, celix_json_pointer_depth(&ptr));
    EXPECT_STREQ("a", celix_json_pointer_token(&ptr, 0));

    celix_json_pointer_push(&ptr, "c");
    EXPECT_EQ(3u, celix_json_pointer_depth(&ptr));

    celix_json_pointer_clear(&ptr);
}

/* ── get-or-create ─────────────────────────────────────────────────────── */

TEST(PointerTest, GetOrCreate) {
    json_t* doc = json_object();
    celix_json_pointer_t* p = celix_json_pointer_create("/x/y/z");
    ASSERT_NE(nullptr, p);

    json_t* node = celix_json_pointer_get_or_create(doc, p);
    ASSERT_NE(nullptr, node);
    json_decref(node); /* get-or-create returns a new reference */

    char* s = json_dumps(doc, JSON_COMPACT);
    EXPECT_STREQ("{\"x\":{\"y\":{\"z\":null}}}", s);
    free(s);

    celix_json_pointer_destroy(p);
    json_decref(doc);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Extended tests — inspired by nlohmann/json's unit-json_pointer.cpp
 * ═══════════════════════════════════════════════════════════════════════════ */

/* ── RFC 6901 §5 canonical fixture ───────────────────────────────────── */

TEST(PointerTest, Rfc6901Section5) {
    /* The example document from RFC 6901 §5 */
    const char* json_str = R"({
		"foo":  ["bar", "baz"],
		"":     0,
		"a/b":  1,
		"c%d":  2,
		"e^f":  3,
		"g|h":  4,
		"i\\j": 5,
		"k\"l": 6,
		" ":    7,
		"m~n":  8
	})";
    json_t* doc = json_loads(json_str, JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);

    /* Test all keys from the RFC example */
    auto check = [&](const char* ptr_str, json_t* expected, bool owned) {
        celix_json_pointer_t p;
        memset(&p, 0, sizeof(p));
        ASSERT_EQ(0, celix_json_pointer_init(&p, ptr_str)) << "Failed: " << ptr_str;
        json_t* v = celix_json_pointer_get(doc, &p);
        ASSERT_NE(nullptr, v) << "Missing: " << ptr_str;
        EXPECT_TRUE(json_equal(expected, v)) << "Mismatch at: " << ptr_str;
        celix_json_pointer_clear(&p);
        if (owned)
            json_decref(expected);
    };

    check("/foo", json_object_get(doc, "foo"), false);
    check("/foo/0", json_string("bar"), true);
    check("/", json_integer(0), true);
    check("/a~1b", json_integer(1), true);
    check("/c%d", json_integer(2), true);
    check("/e^f", json_integer(3), true);
    check("/g|h", json_integer(4), true);
    check("/i\\j", json_integer(5), true);
    check("/k\"l", json_integer(6), true);
    check("/ ", json_integer(7), true);
    check("/m~0n", json_integer(8), true);

    json_decref(doc);
}

/* ── Array index edge cases ──────────────────────────────────────────── */

TEST(PointerTest, LeadingZeroInvalid) {
    celix_json_pointer_t p;
    memset(&p, 0, sizeof(p));
    /* Leading zero in array index is invalid per RFC 6901 */
    EXPECT_NE(0, celix_json_pointer_init(&p, "/foo/01"));
}

TEST(PointerTest, NonNumericArrayIndex) {
    celix_json_pointer_t p;
    /* Non-numeric token used as array index — pointer parses but resolution fails */
    json_t* doc = json_loads("[1,2,3]", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);

    memset(&p, 0, sizeof(p));
    ASSERT_EQ(0, celix_json_pointer_init(&p, "/one"));
    EXPECT_EQ(nullptr, celix_json_pointer_get(doc, &p));

    celix_json_pointer_clear(&p);
    json_decref(doc);
}

TEST(PointerTest, ArithmeticInIndex) {
    json_t* doc = json_loads("[1,2,3]", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);

    celix_json_pointer_t p;
    memset(&p, 0, sizeof(p));
    ASSERT_EQ(0, celix_json_pointer_init(&p, "/1+1"));
    EXPECT_EQ(nullptr, celix_json_pointer_get(doc, &p));

    celix_json_pointer_clear(&p);
    json_decref(doc);
}

/* ── Invalid escape sequences ────────────────────────────────────────── */

TEST(PointerTest, InvalidEscapeSequence) {
    celix_json_pointer_t p;
    memset(&p, 0, sizeof(p));
    /* "~~" — stray tilde not followed by valid escape */
    EXPECT_NE(0, celix_json_pointer_init(&p, "/foo/~~"));
    celix_json_pointer_clear(&p);

    memset(&p, 0, sizeof(p));
    /* "~" at end — tilde with nothing after */
    EXPECT_NE(0, celix_json_pointer_init(&p, "/foo/~"));
    celix_json_pointer_clear(&p);

    memset(&p, 0, sizeof(p));
    /* "~2" — invalid escape character */
    EXPECT_NE(0, celix_json_pointer_init(&p, "/foo/~2"));
    celix_json_pointer_clear(&p);
}

/* ── Single-token array index ────────────────────────────────────────── */

TEST(PointerTest, ArrayAccessByIndex) {
    json_t* doc = json_loads("[10,20,30]", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);

    celix_json_pointer_t p;
    memset(&p, 0, sizeof(p));
    ASSERT_EQ(0, celix_json_pointer_init(&p, "/0"));
    json_t* v = celix_json_pointer_get(doc, &p);
    ASSERT_NE(nullptr, v);
    EXPECT_EQ(10, json_integer_value(v));
    celix_json_pointer_clear(&p);

    memset(&p, 0, sizeof(p));
    ASSERT_EQ(0, celix_json_pointer_init(&p, "/1"));
    v = celix_json_pointer_get(doc, &p);
    EXPECT_EQ(20, json_integer_value(v));
    celix_json_pointer_clear(&p);

    memset(&p, 0, sizeof(p));
    ASSERT_EQ(0, celix_json_pointer_init(&p, "/2"));
    v = celix_json_pointer_get(doc, &p);
    EXPECT_EQ(30, json_integer_value(v));
    celix_json_pointer_clear(&p);

    /* Out of bounds */
    memset(&p, 0, sizeof(p));
    ASSERT_EQ(0, celix_json_pointer_init(&p, "/3"));
    EXPECT_EQ(nullptr, celix_json_pointer_get(doc, &p));
    celix_json_pointer_clear(&p);

    json_decref(doc);
}

/* ── Nested array access ─────────────────────────────────────────────── */

TEST(PointerTest, NestedArrayAccess) {
    json_t* doc = json_loads("[[1,2],[3,4]]", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);

    celix_json_pointer_t p;
    memset(&p, 0, sizeof(p));
    ASSERT_EQ(0, celix_json_pointer_init(&p, "/0/1"));
    json_t* v = celix_json_pointer_get(doc, &p);
    ASSERT_NE(nullptr, v);
    EXPECT_EQ(2, json_integer_value(v));
    celix_json_pointer_clear(&p);

    memset(&p, 0, sizeof(p));
    ASSERT_EQ(0, celix_json_pointer_init(&p, "/1/0"));
    v = celix_json_pointer_get(doc, &p);
    ASSERT_NE(nullptr, v);
    EXPECT_EQ(3, json_integer_value(v));
    celix_json_pointer_clear(&p);

    json_decref(doc);
}

/* ── Set on existing key overwrites ──────────────────────────────────── */

TEST(PointerTest, SetOverwritesExisting) {
    json_t* doc = json_loads(R"({"a":1,"b":2})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);

    celix_json_pointer_t p;
    memset(&p, 0, sizeof(p));
    ASSERT_EQ(0, celix_json_pointer_init(&p, "/b"));

    EXPECT_EQ(0, celix_json_pointer_set(doc, &p, json_integer(99)));

    /* Verify overwrite */
    json_t* v = celix_json_pointer_get(doc, &p);
    ASSERT_NE(nullptr, v);
    EXPECT_EQ(99, json_integer_value(v));

    /* 'a' should still be 1 */
    celix_json_pointer_t pa;
    memset(&pa, 0, sizeof(pa));
    ASSERT_EQ(0, celix_json_pointer_init(&pa, "/a"));
    v = celix_json_pointer_get(doc, &pa);
    EXPECT_EQ(1, json_integer_value(v));

    celix_json_pointer_clear(&p);
    celix_json_pointer_clear(&pa);
    json_decref(doc);
}

/* ── Set array with gap fills nulls ──────────────────────────────────── */

TEST(PointerTest, SetArrayWithGap) {
    json_t* doc = json_loads(R"({"arr":[1,2]})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);

    celix_json_pointer_t p;
    memset(&p, 0, sizeof(p));
    ASSERT_EQ(0, celix_json_pointer_init(&p, "/arr/4"));

    EXPECT_EQ(0, celix_json_pointer_set(doc, &p, json_integer(99)));

    /* Verify array was extended with nulls */
    celix_json_pointer_t pa;
    memset(&pa, 0, sizeof(pa));
    ASSERT_EQ(0, celix_json_pointer_init(&pa, "/arr"));
    json_t* arr = celix_json_pointer_get(doc, &pa);
    ASSERT_NE(nullptr, arr);
    EXPECT_EQ(5u, json_array_size(arr));
    EXPECT_EQ(99, json_integer_value(json_array_get(arr, 4)));
    /* Positions 2, 3 should be null */
    EXPECT_TRUE(json_is_null(json_array_get(arr, 2)));
    EXPECT_TRUE(json_is_null(json_array_get(arr, 3)));

    celix_json_pointer_clear(&p);
    celix_json_pointer_clear(&pa);
    json_decref(doc);
}

/* ── Get on `/-` returns NULL ────────────────────────────────────────── */

TEST(PointerTest, DashIndexGetReturnsNull) {
    json_t* doc = json_loads("[1,2,3]", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);

    celix_json_pointer_t p;
    memset(&p, 0, sizeof(p));
    ASSERT_EQ(0, celix_json_pointer_init(&p, "/-"));
    EXPECT_EQ(nullptr, celix_json_pointer_get(doc, &p));

    celix_json_pointer_clear(&p);
    json_decref(doc);
}

/* ── Resolving into scalar returns NULL ──────────────────────────────── */

TEST(PointerTest, ResolveIntoScalar) {
    json_t* doc = json_loads(R"({"a":42,"b":{"c":1}})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);

    celix_json_pointer_t p;
    memset(&p, 0, sizeof(p));

    /* "/a" is 42 (scalar) — can't descend further */
    ASSERT_EQ(0, celix_json_pointer_init(&p, "/a/foo"));
    EXPECT_EQ(nullptr, celix_json_pointer_get(doc, &p));
    celix_json_pointer_clear(&p);

    /* "/a" is 42 (scalar) — get_or_create should also fail */
    ASSERT_EQ(0, celix_json_pointer_init(&p, "/a/foo"));
    EXPECT_EQ(nullptr, celix_json_pointer_get_or_create(doc, &p));
    celix_json_pointer_clear(&p);

    json_decref(doc);
}

/* ── Remove non-existent key ─────────────────────────────────────────── */

TEST(PointerTest, RemoveNonExistent) {
    json_t* doc = json_loads(R"({"a":1})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);

    celix_json_pointer_t p;
    memset(&p, 0, sizeof(p));
    ASSERT_EQ(0, celix_json_pointer_init(&p, "/x"));
    EXPECT_NE(0, celix_json_pointer_remove(doc, &p)); /* should fail */

    /* Document unchanged */
    json_t* v = json_object_get(doc, "a");
    ASSERT_NE(nullptr, v);
    EXPECT_EQ(1, json_integer_value(v));

    celix_json_pointer_clear(&p);
    json_decref(doc);
}

/* ── Remove root ─────────────────────────────────────────────────────── */

TEST(PointerTest, RemoveRootReturnsError) {
    json_t* doc = json_loads("[1,2]", JSON_DECODE_ANY, nullptr);
    celix_json_pointer_t p;
    memset(&p, 0, sizeof(p)); /* empty → root */
    EXPECT_NE(0, celix_json_pointer_remove(doc, &p));
    celix_json_pointer_clear(&p);
    json_decref(doc);
}

/* ── Token operations: front/back ────────────────────────────────────── */

TEST(PointerTest, TokenFrontBack) {
    celix_json_pointer_t p;
    memset(&p, 0, sizeof(p));
    ASSERT_EQ(0, celix_json_pointer_init(&p, "/a/b/c"));

    /* token(0) is front, token(depth-1) is back */
    EXPECT_STREQ("a", celix_json_pointer_token(&p, 0));
    EXPECT_STREQ("c", celix_json_pointer_token(&p, celix_json_pointer_depth(&p) - 1));

    celix_json_pointer_clear(&p);
}

/* ── Pointer on non-object/array root ────────────────────────────────── */

TEST(PointerTest, PointerOnScalarRoot) {
    json_t* doc = json_integer(42);

    /* Root pointer returns the scalar */
    celix_json_pointer_t p;
    memset(&p, 0, sizeof(p));
    json_t* v = celix_json_pointer_get(doc, &p);
    ASSERT_NE(nullptr, v);
    EXPECT_EQ(42, json_integer_value(v));
    celix_json_pointer_clear(&p);

    /* Non-root pointer on scalar returns NULL */
    memset(&p, 0, sizeof(p));
    ASSERT_EQ(0, celix_json_pointer_init(&p, "/foo"));
    EXPECT_EQ(nullptr, celix_json_pointer_get(doc, &p));
    celix_json_pointer_clear(&p);

    json_decref(doc);
}

/* ── get_or_create with array intermediates ──────────────────────────── */

TEST(PointerTest, SetCreatesArrayIntermediate) {
    /* set handles multi-level paths with array creation better */
    json_t* doc = json_object();
    celix_json_pointer_t p;
    memset(&p, 0, sizeof(p));
    ASSERT_EQ(0, celix_json_pointer_init(&p, "/list/0/item"));

    EXPECT_EQ(0, celix_json_pointer_set(doc, &p, json_string("hello")));

    char* s = json_dumps(doc, JSON_COMPACT);
    EXPECT_STREQ(R"({"list":[{"item":"hello"}]})", s);
    free(s);

    celix_json_pointer_clear(&p);
    json_decref(doc);
}

TEST(PointerTest, GetOrCreateSimpleArray) {
    json_t* doc = json_object();
    celix_json_pointer_t p;
    memset(&p, 0, sizeof(p));
    /* "/arr" creates an object, then push "2" — but get_or_create uses token heuristic:
     * "arr" → object (non-numeric), "2" → object key in that object.
     * For true array creation, use set which has the "look ahead" heuristic. */
    json_t* arr = json_array();
    json_array_append_new(arr, json_integer(10));
    json_array_append_new(arr, json_integer(20));
    json_object_set_new(doc, "arr", arr);

    /* Now "/arr/1" → get element at index 1 */
    ASSERT_EQ(0, celix_json_pointer_init(&p, "/arr/1"));
    json_t* node = celix_json_pointer_get_or_create(doc, &p);
    ASSERT_NE(nullptr, node);
    EXPECT_EQ(20, json_integer_value(node));
    json_decref(node);

    /* "/arr/2" → extend and return new null */
    celix_json_pointer_clear(&p);
    memset(&p, 0, sizeof(p));
    ASSERT_EQ(0, celix_json_pointer_init(&p, "/arr/3"));
    node = celix_json_pointer_get_or_create(doc, &p);
    ASSERT_NE(nullptr, node);
    EXPECT_TRUE(json_is_null(node));
    json_decref(node);

    EXPECT_EQ(4u, json_array_size(json_object_get(doc, "arr")));
    celix_json_pointer_clear(&p);
    json_decref(doc);
}

/* ── get_or_create on existing path ──────────────────────────────────── */

TEST(PointerTest, GetOrCreateExisting) {
    json_t* doc = json_loads(R"({"a":{"b":42}})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);

    celix_json_pointer_t p;
    memset(&p, 0, sizeof(p));
    ASSERT_EQ(0, celix_json_pointer_init(&p, "/a/b"));

    json_t* v = celix_json_pointer_get_or_create(doc, &p);
    ASSERT_NE(nullptr, v);
    EXPECT_EQ(42, json_integer_value(v));
    json_decref(v); /* get_or_create returns a new reference */

    /* Document unchanged */
    char* s = json_dumps(doc, JSON_COMPACT);
    EXPECT_STREQ(R"({"a":{"b":42}})", s);
    free(s);

    celix_json_pointer_clear(&p);
    json_decref(doc);
}

/* ── Escape edge: ~01 should be ~1 then 1, not ~0 + 1 ────────────────── */

TEST(PointerTest, EscapeOrderMatters) {
    /* "~01" in a pointer string means: unescape ~0→~, then the '1' is just '1'
     * So "/m~01n" should resolve to the key "m~1n", NOT "m~0" + "1n" or anything else. */
    celix_json_pointer_t p;
    memset(&p, 0, sizeof(p));
    ASSERT_EQ(0, celix_json_pointer_init(&p, "/m~01n"));
    EXPECT_EQ(1u, celix_json_pointer_depth(&p));
    /* ~0 → ~, then 1 remains as-is */
    EXPECT_STREQ("m~1n", celix_json_pointer_token(&p, 0));
    celix_json_pointer_clear(&p);
}

/* ── Escaped path resolution on a real document ──────────────────────── */

TEST(PointerTest, EscapedPathResolution) {
    json_t* doc = json_loads(R"({"a/b":42,"m~n":99})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);

    /* "/a~1b" should resolve to key "a/b" */
    celix_json_pointer_t p;
    memset(&p, 0, sizeof(p));
    ASSERT_EQ(0, celix_json_pointer_init(&p, "/a~1b"));
    json_t* v = celix_json_pointer_get(doc, &p);
    ASSERT_NE(nullptr, v);
    EXPECT_EQ(42, json_integer_value(v));
    celix_json_pointer_clear(&p);

    /* "/m~0n" should resolve to key "m~n" */
    memset(&p, 0, sizeof(p));
    ASSERT_EQ(0, celix_json_pointer_init(&p, "/m~0n"));
    v = celix_json_pointer_get(doc, &p);
    ASSERT_NE(nullptr, v);
    EXPECT_EQ(99, json_integer_value(v));
    celix_json_pointer_clear(&p);

    json_decref(doc);
}

/* ── Empty token (trailing slash or double slash) ────────────────────── */

TEST(PointerTest, EmptyToken) {
    /* "" is a valid JSON key — "/foo/" has two tokens: "foo" and "" */
    json_t* doc = json_loads(R"({"foo":{"":99}})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);

    celix_json_pointer_t p;
    memset(&p, 0, sizeof(p));
    ASSERT_EQ(0, celix_json_pointer_init(&p, "/foo/"));
    EXPECT_EQ(2u, celix_json_pointer_depth(&p));
    EXPECT_STREQ("foo", celix_json_pointer_token(&p, 0));
    EXPECT_STREQ("", celix_json_pointer_token(&p, 1));

    json_t* v = celix_json_pointer_get(doc, &p);
    ASSERT_NE(nullptr, v);
    EXPECT_EQ(99, json_integer_value(v));

    celix_json_pointer_clear(&p);
    json_decref(doc);
}

/* ── Percent-encoded characters in pointer ───────────────────────────── */

TEST(PointerTest, PercentEncodingIsLiteral) {
    /* RFC 6901: %XX is NOT decoded; it's just the literal characters */
    json_t* doc = json_loads(R"({"%25": "percent"})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);

    celix_json_pointer_t p;
    memset(&p, 0, sizeof(p));
    ASSERT_EQ(0, celix_json_pointer_init(&p, "/%25"));
    json_t* v = celix_json_pointer_get(doc, &p);
    ASSERT_NE(nullptr, v);
    EXPECT_STREQ("percent", json_string_value(v));

    celix_json_pointer_clear(&p);
    json_decref(doc);
}

/* ── Set with new nesting creates objects by default ─────────────────── */

TEST(PointerTest, SetCreatesIntermediateObjects) {
    json_t* doc = json_object();
    celix_json_pointer_t p;
    memset(&p, 0, sizeof(p));
    ASSERT_EQ(0, celix_json_pointer_init(&p, "/config/database/host"));

    EXPECT_EQ(0, celix_json_pointer_set(doc, &p, json_string("localhost")));

    char* s = json_dumps(doc, JSON_COMPACT);
    EXPECT_STREQ(R"({"config":{"database":{"host":"localhost"}}})", s);
    free(s);

    celix_json_pointer_clear(&p);
    json_decref(doc);
}

/* ── Comparison: same tokens, different order ────────────────────────── */

TEST(PointerTest, CompareDifferentOrder) {
    celix_json_pointer_t a, b;
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    celix_json_pointer_init(&a, "/a/b");
    celix_json_pointer_init(&b, "/b/a");
    EXPECT_FALSE(celix_json_pointer_equals(&a, &b));
    celix_json_pointer_clear(&a);
    celix_json_pointer_clear(&b);
}

TEST(PointerTest, CompareDifferentLength) {
    celix_json_pointer_t a, b;
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    celix_json_pointer_init(&a, "/a/b");
    celix_json_pointer_init(&b, "/a/b/c");
    EXPECT_FALSE(celix_json_pointer_equals(&a, &b));
    celix_json_pointer_clear(&a);
    celix_json_pointer_clear(&b);
}

/* ── Push with special characters gets escaped in to_string ──────────── */

TEST(PointerTest, PushWithSlashReEncoded) {
    celix_json_pointer_t p;
    memset(&p, 0, sizeof(p));
    celix_json_pointer_push(&p, "a/b");
    celix_json_pointer_push(&p, "c~d");

    char* s = celix_json_pointer_to_string(&p);
    EXPECT_STREQ("/a~1b/c~0d", s);
    free(s);

    celix_json_pointer_clear(&p);
}

/* ── Set new via new API ─────────────────────────────────────────────── */

TEST(PointerTest, SetNewIncrementsRef) {
    json_t* doc = json_object();
    celix_json_pointer_t p;
    memset(&p, 0, sizeof(p));
    ASSERT_EQ(0, celix_json_pointer_init(&p, "/x"));

    json_t* val = json_string("hello");
    EXPECT_EQ(0, celix_json_pointer_set_new(doc, &p, val));

    /* val should still be valid (set_new incref'd it) */
    EXPECT_STREQ("hello", json_string_value(val));

    json_t* v = celix_json_pointer_get(doc, &p);
    ASSERT_NE(nullptr, v);
    EXPECT_TRUE(json_equal(val, v));

    json_decref(val);
    celix_json_pointer_clear(&p);
    json_decref(doc);
}

/* ── Stack-allocated init/clear repeated ─────────────────────────────── */

TEST(PointerTest, StackReuse) {
    celix_json_pointer_t p;
    memset(&p, 0, sizeof(p));

    ASSERT_EQ(0, celix_json_pointer_init(&p, "/a/b"));
    EXPECT_EQ(2u, celix_json_pointer_depth(&p));
    celix_json_pointer_clear(&p);

    /* Re-init with different path */
    ASSERT_EQ(0, celix_json_pointer_init(&p, "/x/y/z"));
    EXPECT_EQ(3u, celix_json_pointer_depth(&p));
    EXPECT_STREQ("x", celix_json_pointer_token(&p, 0));
    EXPECT_STREQ("y", celix_json_pointer_token(&p, 1));
    EXPECT_STREQ("z", celix_json_pointer_token(&p, 2));
    celix_json_pointer_clear(&p);
}

/* ── celix_json_pointer_contains ───────────────────────────────────────── */

TEST(PointerTest, Contains) {
    json_t* doc = json_loads(R"({"foo":{"bar":42},"arr":[1,2,3]})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);

    celix_json_pointer_t p;
    memset(&p, 0, sizeof(p));

    /* Root pointer always exists */
    EXPECT_EQ(1, celix_json_pointer_contains(doc, &p));

    /* Existing paths */
    ASSERT_EQ(0, celix_json_pointer_init(&p, "/foo"));
    EXPECT_EQ(1, celix_json_pointer_contains(doc, &p));
    celix_json_pointer_clear(&p);

    ASSERT_EQ(0, celix_json_pointer_init(&p, "/foo/bar"));
    EXPECT_EQ(1, celix_json_pointer_contains(doc, &p));
    celix_json_pointer_clear(&p);

    ASSERT_EQ(0, celix_json_pointer_init(&p, "/arr"));
    EXPECT_EQ(1, celix_json_pointer_contains(doc, &p));
    celix_json_pointer_clear(&p);

    ASSERT_EQ(0, celix_json_pointer_init(&p, "/arr/1"));
    EXPECT_EQ(1, celix_json_pointer_contains(doc, &p));
    celix_json_pointer_clear(&p);

    /* Non-existing paths */
    ASSERT_EQ(0, celix_json_pointer_init(&p, "/arr/3"));
    EXPECT_EQ(0, celix_json_pointer_contains(doc, &p));
    celix_json_pointer_clear(&p);

    ASSERT_EQ(0, celix_json_pointer_init(&p, "/foo/baz"));
    EXPECT_EQ(0, celix_json_pointer_contains(doc, &p));
    celix_json_pointer_clear(&p);

    ASSERT_EQ(0, celix_json_pointer_init(&p, "/missing"));
    EXPECT_EQ(0, celix_json_pointer_contains(doc, &p));
    celix_json_pointer_clear(&p);

    /* "-" token returns 0 (no such array element) */
    ASSERT_EQ(0, celix_json_pointer_init(&p, "/arr/-"));
    EXPECT_EQ(0, celix_json_pointer_contains(doc, &p));
    celix_json_pointer_clear(&p);

    json_decref(doc);
}

TEST(PointerTest, ContainsNullDoc) {
    celix_json_pointer_t p;
    memset(&p, 0, sizeof(p));
    ASSERT_EQ(0, celix_json_pointer_init(&p, "/x"));

    /* NULL doc is safe — treated as not containing */
    EXPECT_EQ(0, celix_json_pointer_contains(nullptr, &p));

    celix_json_pointer_clear(&p);
}

TEST(PointerTest, ContainsScalarDoc) {
    json_t* doc = json_integer(42);

    celix_json_pointer_t p;
    memset(&p, 0, sizeof(p));

    /* Root pointer on scalar returns 1 */
    EXPECT_EQ(1, celix_json_pointer_contains(doc, &p));

    /* Non-root on scalar returns 0 */
    ASSERT_EQ(0, celix_json_pointer_init(&p, "/x"));
    EXPECT_EQ(0, celix_json_pointer_contains(doc, &p));
    celix_json_pointer_clear(&p);

    json_decref(doc);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Coverage edge cases — NULL guards, RFC 6901 validation error paths
 * ═══════════════════════════════════════════════════════════════════════════ */

/* ── Lifecycle and NULL guards ─────────────────────────────────────── */

TEST(PointerTest, InitNull) {
    EXPECT_EQ(-1, celix_json_pointer_init(nullptr, "/a"));
}

TEST(PointerTest, InitLeadingZeroNonNumeric) {
    /* Leading-zero token containing a non-digit is NOT an RFC 6901 array index —
     * it is a plain object key, so init must succeed. */
    celix_json_pointer_t p;
    memset(&p, 0, sizeof(p));
    ASSERT_EQ(0, celix_json_pointer_init(&p, "/0a"));
    EXPECT_EQ(1u, celix_json_pointer_depth(&p));
    EXPECT_STREQ("0a", celix_json_pointer_token(&p, 0));
    celix_json_pointer_clear(&p);

    memset(&p, 0, sizeof(p));
    ASSERT_EQ(0, celix_json_pointer_init(&p, "/01a"));
    EXPECT_EQ(1u, celix_json_pointer_depth(&p));
    EXPECT_STREQ("01a", celix_json_pointer_token(&p, 0));
    celix_json_pointer_clear(&p);
}

TEST(PointerTest, CopyNull) {
    EXPECT_EQ(nullptr, celix_json_pointer_copy(nullptr));
}

TEST(PointerTest, DestroyNull) {
    celix_json_pointer_destroy(nullptr); /* must be a safe no-op */
}

TEST(PointerTest, ClearNull) {
    celix_json_pointer_clear(nullptr); /* must be a safe no-op */
}

TEST(PointerTest, PushNull) {
    celix_json_pointer_t p;
    memset(&p, 0, sizeof(p));

    EXPECT_EQ(-1, celix_json_pointer_push(nullptr, "a"));
    EXPECT_EQ(-1, celix_json_pointer_push(&p, nullptr));
}

TEST(PointerTest, ToStringNull) {
    EXPECT_EQ(nullptr, celix_json_pointer_to_string(nullptr));
}

TEST(PointerTest, EscapeNull) {
    EXPECT_EQ(nullptr, celix_json_pointer_escape(nullptr));
}

TEST(PointerTest, UnescapeNull) {
    EXPECT_EQ(nullptr, celix_json_pointer_unescape(nullptr));
}

TEST(PointerTest, ParentNullAndRoot) {
    EXPECT_EQ(nullptr, celix_json_pointer_parent(nullptr, nullptr));

    /* Parent of the root (empty pointer) has no parent */
    celix_json_pointer_t* p = celix_json_pointer_create("");
    ASSERT_NE(nullptr, p);
    EXPECT_EQ(nullptr, celix_json_pointer_parent(p, nullptr));
    celix_json_pointer_destroy(p);
}

TEST(PointerTest, ConcatNull) {
    celix_json_pointer_t* p = celix_json_pointer_create("/a");
    celix_json_pointer_t* suffix = celix_json_pointer_create("/b");
    ASSERT_NE(nullptr, p);
    ASSERT_NE(nullptr, suffix);

    EXPECT_EQ(-1, celix_json_pointer_concat(nullptr, suffix));
    EXPECT_EQ(-1, celix_json_pointer_concat(p, nullptr));

    celix_json_pointer_destroy(p);
    celix_json_pointer_destroy(suffix);
}

TEST(PointerTest, EqualsSelf) {
    celix_json_pointer_t* p = celix_json_pointer_create("/a/b");
    ASSERT_NE(nullptr, p);
    EXPECT_TRUE(celix_json_pointer_equals(p, p)); /* same pointer short-circuit */
    celix_json_pointer_destroy(p);
}

/* ── get_or_create branches ────────────────────────────────────────── */

TEST(PointerTest, GetOrCreateNullDoc) {
    celix_json_pointer_t* p = celix_json_pointer_create("/a");
    ASSERT_NE(nullptr, p);

    EXPECT_EQ(nullptr, celix_json_pointer_get_or_create(nullptr, p));

    /* Scalar root is not a container */
    json_t* doc = json_integer(42);
    EXPECT_EQ(nullptr, celix_json_pointer_get_or_create(doc, p));

    json_decref(doc);
    celix_json_pointer_destroy(p);
}

TEST(PointerTest, GetOrCreateEmptyTokenObject) {
    json_t* doc = json_object();
    celix_json_pointer_t* p = celix_json_pointer_create("/a//b");
    ASSERT_NE(nullptr, p);

    json_t* node = celix_json_pointer_get_or_create(doc, p);
    ASSERT_NE(nullptr, node);
    EXPECT_TRUE(json_is_null(node));
    json_decref(node);

    char* s = json_dumps(doc, JSON_COMPACT);
    EXPECT_STREQ("{\"a\":{\"\":{\"b\":null}}}", s);
    free(s);

    celix_json_pointer_destroy(p);
    json_decref(doc);
}

TEST(PointerTest, GetOrCreateLeadingZeroObject) {
    json_t* doc = json_object();
    /* "01" cannot be parsed via init (RFC 6901 leading-zero rule), so build
     * the pointer with push to exercise the leading-zero heuristic. */
    celix_json_pointer_t* p = celix_json_pointer_create("");
    ASSERT_NE(nullptr, p);
    ASSERT_EQ(0, celix_json_pointer_push(p, "a"));
    ASSERT_EQ(0, celix_json_pointer_push(p, "01"));
    ASSERT_EQ(0, celix_json_pointer_push(p, "b"));

    json_t* node = celix_json_pointer_get_or_create(doc, p);
    ASSERT_NE(nullptr, node);
    json_decref(node);

    char* s = json_dumps(doc, JSON_COMPACT);
    EXPECT_STREQ("{\"a\":{\"01\":{\"b\":null}}}", s);
    free(s);

    celix_json_pointer_destroy(p);
    json_decref(doc);
}

TEST(PointerTest, GetOrCreateDashAppend) {
    json_t* doc = json_loads("[1,2,3]", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);

    celix_json_pointer_t* p = celix_json_pointer_create("/-");
    ASSERT_NE(nullptr, p);

    /* "-" as the last token appends a null element and returns the array (new reference) */
    json_t* node = celix_json_pointer_get_or_create(doc, p);
    ASSERT_NE(nullptr, node);
    EXPECT_TRUE(json_equal(node, doc));
    EXPECT_EQ(4u, json_array_size(node));
    EXPECT_TRUE(json_is_null(json_array_get(node, 3)));
    json_decref(node);

    /* Document has the appended null element */
    char* s = json_dumps(doc, JSON_COMPACT);
    EXPECT_STREQ("[1,2,3,null]", s);
    free(s);

    /* "-" can never resolve to an existing element: a second call appends again */
    json_t* node2 = celix_json_pointer_get_or_create(doc, p);
    ASSERT_NE(nullptr, node2);
    EXPECT_EQ(5u, json_array_size(node2));
    json_decref(node2);

    celix_json_pointer_destroy(p);
    json_decref(doc);
}

TEST(PointerTest, GetOrCreateDashIntermediate) {
    json_t* doc = json_loads("[[1,2]]", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);

    celix_json_pointer_t* p = celix_json_pointer_create("/-/0");
    ASSERT_NE(nullptr, p);

    /* "-" not in last position is invalid */
    EXPECT_EQ(nullptr, celix_json_pointer_get_or_create(doc, p));

    celix_json_pointer_destroy(p);
    json_decref(doc);
}

TEST(PointerTest, GetOrCreateNonNumericArrayIndex) {
    json_t* doc = json_loads("[1,2]", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);

    celix_json_pointer_t* p = celix_json_pointer_create("/abc");
    ASSERT_NE(nullptr, p);

    EXPECT_EQ(nullptr, celix_json_pointer_get_or_create(doc, p));

    celix_json_pointer_destroy(p);
    json_decref(doc);
}

/* ── set branches ──────────────────────────────────────────────────── */

TEST(PointerTest, SetNullArgs) {
    json_t* doc = json_object();
    celix_json_pointer_t* p = celix_json_pointer_create("/a");
    ASSERT_NE(nullptr, p);
    celix_json_pointer_t* empty = celix_json_pointer_create("");
    ASSERT_NE(nullptr, empty);

    /* NULL guards return before consuming value — caller keeps ownership */
    json_t* v1 = json_integer(1);
    EXPECT_EQ(-1, celix_json_pointer_set(nullptr, p, v1));
    json_decref(v1);

    json_t* v2 = json_integer(2);
    EXPECT_EQ(-1, celix_json_pointer_set(doc, nullptr, v2));
    json_decref(v2);

    json_t* v3 = json_integer(3);
    EXPECT_EQ(-1, celix_json_pointer_set(doc, empty, v3));
    json_decref(v3);

    celix_json_pointer_destroy(p);
    celix_json_pointer_destroy(empty);
    json_decref(doc);
}

TEST(PointerTest, SetNextTokenEmptyObject) {
    json_t* doc = json_object();
    celix_json_pointer_t* p = celix_json_pointer_create("/a/");
    ASSERT_NE(nullptr, p);

    EXPECT_EQ(0, celix_json_pointer_set(doc, p, json_integer(7)));

    char* s = json_dumps(doc, JSON_COMPACT);
    EXPECT_STREQ("{\"a\":{\"\":7}}", s);
    free(s);

    celix_json_pointer_destroy(p);
    json_decref(doc);
}

TEST(PointerTest, SetNextTokenLeadingZeroObject) {
    json_t* doc = json_object();
    /* "01" cannot be parsed via init (RFC 6901 leading-zero rule), so build
     * the pointer with push to exercise the leading-zero heuristic. */
    celix_json_pointer_t* p = celix_json_pointer_create("");
    ASSERT_NE(nullptr, p);
    ASSERT_EQ(0, celix_json_pointer_push(p, "a"));
    ASSERT_EQ(0, celix_json_pointer_push(p, "01"));
    ASSERT_EQ(0, celix_json_pointer_push(p, "b"));

    EXPECT_EQ(0, celix_json_pointer_set(doc, p, json_integer(7)));

    char* s = json_dumps(doc, JSON_COMPACT);
    EXPECT_STREQ("{\"a\":{\"01\":{\"b\":7}}}", s);
    free(s);

    celix_json_pointer_destroy(p);
    json_decref(doc);
}

TEST(PointerTest, SetNonNumericArrayIntermediate) {
    json_t* doc = json_loads("[1,2]", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);

    celix_json_pointer_t* p = celix_json_pointer_create("/abc/0");
    ASSERT_NE(nullptr, p);

    /* Fails and consumes (decrefs) the value */
    EXPECT_EQ(-1, celix_json_pointer_set(doc, p, json_integer(9)));

    char* s = json_dumps(doc, JSON_COMPACT);
    EXPECT_STREQ("[1,2]", s);
    free(s);

    celix_json_pointer_destroy(p);
    json_decref(doc);
}

TEST(PointerTest, SetReplacePrimitiveWithObjectEmpty) {
    json_t* doc = json_loads("[1]", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);

    celix_json_pointer_t* p = celix_json_pointer_create("/0/");
    ASSERT_NE(nullptr, p);

    EXPECT_EQ(0, celix_json_pointer_set(doc, p, json_integer(9)));

    char* s = json_dumps(doc, JSON_COMPACT);
    EXPECT_STREQ("[{\"\":9}]", s);
    free(s);

    celix_json_pointer_destroy(p);
    json_decref(doc);
}

TEST(PointerTest, SetReplacePrimitiveWithObjectLeadingZero) {
    json_t* doc = json_loads("[1]", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);

    /* "01" cannot be parsed via init (RFC 6901 leading-zero rule), so build
     * the pointer with push to exercise the leading-zero heuristic. */
    celix_json_pointer_t* p = celix_json_pointer_create("");
    ASSERT_NE(nullptr, p);
    ASSERT_EQ(0, celix_json_pointer_push(p, "0"));
    ASSERT_EQ(0, celix_json_pointer_push(p, "01"));
    ASSERT_EQ(0, celix_json_pointer_push(p, "a"));

    EXPECT_EQ(0, celix_json_pointer_set(doc, p, json_integer(9)));

    char* s = json_dumps(doc, JSON_COMPACT);
    EXPECT_STREQ("[{\"01\":{\"a\":9}}]", s);
    free(s);

    celix_json_pointer_destroy(p);
    json_decref(doc);
}

TEST(PointerTest, SetScalarRootFails) {
    json_t* doc = json_integer(1);

    celix_json_pointer_t* p = celix_json_pointer_create("/a/b");
    ASSERT_NE(nullptr, p);

    /* Intermediate is a scalar — fails and consumes the value */
    EXPECT_EQ(-1, celix_json_pointer_set(doc, p, json_integer(9)));

    celix_json_pointer_destroy(p);
    json_decref(doc);
}

TEST(PointerTest, SetNonNumericArrayLast) {
    json_t* doc = json_loads("[1,2]", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);

    celix_json_pointer_t* p = celix_json_pointer_create("/abc");
    ASSERT_NE(nullptr, p);

    EXPECT_EQ(-1, celix_json_pointer_set(doc, p, json_integer(9)));

    char* s = json_dumps(doc, JSON_COMPACT);
    EXPECT_STREQ("[1,2]", s);
    free(s);

    celix_json_pointer_destroy(p);
    json_decref(doc);
}

TEST(PointerTest, SetScalarLastFails) {
    json_t* doc = json_integer(1);

    celix_json_pointer_t* p = celix_json_pointer_create("/a");
    ASSERT_NE(nullptr, p);

    EXPECT_EQ(-1, celix_json_pointer_set(doc, p, json_integer(9)));

    celix_json_pointer_destroy(p);
    json_decref(doc);
}

/* ── remove branches ───────────────────────────────────────────────── */

TEST(PointerTest, RemoveMissingParent) {
    json_t* doc = json_object();

    celix_json_pointer_t* p = celix_json_pointer_create("/a/b");
    ASSERT_NE(nullptr, p);

    EXPECT_EQ(-1, celix_json_pointer_remove(doc, p));

    char* s = json_dumps(doc, JSON_COMPACT);
    EXPECT_STREQ("{}", s);
    free(s);

    celix_json_pointer_destroy(p);
    json_decref(doc);
}

TEST(PointerTest, RemoveNonNumericArrayIndex) {
    json_t* doc = json_loads("[[1]]", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);

    celix_json_pointer_t* p = celix_json_pointer_create("/0/abc");
    ASSERT_NE(nullptr, p);

    EXPECT_EQ(-1, celix_json_pointer_remove(doc, p));

    celix_json_pointer_destroy(p);
    json_decref(doc);
}

TEST(PointerTest, RemoveArrayIndexOutOfBounds) {
    json_t* doc = json_loads("[1,2]", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, doc);

    celix_json_pointer_t* p = celix_json_pointer_create("/5");
    ASSERT_NE(nullptr, p);

    EXPECT_EQ(-1, celix_json_pointer_remove(doc, p));

    char* s = json_dumps(doc, JSON_COMPACT);
    EXPECT_STREQ("[1,2]", s);
    free(s);

    celix_json_pointer_destroy(p);
    json_decref(doc);
}

TEST(PointerTest, RemoveScalarParent) {
    json_t* doc = json_integer(1);

    celix_json_pointer_t* p = celix_json_pointer_create("/a");
    ASSERT_NE(nullptr, p);

    EXPECT_EQ(-1, celix_json_pointer_remove(doc, p));

    celix_json_pointer_destroy(p);
    json_decref(doc);
}

/* ── Escape / navigation ───────────────────────────────────────────── */

TEST(PointerTest, UnescapeInvalidEscape) {
    /* Invalid escapes are kept as-is: '~' is output and the next
     * character is processed as a plain character. */
    char* out = celix_json_pointer_unescape("a~2b");
    ASSERT_NE(nullptr, out);
    EXPECT_STREQ("a~2b", out);
    free(out);

    out = celix_json_pointer_unescape("~");
    ASSERT_NE(nullptr, out);
    EXPECT_STREQ("~", out);
    free(out);
}

TEST(PointerTest, ParentAllocated) {
    celix_json_pointer_t* p = celix_json_pointer_create("/a/b");
    ASSERT_NE(nullptr, p);

    /* out == NULL → heap-allocated result (must be destroyed) */
    celix_json_pointer_t* pp = celix_json_pointer_parent(p, nullptr);
    ASSERT_NE(nullptr, pp);
    EXPECT_EQ(1u, celix_json_pointer_depth(pp));
    EXPECT_STREQ("a", celix_json_pointer_token(pp, 0));
    celix_json_pointer_destroy(pp);

    celix_json_pointer_destroy(p);
}

/* ── Capacity growth ───────────────────────────────────────────────── */

TEST(PointerTest, CopyLongPointer) {
    /* 9 tokens exceed the initial capacity of 8 → triggers the
     * doubling growth loop in ensure_cap. */
    celix_json_pointer_t* p = celix_json_pointer_create("/a/b/c/d/e/f/g/h/i");
    ASSERT_NE(nullptr, p);
    EXPECT_EQ(9u, celix_json_pointer_depth(p));

    celix_json_pointer_t* copy = celix_json_pointer_copy(p);
    ASSERT_NE(nullptr, copy);
    EXPECT_EQ(9u, celix_json_pointer_depth(copy));
    EXPECT_TRUE(celix_json_pointer_equals(p, copy));

    celix_json_pointer_destroy(p);
    celix_json_pointer_destroy(copy);
}
