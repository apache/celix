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

#include <gtest/gtest.h>

#include "celix_cleanup.h"
#include "celix_jansson_pointer.h"
#include "celix_jansson_uri.h"
#include "celix_json_patch.h"
#include "celix_util.h"
#include "jansson_ei.h"
#include "malloc_ei.h"
#include "string_ei.h"

CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(celix_json_pointer_t, celix_json_pointer_destroy)

/**
 * Error-injection tests for the OOM (out-of-memory) handling paths of
 * celix_util, celix_jansson_pointer, celix_jansson_uri and celix_json_patch.
 *
 * Every allocation-failure branch (if (!ptr) return ...) is exercised by
 * injecting a NULL return for the matching allocator with an exact caller
 * match. The injected caller address is the function that (directly or, via
 * `level`, indirectly) calls the wrapped allocator.
 */
class JanssonExtErrorInjectionTestSuite : public ::testing::Test {
public:
    ~JanssonExtErrorInjectionTestSuite() noexcept override {
        celix_ei_expect_malloc(nullptr, 0, nullptr);
        celix_ei_expect_realloc(nullptr, 0, nullptr);
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_ei_expect_strdup(nullptr, 0, nullptr);
        celix_ei_expect_json_object(nullptr, 0, nullptr);
        celix_ei_expect_json_deep_copy(nullptr, 0, nullptr);
        celix_ei_expect_json_object_set_new(nullptr, 0, 0);
        celix_ei_expect_json_array_append_new(nullptr, 0, 0);
        celix_ei_expect_json_null(nullptr, 0, nullptr);
    }
};

/* ── celix_util.c ─────────────────────────────────────────────────────── */

TEST_F(JanssonExtErrorInjectionTestSuite, UtilStrbufAppendReallocFail) {
    //Given a fresh string buffer and realloc is injected to fail in strbuf_append
    celix_jansson_strbuf_t sb;
    celix_jansson_strbuf_init(&sb);
    celix_ei_expect_realloc((void*)celix_jansson_strbuf_append, 0, nullptr);
    //Then appending should fail
    EXPECT_EQ(-1, celix_jansson_strbuf_append(&sb, "hello", 5));
    celix_jansson_strbuf_free(&sb);
}

TEST_F(JanssonExtErrorInjectionTestSuite, UtilStrbufVappendfReallocFail) {
    //Given a fresh string buffer and realloc is injected to fail in vappendf
    celix_jansson_strbuf_t sb;
    celix_jansson_strbuf_init(&sb);
    celix_ei_expect_realloc((void*)celix_jansson_strbuf_vappendf, 0, nullptr);
    //Then printf-appending should fail
    EXPECT_EQ(-1, celix_jansson_strbuf_appendf(&sb, "%s", "hello"));
    celix_jansson_strbuf_free(&sb);
}

TEST_F(JanssonExtErrorInjectionTestSuite, UtilVecPushReallocFail) {
    //Given a fresh vec and realloc is injected to fail in vec_push
    celix_jansson_vec_t v;
    celix_jansson_vec_init(&v);
    celix_ei_expect_realloc((void*)celix_jansson_vec_push, 0, nullptr);
    //Then pushing an item should fail
    EXPECT_EQ(-1, celix_jansson_vec_push(&v, (void*)0x1));
    celix_jansson_vec_free(&v);
}

/* ── celix_jansson_pointer.c ──────────────────────────────────────────── */

TEST_F(JanssonExtErrorInjectionTestSuite, PointerEnsureCapReallocFail) {
    //Given realloc is injected to fail in ensure_cap (static, called from push)
    celix_ei_expect_realloc((void*)celix_json_pointer_push, 1, nullptr);
    celix_json_pointer_t p{};
    //Then pushing a token should fail
    EXPECT_EQ(-1, celix_json_pointer_push(&p, "a"));
    celix_json_pointer_clear(&p);
}

TEST_F(JanssonExtErrorInjectionTestSuite, PointerCreateCallocFail) {
    //Given calloc is injected to fail in create
    celix_ei_expect_calloc((void*)celix_json_pointer_create, 0, nullptr);
    //Then creating a pointer should fail
    EXPECT_EQ(nullptr, celix_json_pointer_create("/a"));
}

TEST_F(JanssonExtErrorInjectionTestSuite, PointerInitDecodeMallocFail) {
    //Given malloc is injected to fail for the token decode in init
    celix_ei_expect_malloc((void*)celix_json_pointer_init, 0, nullptr);
    celix_json_pointer_t p{};
    //Then initializing a pointer should fail
    EXPECT_EQ(-1, celix_json_pointer_init(&p, "/a"));
    celix_json_pointer_clear(&p);
}

TEST_F(JanssonExtErrorInjectionTestSuite, PointerInitPushStrdupFail) {
    //Given strdup is injected to fail in push while init pushes the first token
    celix_ei_expect_strdup((void*)celix_json_pointer_push, 0, nullptr);
    celix_json_pointer_t p{};
    //Then initializing a pointer should fail
    EXPECT_EQ(-1, celix_json_pointer_init(&p, "/a/b"));
    celix_json_pointer_clear(&p);
}

TEST_F(JanssonExtErrorInjectionTestSuite, PointerInitTrailingSlashPushFail) {
    //Given strdup is injected to fail only on the second push (the empty trailing token)
    celix_ei_expect_strdup((void*)celix_json_pointer_push, 0, nullptr, 2);
    celix_json_pointer_t p{};
    //Then initializing "/a/" should fail
    EXPECT_EQ(-1, celix_json_pointer_init(&p, "/a/"));
    celix_json_pointer_clear(&p);
}

TEST_F(JanssonExtErrorInjectionTestSuite, PointerCopyCallocFail) {
    //Given a pointer with one token
    celix_autoptr(celix_json_pointer_t) src = celix_json_pointer_create("/a");
    ASSERT_NE(nullptr, src);
    //And calloc is injected to fail in copy
    celix_ei_expect_calloc((void*)celix_json_pointer_copy, 0, nullptr);
    //Then copying should fail
    EXPECT_EQ(nullptr, celix_json_pointer_copy(src));
}

TEST_F(JanssonExtErrorInjectionTestSuite, PointerCopyEnsureCapReallocFail) {
    //Given a pointer with one token and realloc is injected to fail in ensure_cap (called from copy)
    celix_autoptr(celix_json_pointer_t) src = celix_json_pointer_create("/a");
    ASSERT_NE(nullptr, src);
    celix_ei_expect_realloc((void*)celix_json_pointer_copy, 1, nullptr);
    //Then copying should fail
    EXPECT_EQ(nullptr, celix_json_pointer_copy(src));
}

