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
