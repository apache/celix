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
#include "test_common.h"
#include "celix_jansson_uri.h"

#include <cstring>
#include <string>

/* ── celix_jansson_uri_update ────────────────────────────────────────────── */

TEST(UriExtendedTest, UpdateFragmentOnly) {
    celix_jansson_uri_t u;
    memset(&u, 0, sizeof(u));
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_init(&u, "http://example.com/root#"));

    /* Update with fragment-only reference */
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_update(&u, "#/definitions/foo"));

    char* loc = celix_jansson_uri_location(&u);
    EXPECT_STREQ("http://example.com/root", loc);
    free(loc);

    char* frag = celix_jansson_uri_fragment(&u);
    EXPECT_STREQ("/definitions/foo", frag);
    free(frag);

    /* Update to identifier fragment */
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_update(&u, "#name"));
    frag = celix_jansson_uri_fragment(&u);
    EXPECT_STREQ("name", frag);
    free(frag);

    celix_jansson_uri_clear(&u);
}

TEST(UriExtendedTest, UpdateRelativePath) {
    celix_jansson_uri_t u;
    memset(&u, 0, sizeof(u));
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_init(&u, "http://json-schema.org/draft-07/schema#"));

    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_update(&u, "other.json"));

    char* loc = celix_jansson_uri_location(&u);
    EXPECT_STREQ("http://json-schema.org/draft-07/other.json", loc);
    free(loc);

    /* Fragment cleared by path update */
    char* frag = celix_jansson_uri_fragment(&u);
    EXPECT_STREQ("", frag);
    free(frag);

    celix_jansson_uri_clear(&u);
}

TEST(UriExtendedTest, UpdateAbsolutePath) {
    celix_jansson_uri_t u;
    memset(&u, 0, sizeof(u));
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_init(&u, "http://a.com/x/y#"));

    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_update(&u, "/new/path"));

    char* loc = celix_jansson_uri_location(&u);
    EXPECT_STREQ("http://a.com/new/path", loc);
    free(loc);

    celix_jansson_uri_clear(&u);
}

TEST(UriExtendedTest, UpdateRelativeToPathWithoutDirectory) {
    celix_jansson_uri_t u;
    memset(&u, 0, sizeof(u));
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_init(&u, "schema.json"));

    /* Old path has no directory, so the relative path replaces it entirely */
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_update(&u, "other.json"));

    char* loc = celix_jansson_uri_location(&u);
    EXPECT_STREQ("other.json", loc);
    free(loc);

    celix_jansson_uri_clear(&u);
}

TEST(UriExtendedTest, UpdateRelativeWithoutBase) {
    celix_jansson_uri_t u;
    memset(&u, 0, sizeof(u));
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_init(&u, ""));

    /* No previous location to resolve against */
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_update(&u, "relative/path"));

    char* loc = celix_jansson_uri_location(&u);
    EXPECT_STREQ("relative/path", loc);
    free(loc);

    celix_jansson_uri_clear(&u);
}

TEST(UriExtendedTest, UpdateFullReplacement) {
    celix_jansson_uri_t u;
    memset(&u, 0, sizeof(u));
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_init(&u, "http://a.com/p"));

    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_update(&u, "https://b.org/q"));

    char* loc = celix_jansson_uri_location(&u);
    EXPECT_STREQ("https://b.org/q", loc);
    free(loc);

    celix_jansson_uri_clear(&u);
}

TEST(UriExtendedTest, UpdateNull) {
    celix_jansson_uri_t u;
    memset(&u, 0, sizeof(u));
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_init(&u, "http://example.com/schema"));

    /* update(NULL) is a no-op */
    int rc = celix_jansson_uri_update(&u, nullptr);
    EXPECT_EQ(0, rc);

    char* loc = celix_jansson_uri_location(&u);
    EXPECT_STREQ("http://example.com/schema", loc);
    free(loc);

    celix_jansson_uri_clear(&u);
}

/* ── celix_jansson_uri_equals ────────────────────────────────────────────── */