TEST_F(JanssonExtErrorInjectionTestSuite, PointerCopyStrdupFail) {
    //Given a pointer with one token and strdup is injected to fail in copy
    celix_autoptr(celix_json_pointer_t) src = celix_json_pointer_create("/a");
    ASSERT_NE(nullptr, src);
    celix_ei_expect_strdup((void*)celix_json_pointer_copy, 0, nullptr);
    //Then copying should fail
    EXPECT_EQ(nullptr, celix_json_pointer_copy(src));
}

TEST_F(JanssonExtErrorInjectionTestSuite, PointerPushStrdupFail) {
    //Given strdup is injected to fail in push
    celix_ei_expect_strdup((void*)celix_json_pointer_push, 0, nullptr);
    celix_json_pointer_t p{};
    //Then pushing a token should fail
    EXPECT_EQ(-1, celix_json_pointer_push(&p, "abc"));
    celix_json_pointer_clear(&p);
}

TEST_F(JanssonExtErrorInjectionTestSuite, PointerToStringMallocFail) {
    //Given a pointer with one token and malloc is injected to fail in to_string
    celix_autoptr(celix_json_pointer_t) src = celix_json_pointer_create("/a");
    ASSERT_NE(nullptr, src);
    celix_ei_expect_malloc((void*)celix_json_pointer_to_string, 0, nullptr);
    //Then serializing should fail
    EXPECT_EQ(nullptr, celix_json_pointer_to_string(src));
}

TEST_F(JanssonExtErrorInjectionTestSuite, PointerEscapeMallocFail) {
    //Given malloc is injected to fail in escape
    celix_ei_expect_malloc((void*)celix_json_pointer_escape, 0, nullptr);
    //Then escaping should fail
    EXPECT_EQ(nullptr, celix_json_pointer_escape("a/b"));
}

TEST_F(JanssonExtErrorInjectionTestSuite, PointerUnescapeMallocFail) {
    //Given malloc is injected to fail in unescape
    celix_ei_expect_malloc((void*)celix_json_pointer_unescape, 0, nullptr);
    //Then unescaping should fail
    EXPECT_EQ(nullptr, celix_json_pointer_unescape("a~1b"));
}

TEST_F(JanssonExtErrorInjectionTestSuite, PointerParentCallocFail) {
    //Given a two-token pointer and calloc is injected to fail in parent (out == nullptr, self-allocated)
    celix_autoptr(celix_json_pointer_t) src = celix_json_pointer_create("/a/b");
    ASSERT_NE(nullptr, src);
    celix_ei_expect_calloc((void*)celix_json_pointer_parent, 0, nullptr);
    //Then computing the parent should fail
    EXPECT_EQ(nullptr, celix_json_pointer_parent(src, nullptr));
}

TEST_F(JanssonExtErrorInjectionTestSuite, PointerParentEnsureCapFail) {
    //Given a two-token pointer and realloc is injected to fail in ensure_cap (called from parent)
    celix_autoptr(celix_json_pointer_t) src = celix_json_pointer_create("/a/b");
    ASSERT_NE(nullptr, src);
    celix_ei_expect_realloc((void*)celix_json_pointer_parent, 1, nullptr);
    //Then computing the parent should fail
    EXPECT_EQ(nullptr, celix_json_pointer_parent(src, nullptr));
}

TEST_F(JanssonExtErrorInjectionTestSuite, PointerParentStrdupFail) {
    //Given a two-token pointer and strdup is injected to fail in parent
    celix_autoptr(celix_json_pointer_t) src = celix_json_pointer_create("/a/b");
    ASSERT_NE(nullptr, src);
    celix_ei_expect_strdup((void*)celix_json_pointer_parent, 0, nullptr);
    //Then computing the parent should fail
    EXPECT_EQ(nullptr, celix_json_pointer_parent(src, nullptr));
}

TEST_F(JanssonExtErrorInjectionTestSuite, PointerConcatPushFail) {
    //Given strdup is injected to fail in push (used by concat)
    celix_autoptr(celix_json_pointer_t) dst = celix_json_pointer_create("");
    celix_autoptr(celix_json_pointer_t) suffix = celix_json_pointer_create("/x");
    ASSERT_NE(nullptr, dst);
    ASSERT_NE(nullptr, suffix);
    celix_ei_expect_strdup((void*)celix_json_pointer_push, 0, nullptr);
    //Then concatenating should fail
    EXPECT_EQ(-1, celix_json_pointer_concat(dst, suffix));
}

TEST_F(JanssonExtErrorInjectionTestSuite, PointerInitRootEmptyPushFail) {
    //Given strdup is injected to fail in push while init parses the root-empty pointer "/"
    celix_ei_expect_strdup((void*)celix_json_pointer_push, 0, nullptr);
    celix_json_pointer_t p{};
    //Then initializing should fail and leave the pointer in the cleared state
    EXPECT_EQ(-1, celix_json_pointer_init(&p, "/"));
    EXPECT_EQ(nullptr, p.tokens);
    EXPECT_EQ(0u, celix_json_pointer_depth(&p));
}

TEST_F(JanssonExtErrorInjectionTestSuite, PointerCreateCleanupOnInitFail) {
    //Given strdup is injected to fail on the second push while create parses "/a/b"
    celix_ei_expect_strdup((void*)celix_json_pointer_push, 0, nullptr, 2);
    //Then creating should fail without leaking the first token or the tokens buffer
    EXPECT_EQ(nullptr, celix_json_pointer_create("/a/b"));
}

TEST_F(JanssonExtErrorInjectionTestSuite, PointerGetOrCreateNullFail) {
    //Given json_null is injected to fail for the final node in get_or_create
    json_auto_t* doc = json_object();
    celix_autoptr(celix_json_pointer_t) p = celix_json_pointer_create("/a");
    ASSERT_NE(nullptr, doc);
    ASSERT_NE(nullptr, p);
    celix_ei_expect_json_null((void*)celix_json_pointer_get_or_create, 0, nullptr);
    //Then get_or_create should fail and not insert the node
    EXPECT_EQ(nullptr, celix_json_pointer_get_or_create(doc, p));
    EXPECT_EQ(nullptr, json_object_get(doc, "a"));
}

