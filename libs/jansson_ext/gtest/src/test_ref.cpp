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
#include <cstring>

#include "celix_json_patch.h"
#include "test_common.h"

/* ── $ref to definitions ──────────────────────────────────────────────── */

TEST(RefTest, SimpleInternalRef) {
    static const char* schema = R"({
		"definitions": {
			"positiveInteger": {
				"type": "integer",
				"minimum": 1
			}
		},
		"$ref": "#/definitions/positiveInteger"
	})";

    auto* v = make_validator();
    ASSERT_NE(nullptr, v);

    json_t* sch = json_loads(schema, 0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    int rc = celix_jansson_schema_set_root_schema(v, sch, &errmsg);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, rc) << (errmsg ? errmsg : "");
    free(errmsg);

    /* Valid: 5 is a positive integer */
    reset_errors();
    json_t* inst = json_loads("5", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    EXPECT_EQ(0, celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr));
    json_decref(inst);

    /* Invalid: 0 is not >= 1 */
    inst = json_loads("0", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_GT(celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr), 0);
    json_decref(inst);

    json_decref(sch);
    free_validator(v);
}

/* ── Unresolved $ref ───────────────────────────────────────────────────── */

TEST(RefTest, UnresolvedRef) {
    static const char* schema = R"({
		"$ref": "#/definitions/nonexistent"
	})";

    auto* v = make_validator();
    ASSERT_NE(nullptr, v);

    json_t* sch = json_loads(schema, 0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    int rc = celix_jansson_schema_set_root_schema(v, sch, &errmsg);
    /* Should fail because the reference doesn't resolve */
    EXPECT_NE(CELIX_JANSSON_SCHEMA_OK, rc)
        << "Expected failure for unresolved $ref, got: " << (errmsg ? errmsg : "success");
    free(errmsg);

    json_decref(sch);
    free_validator(v);
}

/* ── $ref to root ──────────────────────────────────────────────────────── */

TEST(RefTest, RefToRoot) {
    static const char* schema = R"({
		"definitions": {
			"posInt": { "type": "integer", "minimum": 1 }
		},
		"type": "object",
		"properties": {
			"value": { "$ref": "#/definitions/posInt" }
		},
		"required": ["value"]
	})";

    auto* v = make_validator();
    ASSERT_NE(nullptr, v);

    json_t* sch = json_loads(schema, 0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    int rc = celix_jansson_schema_set_root_schema(v, sch, &errmsg);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, rc) << (errmsg ? errmsg : "");
    free(errmsg);

    /* Valid: positive integer */
    reset_errors();
    json_t* inst = json_loads(R"({"value":99})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    EXPECT_EQ(0, celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr));
    json_decref(inst);

    /* Invalid: 0 is not >= 1 */
    inst = json_loads(R"({"value":0})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_GT(celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr), 0);
    json_decref(inst);

    json_decref(sch);
    free_validator(v);
}

/* ── $ref via JSON pointer ────────────────────────────────────────────── */

TEST(RefTest, RefViaJsonPointer) {
    static const char* schema = R"({
		"properties": {
			"name": { "$ref": "#/definitions/nameType" }
		},
		"required": ["name"],
		"definitions": {
			"nameType": { "type": "string", "minLength": 1 }
		}
	})";

    auto* v = make_validator();
    ASSERT_NE(nullptr, v);

    json_t* sch = json_loads(schema, 0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    int rc = celix_jansson_schema_set_root_schema(v, sch, &errmsg);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, rc) << (errmsg ? errmsg : "");
    free(errmsg);

    /* Valid */
    reset_errors();
    json_t* inst = json_loads(R"({"name":"Alice"})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    EXPECT_EQ(0, celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr));
    json_decref(inst);

    /* Invalid: empty name */
    inst = json_loads(R"({"name":""})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_GT(celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr), 0);
    json_decref(inst);

    json_decref(sch);
    free_validator(v);
}

/* ── $ref with default override ────────────────────────────────────────── */

TEST(RefTest, RefWithDefaultOverride) {
    static const char* schema = R"({
		"definitions": {
			"withDefault": {
				"type": "integer",
				"default": 42
			}
		},
		"properties": {
			"val": {
				"$ref": "#/definitions/withDefault",
				"default": 99
			}
		}
	})";

    auto* v = make_validator();
    ASSERT_NE(nullptr, v);

    json_t* sch = json_loads(schema, 0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    int rc = celix_jansson_schema_set_root_schema(v, sch, &errmsg);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, rc) << (errmsg ? errmsg : "");
    free(errmsg);

    /* Validate with empty object — default should be generated */
    reset_errors();
    json_t* patch = nullptr;
    json_t* inst = json_loads("{}", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    int n = celix_jansson_schema_validate(v, inst, capture_error, nullptr, &patch);
    EXPECT_EQ(0, n);

    /* Check that the patch includes the default */
    ASSERT_NE(nullptr, patch);
    EXPECT_TRUE(json_is_array(patch));

    json_decref(inst);
    json_decref(patch);
    json_decref(sch);
    free_validator(v);
}