TEST(UriExtendedTest, Equals) {
    celix_jansson_uri_t a, b;
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));

    /* Identical */
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_init(&a, "http://example.com/schema#/def/foo"));
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_init(&b, "http://example.com/schema#/def/foo"));
    EXPECT_TRUE(celix_jansson_uri_equals(&a, &b));
    celix_jansson_uri_clear(&a);
    celix_jansson_uri_clear(&b);

    /* Different locations */
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_init(&a, "http://a.com/s"));
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_init(&b, "http://b.com/s"));
    EXPECT_FALSE(celix_jansson_uri_equals(&a, &b));
    celix_jansson_uri_clear(&a);
    celix_jansson_uri_clear(&b);

    /* Same location, different fragment */
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_init(&a, "http://e.com/s#/a"));
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_init(&b, "http://e.com/s#/b"));
    EXPECT_FALSE(celix_jansson_uri_equals(&a, &b));
    celix_jansson_uri_clear(&a);
    celix_jansson_uri_clear(&b);

    /* Both empty fragments (with and without '#') are equal */
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_init(&a, "http://e.com/s"));
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_init(&b, "http://e.com/s#"));
    EXPECT_TRUE(celix_jansson_uri_equals(&a, &b));
    celix_jansson_uri_clear(&a);
    celix_jansson_uri_clear(&b);

    /* URN vs HTTP */
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_init(&a, "urn:uuid:123"));
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_init(&b, "http://e.com/s"));
    EXPECT_FALSE(celix_jansson_uri_equals(&a, &b));
    celix_jansson_uri_clear(&a);
    celix_jansson_uri_clear(&b);
}

/* ── celix_jansson_uri_to_string ─────────────────────────────────────────── */

TEST(UriExtendedTest, ToString) {
    celix_jansson_uri_t u;
    memset(&u, 0, sizeof(u));

    /* Plain HTTP URI */
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_init(&u, "http://example.com/schema"));
    char* s = celix_jansson_uri_to_string(&u);
    EXPECT_STREQ("http://example.com/schema", s);
    free(s);
    celix_jansson_uri_clear(&u);

    /* With pointer fragment */
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_init(&u, "http://example.com/schema#/definitions/foo"));
    s = celix_jansson_uri_to_string(&u);
    EXPECT_STREQ("http://example.com/schema#/definitions/foo", s);
    free(s);
    celix_jansson_uri_clear(&u);

    /* With identifier fragment */
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_init(&u, "http://example.com/schema#foo"));
    s = celix_jansson_uri_to_string(&u);
    EXPECT_STREQ("http://example.com/schema#foo", s);
    free(s);
    celix_jansson_uri_clear(&u);

    /* URN */
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_init(&u, "urn:uuid:12345678-1234-1234-1234-123456789abc"));
    s = celix_jansson_uri_to_string(&u);
    EXPECT_STREQ("urn:uuid:12345678-1234-1234-1234-123456789abc", s);
    free(s);
    celix_jansson_uri_clear(&u);

    /* Empty */
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_init(&u, ""));
    s = celix_jansson_uri_to_string(&u);
    EXPECT_STREQ("", s);
    free(s);
    celix_jansson_uri_clear(&u);
}

/* ── Percent-decoded fragment ───────────────────────────────────────────── */

TEST(UriExtendedTest, PercentDecodedFragment) {
    celix_jansson_uri_t u;
    memset(&u, 0, sizeof(u));

    /* Fragment with percent-encoded slash → becomes a JSON Pointer */
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_init(&u, "http://e.com/s#%2Fdefinitions%2Ffoo"));
    char* frag = celix_jansson_uri_fragment(&u);
    EXPECT_STREQ("/definitions/foo", frag);
    free(frag);
    celix_jansson_uri_clear(&u);

    /* Fragment with percent-encoded space → becomes an identifier */
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_init(&u, "http://e.com/s#a%20b"));
    frag = celix_jansson_uri_fragment(&u);
    EXPECT_STREQ("a b", frag);
    free(frag);
    celix_jansson_uri_clear(&u);
}

/* ── celix_jansson_uri_derive with fragments ─────────────────────────────── */

TEST(UriExtendedTest, DeriveFromIdentifierUri) {
    celix_jansson_uri_t base, derived;
    memset(&base, 0, sizeof(base));
    memset(&derived, 0, sizeof(derived));

    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_init(&base, "http://example.com/schema#foo"));

    /* update(NULL) keeps the copied identifier */
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_derive(&base, nullptr, &derived));

    char* frag = celix_jansson_uri_fragment(&derived);
    EXPECT_STREQ("foo", frag);
    free(frag);

    celix_jansson_uri_clear(&base);
    celix_jansson_uri_clear(&derived);
}

TEST(UriExtendedTest, DeriveFromPointerUri) {
    celix_jansson_uri_t base, derived;
    memset(&base, 0, sizeof(base));
    memset(&derived, 0, sizeof(derived));

    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_init(&base, "http://example.com/schema#/definitions/foo"));

    /* update(NULL) keeps the copied pointer tokens */
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_derive(&base, nullptr, &derived));

    char* frag = celix_jansson_uri_fragment(&derived);
    EXPECT_STREQ("/definitions/foo", frag);
    free(frag);

    celix_jansson_uri_clear(&base);
    celix_jansson_uri_clear(&derived);
}