TEST_F(JanssonExtErrorInjectionTestSuite, PointerGetOrCreateSetFail) {
    //Given json_object_set_new is injected to fail in the final write of get_or_create.
    //json_object_set is a static inline calling set_new; level 1 resolves to get_or_create.
    json_auto_t* doc = json_object();
    celix_autoptr(celix_json_pointer_t) p = celix_json_pointer_create("/a");
    ASSERT_NE(nullptr, doc);
    ASSERT_NE(nullptr, p);
    celix_ei_expect_json_object_set_new((void*)celix_json_pointer_get_or_create, 1, -1);
    //Then get_or_create should fail and not insert the node
    EXPECT_EQ(nullptr, celix_json_pointer_get_or_create(doc, p));
    EXPECT_EQ(nullptr, json_object_get(doc, "a"));
}

TEST_F(JanssonExtErrorInjectionTestSuite, PointerGetOrCreateIntermediateSetNewFail) {
    //Given json_object_set_new is injected to fail while inserting the intermediate object
    json_auto_t* doc = json_object();
    celix_autoptr(celix_json_pointer_t) p = celix_json_pointer_create("/a/b");
    ASSERT_NE(nullptr, doc);
    ASSERT_NE(nullptr, p);
    celix_ei_expect_json_object_set_new((void*)celix_json_pointer_get_or_create, 0, -1);
    //Then get_or_create should fail and not insert the intermediate node
    EXPECT_EQ(nullptr, celix_json_pointer_get_or_create(doc, p));
    EXPECT_EQ(nullptr, json_object_get(doc, "a"));
}

TEST_F(JanssonExtErrorInjectionTestSuite, PointerGetOrCreateIntermediateJsonObjectFail) {
    //Given json_object is injected to fail for the intermediate container in get_or_create
    json_auto_t* doc = json_object();
    celix_autoptr(celix_json_pointer_t) p = celix_json_pointer_create("/a/b");
    ASSERT_NE(nullptr, doc);
    ASSERT_NE(nullptr, p);
    celix_ei_expect_json_object((void*)celix_json_pointer_get_or_create, 0, nullptr);
    //Then get_or_create should fail and not insert the intermediate node
    EXPECT_EQ(nullptr, celix_json_pointer_get_or_create(doc, p));
    EXPECT_EQ(nullptr, json_object_get(doc, "a"));
}

TEST_F(JanssonExtErrorInjectionTestSuite, PointerGetOrCreatePaddingNullFail) {
    //Given json_null is injected to fail while padding the target array in get_or_create
    json_auto_t* doc = json_array();
    celix_autoptr(celix_json_pointer_t) p = celix_json_pointer_create("/2");
    ASSERT_NE(nullptr, doc);
    ASSERT_NE(nullptr, p);
    celix_ei_expect_json_null((void*)celix_json_pointer_get_or_create, 0, nullptr);
    //Then get_or_create should fail and the array must remain empty
    EXPECT_EQ(nullptr, celix_json_pointer_get_or_create(doc, p));
    EXPECT_EQ(0u, json_array_size(doc));
}

TEST_F(JanssonExtErrorInjectionTestSuite, PointerGetOrCreatePaddingAppendFail) {
    //Given json_array_append_new is injected to fail while padding the target array in get_or_create
    json_auto_t* doc = json_array();
    celix_autoptr(celix_json_pointer_t) p = celix_json_pointer_create("/2");
    ASSERT_NE(nullptr, doc);
    ASSERT_NE(nullptr, p);
    celix_ei_expect_json_array_append_new((void*)celix_json_pointer_get_or_create, 0, -1);
    //Then get_or_create should fail and the array must remain empty
    EXPECT_EQ(nullptr, celix_json_pointer_get_or_create(doc, p));
    EXPECT_EQ(0u, json_array_size(doc));
}

TEST_F(JanssonExtErrorInjectionTestSuite, PointerGetOrCreateDashAppendFail) {
    //Given json_array_append_new is injected to fail while appending the "-" element in get_or_create
    json_auto_t* doc = json_array();
    celix_autoptr(celix_json_pointer_t) p = celix_json_pointer_create("/-");
    ASSERT_NE(nullptr, doc);
    ASSERT_NE(nullptr, p);
    celix_ei_expect_json_array_append_new((void*)celix_json_pointer_get_or_create, 0, -1);
    //Then get_or_create should fail and the array must remain empty
    EXPECT_EQ(nullptr, celix_json_pointer_get_or_create(doc, p));
    EXPECT_EQ(0u, json_array_size(doc));
}

TEST_F(JanssonExtErrorInjectionTestSuite, PointerGetOrCreateDashNullFail) {
    //Given json_null is injected to fail while appending the "-" element in get_or_create
    json_auto_t* doc = json_array();
    celix_autoptr(celix_json_pointer_t) p = celix_json_pointer_create("/-");
    ASSERT_NE(nullptr, doc);
    ASSERT_NE(nullptr, p);
    celix_ei_expect_json_null((void*)celix_json_pointer_get_or_create, 0, nullptr);
    //Then get_or_create should fail and the array must remain empty
    EXPECT_EQ(nullptr, celix_json_pointer_get_or_create(doc, p));
    EXPECT_EQ(0u, json_array_size(doc));
}

TEST_F(JanssonExtErrorInjectionTestSuite, PointerSetFinalSetNewFail) {
    //Given json_object_set_new is injected to fail in the final write of set
    json_auto_t* doc = json_object();
    celix_autoptr(celix_json_pointer_t) p = celix_json_pointer_create("/a");
    ASSERT_NE(nullptr, doc);
    ASSERT_NE(nullptr, p);
    celix_ei_expect_json_object_set_new((void*)celix_json_pointer_set, 0, -1);
    //Then setting should fail; the value is consumed by the failed set_new
    EXPECT_EQ(-1, celix_json_pointer_set(doc, p, json_integer(42)));
    EXPECT_EQ(nullptr, json_object_get(doc, "a"));
}

TEST_F(JanssonExtErrorInjectionTestSuite, PointerSetFinalAppendNewFail) {
    //Given json_array_append_new is injected to fail in the final "-" write of set
    json_auto_t* doc = json_loads(R"({"a":[1,2]})", 0, nullptr);
    celix_autoptr(celix_json_pointer_t) p = celix_json_pointer_create("/a/-");
    ASSERT_NE(nullptr, doc);
    ASSERT_NE(nullptr, p);
    celix_ei_expect_json_array_append_new((void*)celix_json_pointer_set, 0, -1);
    //Then setting should fail and the document must remain unchanged
    EXPECT_EQ(-1, celix_json_pointer_set(doc, p, json_integer(42)));
    char* dumped = json_dumps(doc, JSON_COMPACT);
    ASSERT_NE(nullptr, dumped);
    EXPECT_STREQ(R"({"a":[1,2]})", dumped);
    free(dumped);
}

