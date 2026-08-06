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

extern "C" {
#include "celix_schema.h"
}

#include <gtest/gtest.h>

/* ── init ────────────────────────────────────────────────────────────────── */

TEST(PathTest, Init) {
    celix_jansson_path_t p;
    celix_jansson_path_init(&p);

    EXPECT_EQ(0, p.len);
    EXPECT_EQ(0, p.cap);
    EXPECT_STREQ("", celix_jansson_path_str(&p));

    celix_jansson_path_free(&p);
}

/* ── push ────────────────────────────────────────────────────────────────── */

TEST(PathTest, PushSingle) {
    celix_jansson_path_t p;
    celix_jansson_path_init(&p);

    int rc = celix_jansson_path_push(&p, "token");
    EXPECT_EQ(0, rc);
    EXPECT_EQ(1, p.len);
    EXPECT_STREQ("/token", celix_jansson_path_str(&p));

    celix_jansson_path_free(&p);
}

TEST(PathTest, PushMultiple) {
    celix_jansson_path_t p;
    celix_jansson_path_init(&p);

    EXPECT_EQ(0, celix_jansson_path_push(&p, "first"));
    EXPECT_EQ(0, celix_jansson_path_push(&p, "second"));
    EXPECT_EQ(0, celix_jansson_path_push(&p, "third"));
    EXPECT_EQ(3, p.len);
    EXPECT_STREQ("/first/second/third", celix_jansson_path_str(&p));

    celix_jansson_path_free(&p);
}

TEST(PathTest, PushTokenWithSpecialChars) {
    celix_jansson_path_t p;
    celix_jansson_path_init(&p);

    /* raw tokens are stored as-is; escaping is tested under str */
    EXPECT_EQ(0, celix_jansson_path_push(&p, "a/b"));
    EXPECT_EQ(0, celix_jansson_path_push(&p, "c~d"));
    EXPECT_EQ(2, p.len);
    EXPECT_STREQ("/a~1b/c~0d", celix_jansson_path_str(&p));

    celix_jansson_path_free(&p);
}

/* ── pop ─────────────────────────────────────────────────────────────────── */

TEST(PathTest, PopFromEmpty) {
    celix_jansson_path_t p;
    celix_jansson_path_init(&p);

    /* should be a safe no-op */
    celix_jansson_path_pop(&p);
    EXPECT_EQ(0, p.len);
    EXPECT_STREQ("", celix_jansson_path_str(&p));

    celix_jansson_path_free(&p);
}

TEST(PathTest, PushThenPop) {
    celix_jansson_path_t p;
    celix_jansson_path_init(&p);

    celix_jansson_path_push(&p, "token");
    celix_jansson_path_pop(&p);
    EXPECT_EQ(0, p.len);
    EXPECT_STREQ("", celix_jansson_path_str(&p));

    celix_jansson_path_free(&p);
}

TEST(PathTest, PushTwoPopOne) {
    celix_jansson_path_t p;
    celix_jansson_path_init(&p);

    celix_jansson_path_push(&p, "first");
    celix_jansson_path_push(&p, "second");
    celix_jansson_path_pop(&p);
    EXPECT_EQ(1, p.len);
    EXPECT_STREQ("/first", celix_jansson_path_str(&p));

    celix_jansson_path_free(&p);
}

/* ── str (JSON Pointer serialization) ────────────────────────────────────── */

TEST(PathTest, StrEmpty) {
    celix_jansson_path_t p;
    celix_jansson_path_init(&p);

    EXPECT_STREQ("", celix_jansson_path_str(&p));

    celix_jansson_path_free(&p);
}

TEST(PathTest, StrSingle) {
    celix_jansson_path_t p;
    celix_jansson_path_init(&p);

    celix_jansson_path_push(&p, "hello");
    EXPECT_STREQ("/hello", celix_jansson_path_str(&p));

    celix_jansson_path_free(&p);
}

TEST(PathTest, StrEscapingTilde) {
    celix_jansson_path_t p;
    celix_jansson_path_init(&p);

    celix_jansson_path_push(&p, "~");
    EXPECT_STREQ("/~0", celix_jansson_path_str(&p));

    celix_jansson_path_free(&p);
}

TEST(PathTest, StrEscapingSlash) {
    celix_jansson_path_t p;
    celix_jansson_path_init(&p);

    celix_jansson_path_push(&p, "/");
    EXPECT_STREQ("/~1", celix_jansson_path_str(&p));

    celix_jansson_path_free(&p);
}

TEST(PathTest, StrEscapingCombined) {
    celix_jansson_path_t p;
    celix_jansson_path_init(&p);

    celix_jansson_path_push(&p, "a/b");
    celix_jansson_path_push(&p, "c~d");
    EXPECT_STREQ("/a~1b/c~0d", celix_jansson_path_str(&p));

    celix_jansson_path_free(&p);
}

TEST(PathTest, StrCacheStable) {
    celix_jansson_path_t p;
    celix_jansson_path_init(&p);

    celix_jansson_path_push(&p, "x");
    const char* s1 = celix_jansson_path_str(&p);
    const char* s2 = celix_jansson_path_str(&p);
    EXPECT_EQ(s1, s2); /* same pointer — cache is valid */

    celix_jansson_path_free(&p);
}

TEST(PathTest, StrCacheInvalidatedOnPush) {
    celix_jansson_path_t p;
    celix_jansson_path_init(&p);

    celix_jansson_path_push(&p, "x");
    const char* s1 = celix_jansson_path_str(&p);
    celix_jansson_path_push(&p, "y");
    const char* s2 = celix_jansson_path_str(&p);
    EXPECT_NE(s1, s2); /* cache was invalidated, new string allocated */

    celix_jansson_path_free(&p);
}

TEST(PathTest, StrCacheInvalidatedOnPop) {
    celix_jansson_path_t p;
    celix_jansson_path_init(&p);

    celix_jansson_path_push(&p, "x");
    celix_jansson_path_push(&p, "y");
    const char* s1 = celix_jansson_path_str(&p);
    celix_jansson_path_pop(&p);
    const char* s2 = celix_jansson_path_str(&p);
    EXPECT_NE(s1, s2); /* cache was invalidated, new string allocated */

    celix_jansson_path_free(&p);
}

/* ── free ────────────────────────────────────────────────────────────────── */

TEST(PathTest, FreeEmpty) {
    celix_jansson_path_t p;
    celix_jansson_path_init(&p);

    celix_jansson_path_free(&p);
    /* no crash = pass */
    EXPECT_EQ(0, p.len);
    EXPECT_EQ(0, p.cap);
}

TEST(PathTest, FreeWithTokens) {
    celix_jansson_path_t p;
    celix_jansson_path_init(&p);

    celix_jansson_path_push(&p, "a");
    celix_jansson_path_push(&p, "b");
    celix_jansson_path_free(&p);
    /* no crash = pass */
    EXPECT_EQ(0, p.len);
    EXPECT_EQ(0, p.cap);
}

TEST(PathTest, FreeThenReinit) {
    celix_jansson_path_t p;
    celix_jansson_path_init(&p);

    celix_jansson_path_push(&p, "a");
    celix_jansson_path_free(&p);

    /* struct is zeroed — re-init and use again */
    celix_jansson_path_init(&p);
    EXPECT_EQ(0, p.len);
    EXPECT_STREQ("", celix_jansson_path_str(&p));
    celix_jansson_path_push(&p, "b");
    EXPECT_STREQ("/b", celix_jansson_path_str(&p));

    celix_jansson_path_free(&p);
}