/* ── celix_jansson_uri_append with identifier URI ────────────────────────── */

TEST(UriExtendedTest, AppendToIdentifierUriIsNoOp) {
    celix_jansson_uri_t base, result;
    memset(&base, 0, sizeof(base));
    memset(&result, 0, sizeof(result));

    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_init(&base, "http://example.com/schema#foo"));

    /* Appending to an identifier URI is a no-op */
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_append(&base, "bar", &result));

    char* frag = celix_jansson_uri_fragment(&result);
    EXPECT_STREQ("foo", frag);
    free(frag);

    celix_jansson_uri_clear(&base);
    celix_jansson_uri_clear(&result);
}

/* ── reusing the out buffer (documented: out is cleared first) ───────────── */

TEST(UriExtendedTest, DeriveReuseOutputBuffer) {
    celix_jansson_uri_t base, out;
    memset(&base, 0, sizeof(base));
    memset(&out, 0, sizeof(out));

    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_init(&base, "http://example.com/schema"));
    /* Pre-fill out with an unrelated URI — derive must not leak or keep
     * these stale components (out is cleared first) */
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_init(&out, "http://stale.example/old#/definitions/old"));

    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_derive(&base, "child.json", &out));
    char* loc = celix_jansson_uri_location(&out);
    EXPECT_STREQ("http://example.com/child.json", loc);
    free(loc);

    /* Second derive into the same out without clearing in between */
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_derive(&base, "other.json", &out));
    loc = celix_jansson_uri_location(&out);
    EXPECT_STREQ("http://example.com/other.json", loc);
    free(loc);

    celix_jansson_uri_clear(&base);
    celix_jansson_uri_clear(&out);
}

TEST(UriExtendedTest, AppendReuseOutputBuffer) {
    celix_jansson_uri_t base, out;
    memset(&base, 0, sizeof(base));
    memset(&out, 0, sizeof(out));

    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_init(&base, "http://example.com/schema#/definitions"));
    /* Pre-fill out with an unrelated URI */
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_init(&out, "http://stale.example/old#/definitions/old"));

    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_append(&base, "Foo", &out));
    char* frag = celix_jansson_uri_fragment(&out);
    EXPECT_STREQ("/definitions/Foo", frag);
    free(frag);

    /* Second append into the same out — stale tokens must not accumulate */
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_append(&base, "Bar", &out));
    frag = celix_jansson_uri_fragment(&out);
    EXPECT_STREQ("/definitions/Bar", frag);
    free(frag);

    celix_jansson_uri_clear(&base);
    celix_jansson_uri_clear(&out);
}

/* ── celix_auto automatic cleanup ────────────────────────────────────────── */

TEST(UriExtendedTest, AutoInit) {
    /* celix_auto → celix_jansson_uri_clear() runs at scope exit. */
    celix_auto(celix_jansson_uri_t) u;
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_init(&u, "http://example.com/schema#/definitions/foo"));

    char* loc = celix_jansson_uri_location(&u);
    EXPECT_STREQ("http://example.com/schema", loc);
    free(loc);
    char* frag = celix_jansson_uri_fragment(&u);
    EXPECT_STREQ("/definitions/foo", frag);
    free(frag);
}

TEST(UriExtendedTest, AutoZeroedStructSafe) {
    /* Zeroed struct is safe to auto-clean without any init. */
    celix_auto(celix_jansson_uri_t) u{};
}

TEST(UriExtendedTest, AutoInitFailureIsSafe) {
    /* init() zeroes first and clears on failure, so scope-exit cleanup is
     * safe even when init fails (here: '~' not followed by 0/1). */
    celix_auto(celix_jansson_uri_t) u;
    ASSERT_NE(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_init(&u, "http://example.com/schema#/bad~"));
}

TEST(UriExtendedTest, AutoDeriveInto) {
    celix_auto(celix_jansson_uri_t) base;
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_init(&base, "http://example.com/root"));

    /* Both URIs are cleared automatically at scope exit. */
    celix_auto(celix_jansson_uri_t) out{};
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_derive(&base, "child.json", &out));

    char* loc = celix_jansson_uri_location(&out);
    EXPECT_STREQ("http://example.com/child.json", loc);
    free(loc);
}