TEST_F(JanssonExtErrorInjectionTestSuite, PointerSetIntermediateSetNewFail) {
    //Given json_object_set_new is injected to fail while inserting the intermediate object in set
    json_auto_t* doc = json_object();
    celix_autoptr(celix_json_pointer_t) p = celix_json_pointer_create("/a/b");
    ASSERT_NE(nullptr, doc);
    ASSERT_NE(nullptr, p);
    celix_ei_expect_json_object_set_new((void*)celix_json_pointer_set, 0, -1);
    //Then setting should fail and the intermediate node must not be inserted
    EXPECT_EQ(-1, celix_json_pointer_set(doc, p, json_integer(42)));
    EXPECT_EQ(nullptr, json_object_get(doc, "a"));
}

TEST_F(JanssonExtErrorInjectionTestSuite, PointerSetIntermediateJsonArrayFail) {
    //Given json_array is injected to fail for the intermediate container in set
    json_auto_t* doc = json_object();
    celix_autoptr(celix_json_pointer_t) p = celix_json_pointer_create("/a/0/x");
    ASSERT_NE(nullptr, doc);
    ASSERT_NE(nullptr, p);
    celix_ei_expect_json_array((void*)celix_json_pointer_set, 0, nullptr);
    //Then setting should fail and the intermediate node must not be inserted
    EXPECT_EQ(-1, celix_json_pointer_set(doc, p, json_integer(42)));
    EXPECT_EQ(nullptr, json_object_get(doc, "a"));
}

TEST_F(JanssonExtErrorInjectionTestSuite, PointerSetIntermediateReplFail) {
    //Given json_object is injected to fail for the replacement container in set
    json_auto_t* doc = json_loads("[1]", 0, nullptr);
    celix_autoptr(celix_json_pointer_t) p = celix_json_pointer_create("/0/b");
    ASSERT_NE(nullptr, doc);
    ASSERT_NE(nullptr, p);
    celix_ei_expect_json_object((void*)celix_json_pointer_set, 0, nullptr);
    //Then setting should fail and the document must remain unchanged
    EXPECT_EQ(-1, celix_json_pointer_set(doc, p, json_integer(42)));
    EXPECT_EQ(1u, json_array_size(doc));
}

TEST_F(JanssonExtErrorInjectionTestSuite, PointerSetPaddingAppendFail) {
    //Given json_array_append_new is injected to fail while padding the target array in set
    json_auto_t* doc = json_loads(R"({"arr":[]})", 0, nullptr);
    celix_autoptr(celix_json_pointer_t) p = celix_json_pointer_create("/arr/2");
    ASSERT_NE(nullptr, doc);
    ASSERT_NE(nullptr, p);
    celix_ei_expect_json_array_append_new((void*)celix_json_pointer_set, 0, -1);
    //Then setting should fail and the array must remain empty
    EXPECT_EQ(-1, celix_json_pointer_set(doc, p, json_integer(42)));
    EXPECT_EQ(0u, json_array_size(json_object_get(doc, "arr")));
}

TEST_F(JanssonExtErrorInjectionTestSuite, PointerSetPaddingNullFail) {
    //Given json_null is injected to fail while padding the target array in set
    json_auto_t* doc = json_loads(R"({"arr":[]})", 0, nullptr);
    celix_autoptr(celix_json_pointer_t) p = celix_json_pointer_create("/arr/2");
    ASSERT_NE(nullptr, doc);
    ASSERT_NE(nullptr, p);
    celix_ei_expect_json_null((void*)celix_json_pointer_set, 0, nullptr);
    //Then setting should fail and the array must remain empty
    EXPECT_EQ(-1, celix_json_pointer_set(doc, p, json_integer(42)));
    EXPECT_EQ(0u, json_array_size(json_object_get(doc, "arr")));
}

TEST_F(JanssonExtErrorInjectionTestSuite, PointerSetIntermediatePaddingAppendFail) {
    //Given json_array_append_new is injected to fail while padding an intermediate array in set.
    //"/arr/2/0" makes the padding happen before the walk descends into the new array slot.
    json_auto_t* doc = json_loads(R"({"arr":[]})", 0, nullptr);
    celix_autoptr(celix_json_pointer_t) p = celix_json_pointer_create("/arr/2/0");
    ASSERT_NE(nullptr, doc);
    ASSERT_NE(nullptr, p);
    celix_ei_expect_json_array_append_new((void*)celix_json_pointer_set, 0, -1);
    //Then setting should fail and the array must remain empty
    EXPECT_EQ(-1, celix_json_pointer_set(doc, p, json_integer(42)));
    EXPECT_EQ(0u, json_array_size(json_object_get(doc, "arr")));
}

TEST_F(JanssonExtErrorInjectionTestSuite, PointerRemovePushFail) {
    //Given strdup is injected to fail while building the parent pointer in remove.
    //The root also has a key "c", so the OLD code would resolve the parent to the
    //root and delete the wrong node.
    json_auto_t* doc = json_loads(R"({"a":{"c":1},"c":99})", 0, nullptr);
    celix_autoptr(celix_json_pointer_t) p = celix_json_pointer_create("/a/c");
    ASSERT_NE(nullptr, doc);
    ASSERT_NE(nullptr, p);
    celix_ei_expect_strdup((void*)celix_json_pointer_push, 0, nullptr);
    //Then removing should fail and the document must remain unchanged
    EXPECT_EQ(-1, celix_json_pointer_remove(doc, p));
    char* dumped = json_dumps(doc, JSON_COMPACT);
    ASSERT_NE(nullptr, dumped);
    EXPECT_STREQ(R"({"a":{"c":1},"c":99})", dumped);
    free(dumped);
}

/* ── celix_jansson_uri.c ──────────────────────────────────────────────── */

TEST_F(JanssonExtErrorInjectionTestSuite, UriUpdateLocationMallocFail) {
    //Given malloc is injected to fail for the location in update
    celix_jansson_uri_t u{};
    celix_ei_expect_malloc((void*)celix_jansson_uri_update, 0, nullptr);
    //Then updating the URI should fail with NOMEM
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_uri_update(&u, "http://example.com"));
    celix_jansson_uri_clear(&u);
}