/* ── $ref default passthrough (ref has no default, target has one) ──────── */

TEST(RefTest, RefDefaultValuePassthrough) {
    static const char* schema = R"({
		"definitions": {
			"withDefault": {
				"type": "integer",
				"default": 42
			}
		},
		"type": "object",
		"properties": {
			"val": {
				"$ref": "#/definitions/withDefault"
			}
		}
	})";

    auto* v = make_validator();
    ASSERT_NE(nullptr, v);

    json_t* sch = json_loads(schema, 0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    int rc = celix_jansson_schema_set_root_schema(v, sch, &errmsg);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, rc) << (errmsg ? errmsg : "");
    free(errmsg);

    /* Validate empty object — the ref node carries no default of its own, so
     * dv_ref must inherit the default from the referenced definition. */
    reset_errors();
    json_t* patch = nullptr;
    json_t* inst = json_loads("{}", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    int n = celix_jansson_schema_validate(v, inst, capture_error, nullptr, &patch);
    EXPECT_EQ(0, n);

    json_t* filled = celix_json_patch_apply(inst, patch);
    ASSERT_NE(nullptr, filled);
    json_t* expected = json_loads(R"({"val":42})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, expected);
    EXPECT_TRUE(json_equal(filled, expected));

    json_decref(inst);
    json_decref(patch);
    json_decref(filled);
    json_decref(expected);
    json_decref(sch);
    free_validator(v);
}

/* ── Forward $ref to $id-based target (triggers root_insert placeholder) ─── */

TEST(RefTest, ForwardRefToIdBasedTarget) {
    /* Definition "A" references "http://example.com/b" via $ref.
     * Definition "B" declares $id "http://example.com/b" and appears
     * after "A" in JSON key order. Since definitions are compiled
     * iteratively, "A" sees a placeholder for "b"'s URI, which is
     * resolved when "B" is registered via celix_jansson_schema_root_insert.
     * This exercises the waiting-placeholder resolution at line 2167-2173. */
    static const char* schema = R"({
        "definitions": {
            "A": {
                "$ref": "http://example.com/b"
            },
            "B": {
                "$id": "http://example.com/b",
                "type": "integer",
                "minimum": 1
            }
        },
        "properties": {
            "value": { "$ref": "#/definitions/A" }
        }
    })";

    auto* v = make_validator();
    ASSERT_NE(nullptr, v);

    json_t* sch = json_loads(schema, 0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    int rc = celix_jansson_schema_set_root_schema(v, sch, &errmsg);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, rc) << (errmsg ? errmsg : "");
    free(errmsg);

    /* Valid: 5 resolves through A → B chain to positive integer */
    reset_errors();
    json_t* inst = json_loads(R"({"value":5})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    EXPECT_EQ(0, celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr));
    json_decref(inst);

    /* Invalid: 0 is not >= 1 */
    inst = json_loads(R"({"value":0})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_GT(celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr), 0);
    json_decref(inst);

    json_decref(sch);
    free_validator(v);
}

/* ── Forward $ref resolution (placeholder → registered schema) ───────────── */

TEST(RefTest, ForwardRefToDefinition) {
    /* Forward reference: $ref to a definition that appears later in the
     * JSON object. This exercises the placeholder → resolve path in
     * celix_jansson_schema_root_insert, where a waiting placeholder is
     * removed from sf->unresolved after the target node is registered. */
    static const char* schema = R"({
        "properties": {
            "value": { "$ref": "#/definitions/posInt" }
        },
        "definitions": {
            "posInt": {
                "type": "integer",
                "minimum": 1
            }
        }
    })";

    auto* v = make_validator();
    ASSERT_NE(nullptr, v);

    json_t* sch = json_loads(schema, 0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    int rc = celix_jansson_schema_set_root_schema(v, sch, &errmsg);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, rc) << (errmsg ? errmsg : "");
    free(errmsg);

    /* Valid: positive integer */
    reset_errors();
    json_t* inst = json_loads(R"({"value":42})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    EXPECT_EQ(0, celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr));
    json_decref(inst);

    /* Invalid: 0 is not >= 1 */
    inst = json_loads(R"({"value":0})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_GT(celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr), 0);
    json_decref(inst);

    json_decref(sch);
    free_validator(v);
}

/* ── Multiple forward $refs to same definition ───────────────────────────── */

TEST(RefTest, MultipleForwardRefsToSameDefinition) {
    /* Multiple forward $refs to the same definition. Each creates a
     * placeholder in sf->unresolved; the first one that gets resolved
     * via celix_jansson_schema_root_insert should remove and wire up
     * the placeholder. All of them should be cleaned up without leak. */
    static const char* schema = R"({
        "properties": {
            "a": { "$ref": "#/definitions/posInt" },
            "b": { "$ref": "#/definitions/posInt" }
        },
        "definitions": {
            "posInt": {
                "type": "integer",
                "minimum": 1
            }
        }
    })";

    auto* v = make_validator();
    ASSERT_NE(nullptr, v);

    json_t* sch = json_loads(schema, 0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    int rc = celix_jansson_schema_set_root_schema(v, sch, &errmsg);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, rc) << (errmsg ? errmsg : "");
    free(errmsg);

    /* Both properties should validate */
    reset_errors();
    json_t* inst = json_loads(R"({"a":5,"b":10})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    EXPECT_EQ(0, celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr));
    json_decref(inst);

    /* Both invalid */
    inst = json_loads(R"({"a":0,"b":-1})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_EQ(2, celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr));
    json_decref(inst);

    json_decref(sch);
    free_validator(v);
}

/* ── External $ref with no loader ─────────────────────────────────────── */

TEST(RefTest, ExternalRefWithoutLoader) {
    auto* v = celix_jansson_schema_validator_create(
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    ASSERT_NE(nullptr, v);

    json_t* sch = json_loads(R"({"$ref":"http://example.com/schema"})", 0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    int rc = celix_jansson_schema_set_root_schema(v, sch, &errmsg);
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_LOADER, rc);
    free(errmsg);

    json_decref(sch);
    free_validator(v);
}

/* ── Loader returns error ──────────────────────────────────────────────── */

static int failing_loader(const char* /*uri*/, json_t** /*out*/, void* /*ud*/) {
    return CELIX_JANSSON_SCHEMA_ERROR_LOADER;
}

TEST(RefTest, LoaderReturnsError) {
    auto* v = celix_jansson_schema_validator_create(
        failing_loader, nullptr, nullptr, nullptr, nullptr, nullptr);
    ASSERT_NE(nullptr, v);

    json_t* sch = json_loads(R"({"$ref":"http://example.com/schema"})", 0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    int rc = celix_jansson_schema_set_root_schema(v, sch, &errmsg);
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_LOADER, rc);
    free(errmsg);

    json_decref(sch);
    free_validator(v);
}

/* ── Loader returns non-schema value ───────────────────────────────────── */

static int non_schema_loader(const char* /*uri*/, json_t** out, void* /*ud*/) {
    *out = json_integer(42); /* not a schema — must be boolean or object */
    return CELIX_JANSSON_SCHEMA_OK;
}

TEST(RefTest, LoaderReturnsNonSchema) {
    auto* v = celix_jansson_schema_validator_create(
        non_schema_loader, nullptr, nullptr, nullptr, nullptr, nullptr);
    ASSERT_NE(nullptr, v);

    json_t* sch = json_loads(R"({"$ref":"http://example.com/schema"})", 0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    int rc = celix_jansson_schema_set_root_schema(v, sch, &errmsg);
    EXPECT_NE(CELIX_JANSSON_SCHEMA_OK, rc);
    free(errmsg);

    json_decref(sch);
    free_validator(v);
}

/* ── Multiple refs to same unresolved external target ──────────────────── */

TEST(RefTest, DuplicateUnresolvedExternalRef) {
    auto* v = celix_jansson_schema_validator_create(
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    ASSERT_NE(nullptr, v);

    json_t* sch = json_loads(
        R"({"properties":{"a":{"$ref":"http://missing/x#/a"},"b":{"$ref":"http://missing/x#/a"}}})",
        0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    int rc = celix_jansson_schema_set_root_schema(v, sch, &errmsg);
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_LOADER, rc);
    free(errmsg);

    json_decref(sch);
    free_validator(v);
}

/* ── Chained external $ref loader (root → A.json → B.json) ─────────────── */

struct chained_loader_ctx {
    json_t* a_schema; /* cached docs — loader deep-copies per call */
    json_t* b_schema;
    int a_calls;
    int b_calls;
    int fail_b; /* when set, loader returns LOADER error for B.json */
};

static int chained_loader(const char* uri, json_t** out, void* ud) {
    auto* ctx = static_cast<chained_loader_ctx*>(ud);
    if (strcmp(uri, "http://example.com/A.json") == 0) {
        ctx->a_calls++;
        *out = json_deep_copy(ctx->a_schema);
    } else if (strcmp(uri, "http://example.com/B.json") == 0) {
        ctx->b_calls++;
        if (ctx->fail_b) {
            return CELIX_JANSSON_SCHEMA_ERROR_LOADER;
        }
        *out = json_deep_copy(ctx->b_schema);
    } else {
        return CELIX_JANSSON_SCHEMA_ERROR_LOADER;
    }
    return *out ? CELIX_JANSSON_SCHEMA_OK : CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
}

static void init_chained_ctx(chained_loader_ctx* ctx, const char* a_json,
                             const char* b_json, int fail_b) {
    std::memset(ctx, 0, sizeof(*ctx));
    ctx->a_schema = json_loads(a_json, 0, nullptr);
    ctx->b_schema = json_loads(b_json, 0, nullptr);
    ctx->fail_b = fail_b;
    ASSERT_NE(nullptr, ctx->a_schema);
    ASSERT_NE(nullptr, ctx->b_schema);
}

static void free_chained_ctx(chained_loader_ctx* ctx) {
    json_decref(ctx->a_schema);
    json_decref(ctx->b_schema);
}

/* Shared root for the chained external-ref tests: root → A.json */
static const char* chained_ref_root = R"({
    "type": "object",
    "properties": {
        "value": { "$ref": "http://example.com/A.json" }
    }
})";

/* ── Chained external refs: B.json loaded in-place, whole-document ref ──── */

TEST(RefTest, ChainedExternalRefNoFragment) {
    /* Root → A.json → B.json.  A is loaded by Phase A of
     * resolve_external_refs; B's file entry is created during A's compile,
     * so B is loaded in-place by resolve_placeholder — the whole-document
     * ref gets auto-resolved by the root registration. */
    static const char* a_json = R"({"$ref": "http://example.com/B.json"})";
    static const char* b_json = R"({"type": "integer", "minimum": 1})";

    chained_loader_ctx ctx;
    init_chained_ctx(&ctx, a_json, b_json, 0);

    auto* v = celix_jansson_schema_validator_create(
        chained_loader, &ctx, nullptr, nullptr, nullptr, nullptr);
    ASSERT_NE(nullptr, v);

    json_t* sch = json_loads(chained_ref_root, 0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    int rc = celix_jansson_schema_set_root_schema(v, sch, &errmsg);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, rc) << (errmsg ? errmsg : "");
    free(errmsg);

    /* Valid: 5 is an integer >= 1 */
    json_t* inst = json_loads(R"({"value":5})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_EQ(0, celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr));
    json_decref(inst);

    /* Invalid: 0 is < 1 */
    inst = json_loads(R"({"value":0})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_GT(celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr), 0);
    json_decref(inst);

    /* Each document loaded exactly once */
    EXPECT_EQ(1, ctx.a_calls);
    EXPECT_EQ(1, ctx.b_calls);

    json_decref(sch);
    free_validator(v);
    free_chained_ctx(&ctx);
}

/* ── Chained external refs with a fragment walk ────────────────────────── */

TEST(RefTest, ChainedExternalRefWithFragment) {
    /* B.json is loaded in-place, then the /definitions/X fragment is
     * compiled by the document-fragment walk. */
    static const char* a_json = R"({"$ref": "http://example.com/B.json#/definitions/X"})";
    static const char* b_json = R"({
        "definitions": {
            "X": { "type": "integer", "minimum": 1 }
        }
    })";

    chained_loader_ctx ctx;
    init_chained_ctx(&ctx, a_json, b_json, 0);

    auto* v = celix_jansson_schema_validator_create(
        chained_loader, &ctx, nullptr, nullptr, nullptr, nullptr);
    ASSERT_NE(nullptr, v);

    json_t* sch = json_loads(chained_ref_root, 0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    int rc = celix_jansson_schema_set_root_schema(v, sch, &errmsg);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, rc) << (errmsg ? errmsg : "");
    free(errmsg);

    /* Valid: 5 is an integer >= 1 */
    json_t* inst = json_loads(R"({"value":5})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_EQ(0, celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr));
    json_decref(inst);

    /* Invalid: 0 is < 1 */
    inst = json_loads(R"({"value":0})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_GT(celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr), 0);
    json_decref(inst);

    EXPECT_EQ(1, ctx.a_calls);
    EXPECT_EQ(1, ctx.b_calls);

    json_decref(sch);
    free_validator(v);
    free_chained_ctx(&ctx);
}

/* ── Chained external ref to a missing fragment → REF_UNRESOLVED ───────── */

TEST(RefTest, ChainedExternalRefMissingFragment) {
    /* B.json loads fine but the requested fragment does not exist; the
     * fragment walk fails and the ref stays unresolved. */
    static const char* a_json = R"({"$ref": "http://example.com/B.json#/definitions/doesNotExist"})";
    static const char* b_json = R"({
        "definitions": {
            "X": { "type": "integer" }
        }
    })";

    chained_loader_ctx ctx;
    init_chained_ctx(&ctx, a_json, b_json, 0);

    auto* v = celix_jansson_schema_validator_create(
        chained_loader, &ctx, nullptr, nullptr, nullptr, nullptr);
    ASSERT_NE(nullptr, v);

    json_t* sch = json_loads(chained_ref_root, 0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    int rc = celix_jansson_schema_set_root_schema(v, sch, &errmsg);
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_REF_UNRESOLVED, rc);
    EXPECT_NE(nullptr, errmsg);
    free(errmsg);

    EXPECT_EQ(1, ctx.a_calls);
    EXPECT_EQ(1, ctx.b_calls);

    json_decref(sch);
    free_validator(v);
    free_chained_ctx(&ctx);
}

/* ── Chained external ref where the second load fails → LOADER ─────────── */

TEST(RefTest, ChainedExternalRefLoaderError) {
    /* The in-place load of B.json fails in resolve_placeholder; the next
     * iteration's Phase A retries it and aborts with the loader's rc.
     * b_calls == 2 proves both the in-place attempt and the retry ran. */
    static const char* a_json = R"({"$ref": "http://example.com/B.json"})";
    static const char* b_json = R"({"type": "integer"})";

    chained_loader_ctx ctx;
    init_chained_ctx(&ctx, a_json, b_json, 1 /* fail_b */);

    auto* v = celix_jansson_schema_validator_create(
        chained_loader, &ctx, nullptr, nullptr, nullptr, nullptr);
    ASSERT_NE(nullptr, v);

    json_t* sch = json_loads(chained_ref_root, 0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    int rc = celix_jansson_schema_set_root_schema(v, sch, &errmsg);
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_LOADER, rc);
    free(errmsg);

    /* One call from the in-place load, one from Phase A's retry */
    EXPECT_EQ(1, ctx.a_calls);
    EXPECT_EQ(2, ctx.b_calls);

    json_decref(sch);
    free_validator(v);
    free_chained_ctx(&ctx);
}

/* ── Baseline: single-level external ref (Phase A only) ────────────────── */

TEST(RefTest, SingleLevelExternalRef) {
    /* Root → A.json only; handled entirely by Phase A of
     * resolve_external_refs, never touching the in-place load path.
     * b_json is never requested, so the loader must never be called for it. */
    static const char* a_json = R"({"type": "integer", "minimum": 1})";
    static const char* b_json = R"({"type": "integer"})";

    chained_loader_ctx ctx;
    init_chained_ctx(&ctx, a_json, b_json, 0);

    auto* v = celix_jansson_schema_validator_create(
        chained_loader, &ctx, nullptr, nullptr, nullptr, nullptr);
    ASSERT_NE(nullptr, v);

    json_t* sch = json_loads(chained_ref_root, 0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    int rc = celix_jansson_schema_set_root_schema(v, sch, &errmsg);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, rc) << (errmsg ? errmsg : "");
    free(errmsg);

    /* Valid: 5 is an integer >= 1 */
    json_t* inst = json_loads(R"({"value":5})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_EQ(0, celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr));
    json_decref(inst);

    /* Invalid: 0 is < 1 */
    inst = json_loads(R"({"value":0})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_GT(celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr), 0);
    json_decref(inst);

    EXPECT_EQ(1, ctx.a_calls);
    EXPECT_EQ(0, ctx.b_calls);

    json_decref(sch);
    free_validator(v);
    free_chained_ctx(&ctx);
}