TEST_F(JanssonExtErrorInjectionTestSuite, UriUpdatePercentDecodeReallocFail) {
    //Given realloc is injected to fail in strbuf_append (used by percent_decode).
    //The single-char fragment makes percent_decode loop exactly once, so the
    //single injected realloc failure is not recovered by a later iteration.
    celix_jansson_uri_t u{};
    celix_ei_expect_realloc((void*)celix_jansson_strbuf_append, 0, nullptr);
    //Then updating a URI with a fragment should fail with NOMEM
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_uri_update(&u, "http://example.com#a"));
    celix_jansson_uri_clear(&u);
}

TEST_F(JanssonExtErrorInjectionTestSuite, UriUpdateInvalidPointerFragment) {
    //Given a fragment that is not a valid JSON Pointer ("~" without escape character)
    celix_jansson_uri_t u{};
    //Then updating should fail with NOMEM (pointer init failure is mapped to NOMEM)
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_uri_update(&u, "http://example.com#/a~"));
    celix_jansson_uri_clear(&u);
}

TEST_F(JanssonExtErrorInjectionTestSuite, UriAppendCopyPushFail) {
    //Given a URI with one pointer token and strdup is injected to fail in push while copying tokens
    celix_jansson_uri_t u{};
    ASSERT_EQ(0, celix_jansson_uri_init(&u, "#/a"));
    celix_jansson_uri_t out{};
    celix_ei_expect_strdup((void*)celix_json_pointer_push, 0, nullptr);
    //Then appending a token should fail with NOMEM
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_uri_append(&u, "x", &out));
    celix_jansson_uri_clear(&u);
    celix_jansson_uri_clear(&out);
}

TEST_F(JanssonExtErrorInjectionTestSuite, UriAppendFinalPushFail) {
    //Given a URI without pointer tokens and strdup is injected to fail in push for the final token
    celix_jansson_uri_t u{};
    ASSERT_EQ(0, celix_jansson_uri_init(&u, "http://example.com"));
    celix_jansson_uri_t out{};
    celix_ei_expect_strdup((void*)celix_json_pointer_push, 0, nullptr);
    //Then appending a token should fail with NOMEM
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_uri_append(&u, "x", &out));
    celix_jansson_uri_clear(&u);
    celix_jansson_uri_clear(&out);
}

TEST_F(JanssonExtErrorInjectionTestSuite, UriUpdateSchemeMallocFail) {
    //Given malloc is injected to fail for the scheme (update's 2nd malloc:
    //#1 is the location buffer; the authority is strdup'd, not malloc'd)
    celix_jansson_uri_t u{};
    celix_ei_expect_malloc((void*)celix_jansson_uri_update, 0, nullptr, 2);
    //Then updating the URI should fail with NOMEM instead of silently dropping the scheme
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_uri_update(&u, "http://example.com"));
    celix_jansson_uri_clear(&u);
}

TEST_F(JanssonExtErrorInjectionTestSuite, UriUpdateAuthorityStrdupFail) {
    //Given strdup is injected to fail for the authority ("http://example.com" has
    //no slash, so its only strdup is the authority)
    celix_jansson_uri_t u{};
    celix_ei_expect_strdup((void*)celix_jansson_uri_update, 0, nullptr);
    //Then updating the URI should fail with NOMEM
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_uri_update(&u, "http://example.com"));
    celix_jansson_uri_clear(&u);
}

TEST_F(JanssonExtErrorInjectionTestSuite, UriUpdatePathStrdupFail) {
    //Given strdup is injected to fail for the path ("http://example.com/path" has
    //a slash, so its first strdup is the path)
    celix_jansson_uri_t u{};
    celix_ei_expect_strdup((void*)celix_jansson_uri_update, 0, nullptr);
    //Then updating the URI should fail with NOMEM
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_uri_update(&u, "http://example.com/path"));
    celix_jansson_uri_clear(&u);
}

TEST_F(JanssonExtErrorInjectionTestSuite, UriUpdateOldPathStrdupFail) {
    //Given strdup is injected to fail for the old-path copy during relative resolution
    celix_jansson_uri_t u{};
    ASSERT_EQ(0, celix_jansson_uri_init(&u, "http://example.com/a"));
    celix_ei_expect_strdup((void*)celix_jansson_uri_update, 0, nullptr);
    //Then updating should fail with NOMEM before u is modified
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_uri_update(&u, "b"));
    ASSERT_NE(nullptr, u.path);
    EXPECT_STREQ("/a", u.path);
    celix_jansson_uri_clear(&u);
}

TEST_F(JanssonExtErrorInjectionTestSuite, UriDeriveCopyStrdupFail) {
    //Given strdup is injected to fail while copying the base components in derive
    celix_jansson_uri_t base{};
    ASSERT_EQ(0, celix_jansson_uri_init(&base, "http://example.com/a"));
    celix_jansson_uri_t out{};
    celix_ei_expect_strdup((void*)celix_jansson_uri_derive, 0, nullptr);
    //Then deriving should fail with NOMEM (out is left cleared)
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_uri_derive(&base, "x", &out));
    celix_jansson_uri_clear(&base);
    celix_jansson_uri_clear(&out);
}

TEST_F(JanssonExtErrorInjectionTestSuite, UriDerivePointerPushFail) {
    //Given strdup is injected to fail in push while copying the base pointer tokens.
    //The base copy produces no strdup for "#/a" (no location components), so the
    //first push strdup fails — today derive silently succeeds with a dropped token.
    celix_jansson_uri_t base{};
    ASSERT_EQ(0, celix_jansson_uri_init(&base, "#/a"));
    celix_jansson_uri_t out{};
    celix_ei_expect_strdup((void*)celix_json_pointer_push, 0, nullptr);
    //Then deriving should fail with NOMEM
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_uri_derive(&base, "#", &out));
    celix_jansson_uri_clear(&base);
    celix_jansson_uri_clear(&out);
}

TEST_F(JanssonExtErrorInjectionTestSuite, UriAppendIdentifierStrdupFail) {
    //Given strdup is injected to fail for the identifier copy in append.
    //Today append returns 0 unconditionally on this path.
    celix_jansson_uri_t u{};
    ASSERT_EQ(0, celix_jansson_uri_init(&u, "#abc"));
    celix_jansson_uri_t out{};
    celix_ei_expect_strdup((void*)celix_jansson_uri_append, 0, nullptr);
    //Then appending should fail with NOMEM
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_uri_append(&u, "x", &out));
    celix_jansson_uri_clear(&u);
    celix_jansson_uri_clear(&out);
}

TEST_F(JanssonExtErrorInjectionTestSuite, UriLocationUrnStrdupFail) {
    //Given strdup is injected to fail in location for the URN copy
    celix_jansson_uri_t u{};
    ASSERT_EQ(0, celix_jansson_uri_init(&u, "urn:foo"));
    celix_ei_expect_strdup((void*)celix_jansson_uri_location, 0, nullptr);
    //Then location returns NULL (OOM signal)
    EXPECT_EQ(nullptr, celix_jansson_uri_location(&u));
    celix_jansson_uri_clear(&u);
}

TEST_F(JanssonExtErrorInjectionTestSuite, UriToStringNoCrashOnLocationOom) {
    //Given strdup is injected to fail in location while to_string runs
    //(location returns NULL; today to_string crashes with strlen(NULL))
    celix_jansson_uri_t u{};
    ASSERT_EQ(0, celix_jansson_uri_init(&u, "urn:foo"));
    celix_ei_expect_strdup((void*)celix_jansson_uri_location, 0, nullptr);
    //Then to_string must not crash and returns NULL (OOM signal)
    EXPECT_EQ(nullptr, celix_jansson_uri_to_string(&u));
    celix_jansson_uri_clear(&u);
}

TEST_F(JanssonExtErrorInjectionTestSuite, UriEscapeEmpty) {
    //Given an empty string (no injection)
    //Then escape("") must return "" rather than NULL
    char* r = celix_jansson_uri_escape("");
    ASSERT_NE(nullptr, r);
    EXPECT_STREQ("", r);
    free(r);
}

TEST_F(JanssonExtErrorInjectionTestSuite, UriEscapeAppendFail) {
    //Given realloc is injected to fail in strbuf_append while escaping.
    //"a~" appends exactly once before the failure, so the one-shot injection
    //is not recovered by a later iteration.
    celix_ei_expect_realloc((void*)celix_jansson_strbuf_append, 0, nullptr);
    //Then escape must return NULL (OOM signal), not a partial string
    EXPECT_EQ(nullptr, celix_jansson_uri_escape("a~"));
}

TEST_F(JanssonExtErrorInjectionTestSuite, UriEqualsOomNoCrash) {
    //Given strdup is injected to fail in location for the first URI
    celix_jansson_uri_t a{};
    celix_jansson_uri_t b{};
    ASSERT_EQ(0, celix_jansson_uri_init(&a, "urn:foo"));
    ASSERT_EQ(0, celix_jansson_uri_init(&b, "urn:bar"));
    celix_ei_expect_strdup((void*)celix_jansson_uri_location, 0, nullptr);
    //Then equals degrades to false instead of strcmp(NULL, ...) crashing
    EXPECT_FALSE(celix_jansson_uri_equals(&a, &b));
    celix_jansson_uri_clear(&a);
    celix_jansson_uri_clear(&b);
}

TEST_F(JanssonExtErrorInjectionTestSuite, UriInitFailureLeavesCleared) {
    //Given malloc is injected to fail for the location buffer in update
    celix_jansson_uri_t u{};
    celix_ei_expect_malloc((void*)celix_jansson_uri_update, 0, nullptr);
    //Then init fails with NOMEM and u is left fully cleared
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_uri_init(&u, "http://example.com"));
    EXPECT_EQ(nullptr, u.scheme);
    EXPECT_EQ(nullptr, u.authority);
    EXPECT_EQ(nullptr, u.path);
    celix_jansson_uri_clear(&u);
}

TEST_F(JanssonExtErrorInjectionTestSuite, UriUpdateOldSchemeStrdupFail) {
    //Given strdup is injected to fail for the old-scheme copy (2nd strdup in update)
    celix_jansson_uri_t u{};
    ASSERT_EQ(0, celix_jansson_uri_init(&u, "http://example.com/a"));
    celix_ei_expect_strdup((void*)celix_jansson_uri_update, 0, nullptr, 2);
    //Then updating should fail with NOMEM before u is modified
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_uri_update(&u, "b"));
    ASSERT_NE(nullptr, u.scheme);
    EXPECT_STREQ("http", u.scheme);
    celix_jansson_uri_clear(&u);
}

TEST_F(JanssonExtErrorInjectionTestSuite, UriUpdateOldAuthorityStrdupFail) {
    //Given strdup is injected to fail for the old-authority copy (3rd strdup in update)
    celix_jansson_uri_t u{};
    ASSERT_EQ(0, celix_jansson_uri_init(&u, "http://example.com/a"));
    celix_ei_expect_strdup((void*)celix_jansson_uri_update, 0, nullptr, 3);
    //Then updating should fail with NOMEM before u is modified
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_uri_update(&u, "b"));
    ASSERT_NE(nullptr, u.authority);
    EXPECT_STREQ("example.com", u.authority);
    celix_jansson_uri_clear(&u);
}

TEST_F(JanssonExtErrorInjectionTestSuite, UriUpdateAuthorityMallocFail) {
    //Given malloc is injected to fail for the authority (update's 3rd malloc:
    //#1 location, #2 scheme, #3 authority — the path is strdup'd)
    celix_jansson_uri_t u{};
    celix_ei_expect_malloc((void*)celix_jansson_uri_update, 0, nullptr, 3);
    //Then updating the URI should fail with NOMEM
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_uri_update(&u, "http://example.com/path"));
    celix_jansson_uri_clear(&u);
}

TEST_F(JanssonExtErrorInjectionTestSuite, UriUpdateAbsolutePathStrdupFail) {
    //Given strdup is injected to fail for the absolute-path copy
    //(fresh u has no old components, so the first strdup is the path)
    celix_jansson_uri_t u{};
    celix_ei_expect_strdup((void*)celix_jansson_uri_update, 0, nullptr);
    //Then updating with an absolute path should fail with NOMEM
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_uri_update(&u, "/x"));
    celix_jansson_uri_clear(&u);
}

TEST_F(JanssonExtErrorInjectionTestSuite, UriUpdateRelativeStrbufFail) {
    //Given realloc is injected to fail in strbuf_append during relative-path
    //resolution (the only strbuf_append call in this update)
    celix_jansson_uri_t u{};
    ASSERT_EQ(0, celix_jansson_uri_init(&u, "http://example.com/a/b"));
    celix_ei_expect_realloc((void*)celix_jansson_strbuf_append, 0, nullptr);
    //Then updating should fail with NOMEM
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_uri_update(&u, "x"));
    celix_jansson_uri_clear(&u);
}

TEST_F(JanssonExtErrorInjectionTestSuite, UriUpdateRelativeNoDirStrdupFail) {
    //Given a URI whose path has no directory (no '/') and strdup is injected
    //to fail for the relative-path copy (2nd strdup: #1 is the old-path copy)
    celix_jansson_uri_t u{};
    u.path = strdup("abc");
    ASSERT_NE(nullptr, u.path);
    celix_ei_expect_strdup((void*)celix_jansson_uri_update, 0, nullptr, 2);
    //Then updating should fail with NOMEM
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_uri_update(&u, "x"));
    celix_jansson_uri_clear(&u);
}

TEST_F(JanssonExtErrorInjectionTestSuite, UriUpdateRelativeNoBaseStrdupFail) {
    //Given a fresh URI (no base path) and strdup is injected to fail for the
    //relative-path copy (the first strdup in update)
    celix_jansson_uri_t u{};
    celix_ei_expect_strdup((void*)celix_jansson_uri_update, 0, nullptr);
    //Then updating should fail with NOMEM
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_uri_update(&u, "x"));
    celix_jansson_uri_clear(&u);
}

TEST_F(JanssonExtErrorInjectionTestSuite, UriDeriveAuthorityStrdupFail) {
    //Given strdup is injected to fail for the authority copy (2nd strdup in derive)
    celix_jansson_uri_t base{};
    ASSERT_EQ(0, celix_jansson_uri_init(&base, "http://example.com/a"));
    celix_jansson_uri_t out{};
    celix_ei_expect_strdup((void*)celix_jansson_uri_derive, 0, nullptr, 2);
    //Then deriving should fail with NOMEM
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_uri_derive(&base, "x", &out));
    celix_jansson_uri_clear(&base);
    celix_jansson_uri_clear(&out);
}

TEST_F(JanssonExtErrorInjectionTestSuite, UriDerivePathStrdupFail) {
    //Given strdup is injected to fail for the path copy (3rd strdup in derive)
    celix_jansson_uri_t base{};
    ASSERT_EQ(0, celix_jansson_uri_init(&base, "http://example.com/a"));
    celix_jansson_uri_t out{};
    celix_ei_expect_strdup((void*)celix_jansson_uri_derive, 0, nullptr, 3);
    //Then deriving should fail with NOMEM
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_uri_derive(&base, "x", &out));
    celix_jansson_uri_clear(&base);
    celix_jansson_uri_clear(&out);
}

TEST_F(JanssonExtErrorInjectionTestSuite, UriDeriveUrnStrdupFail) {
    //Given a base carrying all four components and strdup is injected to fail
    //for the urn copy (4th strdup in derive)
    celix_jansson_uri_t base{};
    base.scheme = strdup("http");
    base.authority = strdup("example.com");
    base.path = strdup("/a");
    base.urn = strdup("urn:foo");
    celix_jansson_uri_t out{};
    celix_ei_expect_strdup((void*)celix_jansson_uri_derive, 0, nullptr, 4);
    //Then deriving should fail with NOMEM
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_uri_derive(&base, "x", &out));
    celix_jansson_uri_clear(&base);
    celix_jansson_uri_clear(&out);
}

TEST_F(JanssonExtErrorInjectionTestSuite, UriDeriveIdentifierStrdupFail) {
    //Given a URI with an identifier fragment and strdup is injected to fail
    //while copying it in derive (no location components to copy)
    celix_jansson_uri_t u{};
    ASSERT_EQ(0, celix_jansson_uri_init(&u, "#abc"));
    celix_jansson_uri_t out{};
    celix_ei_expect_strdup((void*)celix_jansson_uri_derive, 0, nullptr);
    //Then deriving should fail with NOMEM
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_uri_derive(&u, "x", &out));
    celix_jansson_uri_clear(&u);
    celix_jansson_uri_clear(&out);
}

TEST_F(JanssonExtErrorInjectionTestSuite, UriAppendComponentStrdupFail) {
    //Given strdup is injected to fail while copying the scheme component in append
    celix_jansson_uri_t u{};
    ASSERT_EQ(0, celix_jansson_uri_init(&u, "http://example.com/a"));
    celix_jansson_uri_t out{};
    celix_ei_expect_strdup((void*)celix_jansson_uri_append, 0, nullptr);
    //Then appending a token should fail with NOMEM (out is left cleared)
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_uri_append(&u, "x", &out));
    celix_jansson_uri_clear(&u);
    celix_jansson_uri_clear(&out);
}

TEST_F(JanssonExtErrorInjectionTestSuite, UriAppendAuthorityStrdupFail) {
    //Given strdup is injected to fail while copying the authority component (2nd)
    celix_jansson_uri_t u{};
    ASSERT_EQ(0, celix_jansson_uri_init(&u, "http://example.com/a"));
    celix_jansson_uri_t out{};
    celix_ei_expect_strdup((void*)celix_jansson_uri_append, 0, nullptr, 2);
    //Then appending a token should fail with NOMEM (out is left cleared)
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_uri_append(&u, "x", &out));
    celix_jansson_uri_clear(&u);
    celix_jansson_uri_clear(&out);
}

TEST_F(JanssonExtErrorInjectionTestSuite, UriAppendPathStrdupFail) {
    //Given strdup is injected to fail while copying the path component (3rd)
    celix_jansson_uri_t u{};
    ASSERT_EQ(0, celix_jansson_uri_init(&u, "http://example.com/a"));
    celix_jansson_uri_t out{};
    celix_ei_expect_strdup((void*)celix_jansson_uri_append, 0, nullptr, 3);
    //Then appending a token should fail with NOMEM (out is left cleared)
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_uri_append(&u, "x", &out));
    celix_jansson_uri_clear(&u);
    celix_jansson_uri_clear(&out);
}

TEST_F(JanssonExtErrorInjectionTestSuite, UriAppendUrnStrdupFail) {
    //Given a URI carrying all four components and strdup is injected to fail
    //while copying the urn component (4th)
    celix_jansson_uri_t u{};
    u.scheme = strdup("http");
    u.authority = strdup("example.com");
    u.path = strdup("/a");
    u.urn = strdup("urn:foo");
    celix_jansson_uri_t out{};
    celix_ei_expect_strdup((void*)celix_jansson_uri_append, 0, nullptr, 4);
    //Then appending a token should fail with NOMEM (out is left cleared)
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_uri_append(&u, "x", &out));
    celix_jansson_uri_clear(&u);
    celix_jansson_uri_clear(&out);
}

TEST_F(JanssonExtErrorInjectionTestSuite, UriLocationSchemeAppendsFail) {
    //Given realloc is injected to fail in strbuf_append for the first append
    //(the scheme) inside location
    celix_jansson_uri_t u{};
    ASSERT_EQ(0, celix_jansson_uri_init(&u, "http://example.com/a"));
    celix_ei_expect_realloc((void*)celix_jansson_strbuf_append, 0, nullptr);
    //Then location returns NULL (OOM signal)
    EXPECT_EQ(nullptr, celix_jansson_uri_location(&u));
    celix_jansson_uri_clear(&u);
}

TEST_F(JanssonExtErrorInjectionTestSuite, UriLocationAuthorityAppendsFail) {
    //Given a long authority (scheme "://" fit the initial 64-byte buffer) and
    //realloc is injected to fail for the 2nd realloc: #1 scheme appends,
    //#2 authority appends (78 bytes needed > 64)
    celix_jansson_uri_t u{};
    ASSERT_EQ(0, celix_jansson_uri_init(&u, "http://aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
    celix_ei_expect_realloc((void*)celix_jansson_strbuf_append, 0, nullptr, 2);
    //Then location returns NULL (OOM signal)
    EXPECT_EQ(nullptr, celix_jansson_uri_location(&u));
    celix_jansson_uri_clear(&u);
}

TEST_F(JanssonExtErrorInjectionTestSuite, UriLocationPathAppendsFail) {
    //Given a long path (scheme "://" authority fit the initial 64-byte buffer)
    //and realloc is injected to fail for the 2nd realloc: #1 scheme appends,
    //#2 path appends (79 bytes needed > 64)
    celix_jansson_uri_t u{};
    ASSERT_EQ(0, celix_jansson_uri_init(&u, "http://aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"));
    celix_ei_expect_realloc((void*)celix_jansson_strbuf_append, 0, nullptr, 2);
    //Then location returns NULL (OOM signal)
    EXPECT_EQ(nullptr, celix_jansson_uri_location(&u));
    celix_jansson_uri_clear(&u);
}

TEST_F(JanssonExtErrorInjectionTestSuite, UriToStringAppendsFail) {
    //Given a URN (location returns via strdup, no strbuf) and realloc is
    //injected to fail for to_string's own first append of the location part
    celix_jansson_uri_t u{};
    ASSERT_EQ(0, celix_jansson_uri_init(&u, "urn:foo"));
    celix_ei_expect_realloc((void*)celix_jansson_strbuf_append, 0, nullptr);
    //Then to_string returns NULL (OOM signal)
    EXPECT_EQ(nullptr, celix_jansson_uri_to_string(&u));
    celix_jansson_uri_clear(&u);
}

TEST_F(JanssonExtErrorInjectionTestSuite, UriToStringFragmentAppendsFail) {
    //Given a URN with a long fragment (location "#" fit the 64-byte buffer,
    //the fragment appends needs 132 bytes > 64) and realloc is injected to
    //fail for the 2nd realloc: #1 location appends, #2 fragment appends
    celix_jansson_uri_t u{};
    ASSERT_EQ(0, celix_jansson_uri_init(&u, "urn:aaaaaaaaaaaaaaaaaaaaaaaaaa#bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"));
    celix_ei_expect_realloc((void*)celix_jansson_strbuf_append, 0, nullptr, 2);
    //Then to_string returns NULL (OOM signal)
    EXPECT_EQ(nullptr, celix_jansson_uri_to_string(&u));
    celix_jansson_uri_clear(&u);
}

TEST_F(JanssonExtErrorInjectionTestSuite, UriEqualsFragmentOomNoCrash) {
    //Given equal locations and strdup is injected to fail in fragment for the
    //first URI — equals must degrade to false instead of strcmp(NULL, ...)
    celix_jansson_uri_t a{};
    celix_jansson_uri_t b{};
    ASSERT_EQ(0, celix_jansson_uri_init(&a, "urn:foo"));
    ASSERT_EQ(0, celix_jansson_uri_init(&b, "urn:foo"));
    celix_ei_expect_strdup((void*)celix_jansson_uri_fragment, 0, nullptr);
    //Then equals returns false without crashing
    EXPECT_FALSE(celix_jansson_uri_equals(&a, &b));
    celix_jansson_uri_clear(&a);
    celix_jansson_uri_clear(&b);
}

/* ── celix_json_patch.c ───────────────────────────────────────────────── */

TEST_F(JanssonExtErrorInjectionTestSuite, PatchAddJsonObjectFail) {
    //Given json_object is injected to fail in patch_add
    json_auto_t* patch = json_array();
    celix_ei_expect_json_object((void*)celix_json_patch_add, 0, nullptr);
    //Then adding an operation should fail; the value is not consumed on failure
    EXPECT_EQ(-1, celix_json_patch_add(patch, "/a", json_true()));
}

TEST_F(JanssonExtErrorInjectionTestSuite, PatchReplaceJsonObjectFail) {
    //Given json_object is injected to fail in patch_replace
    json_auto_t* patch = json_array();
    celix_ei_expect_json_object((void*)celix_json_patch_replace, 0, nullptr);
    //Then replacing should fail; the value is not consumed on failure
    EXPECT_EQ(-1, celix_json_patch_replace(patch, "/a", json_true()));
}

TEST_F(JanssonExtErrorInjectionTestSuite, PatchRemoveJsonObjectFail) {
    //Given json_object is injected to fail in patch_remove
    json_auto_t* patch = json_array();
    celix_ei_expect_json_object((void*)celix_json_patch_remove, 0, nullptr);
    //Then removing should fail
    EXPECT_EQ(-1, celix_json_patch_remove(patch, "/a"));
}

TEST_F(JanssonExtErrorInjectionTestSuite, PatchApplyDeepCopyFail) {
    //Given json_deep_copy is injected to fail in patch_apply
    json_auto_t* original = json_object();
    json_auto_t* patch = json_array();
    celix_ei_expect_json_deep_copy((void*)celix_json_patch_apply, 0, nullptr);
    //Then applying the patch should fail
    EXPECT_EQ(nullptr, celix_json_patch_apply(original, patch));
}
