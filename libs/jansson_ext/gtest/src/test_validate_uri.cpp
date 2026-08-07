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

/* ── validate_uri("#") — equivalent to validate() ──────────────────────── */

TEST(ValidateUriTest, RootSchemaViaHash) {
    static const char* schema = R"({
        "type": "object",
        "properties": {
            "name": { "type": "string" }
        },
        "required": ["name"]
    })";

    auto* v = make_validator();
    ASSERT_NE(nullptr, v);

    json_t* sch = json_loads(schema, 0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    int rc = celix_jansson_schema_set_root_schema(v, sch, &errmsg);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, rc) << (errmsg ? errmsg : "");
    free(errmsg);

    /* Valid: has required "name" property */
    json_t* inst = json_loads(R"({"name":"test"})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_EQ(0, celix_jansson_schema_validate_uri(v, inst, "#", capture_error, nullptr, nullptr));
    json_decref(inst);

    /* Invalid: missing required "name" property */
    inst = json_loads(R"({})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_GT(celix_jansson_schema_validate_uri(v, inst, "#", capture_error, nullptr, nullptr), 0);
    json_decref(inst);

    json_decref(sch);
    free_validator(v);
}

/* ── validate_uri(NULL) — equivalent to validate() ─────────────────────── */

TEST(ValidateUriTest, RootSchemaViaNull) {
    static const char* schema = R"({
        "type": "string",
        "minLength": 3
    })";

    auto* v = make_validator();
    ASSERT_NE(nullptr, v);

    json_t* sch = json_loads(schema, 0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    int rc = celix_jansson_schema_set_root_schema(v, sch, &errmsg);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, rc) << (errmsg ? errmsg : "");
    free(errmsg);

    /* Valid: string long enough */
    json_t* inst = json_loads(R"("abc")", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_EQ(0, celix_jansson_schema_validate_uri(v, inst, nullptr, capture_error, nullptr, nullptr));
    json_decref(inst);

    /* Invalid: string too short */
    inst = json_loads(R"("ab")", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_GT(celix_jansson_schema_validate_uri(v, inst, nullptr, capture_error, nullptr, nullptr), 0);
    json_decref(inst);

    json_decref(sch);
    free_validator(v);
}

/* ── validate_uri("#/definitions/MyType") — subschema validation ──────── */

TEST(ValidateUriTest, DefinitionsSubschema) {
    static const char* schema = R"({
        "definitions": {
            "PositiveInt": {
                "type": "integer",
                "minimum": 1
            },
            "ShortString": {
                "type": "string",
                "maxLength": 5
            }
        },
        "type": "object"
    })";

    auto* v = make_validator();
    ASSERT_NE(nullptr, v);

    json_t* sch = json_loads(schema, 0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    int rc = celix_jansson_schema_set_root_schema(v, sch, &errmsg);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, rc) << (errmsg ? errmsg : "");
    free(errmsg);

    /* Validate against PositiveInt: 5 is valid */
    json_t* inst = json_loads("5", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_EQ(0, celix_jansson_schema_validate_uri(v, inst, "#/definitions/PositiveInt", capture_error, nullptr, nullptr));
    json_decref(inst);

    /* Validate against PositiveInt: 0 is invalid (minimum=1) */
    inst = json_loads("0", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_GT(celix_jansson_schema_validate_uri(v, inst, "#/definitions/PositiveInt", capture_error, nullptr, nullptr), 0);
    json_decref(inst);

    /* Validate against ShortString: "hello" is valid (len=5) */
    inst = json_loads(R"("hello")", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_EQ(0, celix_jansson_schema_validate_uri(v, inst, "#/definitions/ShortString", capture_error, nullptr, nullptr));
    json_decref(inst);

    /* Validate against ShortString: "too long" is invalid (>5) */
    inst = json_loads(R"("too long")", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_GT(celix_jansson_schema_validate_uri(v, inst, "#/definitions/ShortString", capture_error, nullptr, nullptr), 0);
    json_decref(inst);

    /* Validate against ShortString: integer is rejected (type mismatch) */
    inst = json_loads("123", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_GT(celix_jansson_schema_validate_uri(v, inst, "#/definitions/ShortString", capture_error, nullptr, nullptr), 0);
    json_decref(inst);

    json_decref(sch);
    free_validator(v);
}

/* ── validate_uri("#/properties/name") — property subschema ───────────── */

TEST(ValidateUriTest, PropertySubschema) {
    static const char* schema = R"({
        "type": "object",
        "properties": {
            "email": {
                "type": "string",
                "format": "email"
            },
            "age": {
                "type": "integer",
                "minimum": 0,
                "maximum": 150
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

    /* Validate against the "age" property schema: 25 is valid */
    json_t* inst = json_loads("25", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_EQ(0, celix_jansson_schema_validate_uri(v, inst, "#/properties/age", capture_error, nullptr, nullptr));
    json_decref(inst);

    /* Validate against "age": 200 is invalid (>150) */
    inst = json_loads("200", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_GT(celix_jansson_schema_validate_uri(v, inst, "#/properties/age", capture_error, nullptr, nullptr), 0);
    json_decref(inst);

    /* Validate against "age": string is invalid (type mismatch) */
    inst = json_loads(R"("twenty-five")", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_GT(celix_jansson_schema_validate_uri(v, inst, "#/properties/age", capture_error, nullptr, nullptr), 0);
    json_decref(inst);

    json_decref(sch);
    free_validator(v);
}

/* ── validate_uri with empty string ────────────────────────────────────── */

TEST(ValidateUriTest, EmptyStringUri) {
    static const char* schema = R"({
        "type": "integer",
        "minimum": 10
    })";

    auto* v = make_validator();
    ASSERT_NE(nullptr, v);

    json_t* sch = json_loads(schema, 0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    int rc = celix_jansson_schema_set_root_schema(v, sch, &errmsg);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, rc) << (errmsg ? errmsg : "");
    free(errmsg);

    /* Empty string URI should fallback to root schema */
    json_t* inst = json_loads("15", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_EQ(0, celix_jansson_schema_validate_uri(v, inst, "", capture_error, nullptr, nullptr));
    json_decref(inst);

    inst = json_loads("5", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_GT(celix_jansson_schema_validate_uri(v, inst, "", capture_error, nullptr, nullptr), 0);
    json_decref(inst);

    json_decref(sch);
    free_validator(v);
}

/* ── validate_uri with non-existent fragment ───────────────────────────── */

TEST(ValidateUriTest, NonExistentFragmentFallback) {
    static const char* schema = R"({
        "definitions": {
            "Foo": { "type": "string" }
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

    /* Non-existent fragment "#/definitions/NonExistent" — fallback to root schema
     * (root schema has no constraints, so anything passes) */
    json_t* inst = json_loads("42", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_EQ(0, celix_jansson_schema_validate_uri(v, inst, "#/definitions/NonExistent", capture_error, nullptr, nullptr));
    json_decref(inst);

    json_decref(sch);
    free_validator(v);
}

/* ── validate_uri with fragment of an unloaded external document ────────── */

static int unloaded_fragment_loader(const char* /*uri*/, json_t** /*out*/, void* /*ud*/) {
    return CELIX_JANSSON_SCHEMA_ERROR_LOADER;
}

TEST(ValidateUriTest, UnloadedExternalFragmentFallback) {
    /* Root schema references an external document that the loader fails to
     * load.  The file entry exists (document == NULL).  validate_uri with a
     * fragment of that location drives the on-demand document-fragment
     * resolution, which finds no document and falls back to the root
     * schema's placeholder $ref. */
    static const char* schema = R"({
        "$ref": "http://example.com/schema#/definitions/x"
    })";

    auto* v = celix_jansson_schema_validator_create(
        unloaded_fragment_loader, nullptr,
        celix_jansson_schema_default_format_check, nullptr,
        nullptr, nullptr);
    ASSERT_NE(nullptr, v);

    json_t* sch = json_loads(schema, 0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    int rc = celix_jansson_schema_set_root_schema(v, sch, &errmsg);
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_LOADER, rc);
    free(errmsg);

    /* The external document was never loaded, so fragment resolution finds
     * nothing; the fallback root $ref has no target and reports it. */
    json_t* inst = json_integer(42);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_EQ(1, celix_jansson_schema_validate_uri(v, inst,
        "http://example.com/schema#/definitions/x", capture_error, nullptr, nullptr));
    json_decref(inst);
    ASSERT_EQ(1u, captured_messages.size());
    EXPECT_EQ("unresolved or freed schema-reference", captured_messages[0]);

    json_decref(sch);
    free_validator(v);
}

/* ── validate_uri with $ref-based subschema ────────────────────────────── */

TEST(ValidateUriTest, SubschemaReachedViaRef) {
    static const char* schema = R"({
        "definitions": {
            "EmailStr": {
                "type": "string",
                "minLength": 5
            }
        },
        "properties": {
            "email": { "$ref": "#/definitions/EmailStr" }
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

    /* Validate against EmailStr definition directly */
    json_t* inst = json_loads(R"("hello@test.com")", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_EQ(0, celix_jansson_schema_validate_uri(v, inst, "#/definitions/EmailStr", capture_error, nullptr, nullptr));
    json_decref(inst);

    /* Too short for EmailStr */
    inst = json_loads(R"("hi")", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_GT(celix_jansson_schema_validate_uri(v, inst, "#/definitions/EmailStr", capture_error, nullptr, nullptr), 0);
    json_decref(inst);

    /* Type mismatch for EmailStr */
    inst = json_loads("12345", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_GT(celix_jansson_schema_validate_uri(v, inst, "#/definitions/EmailStr", capture_error, nullptr, nullptr), 0);
    json_decref(inst);

    json_decref(sch);
    free_validator(v);
}

/* ── validate_uri with boolean subschema ───────────────────────────────── */

TEST(ValidateUriTest, BooleanSubschema) {
    static const char* schema = R"({
        "definitions": {
            "AlwaysTrue": true,
            "AlwaysFalse": false
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

    /* true schema — accepts anything */
    json_t* inst = json_loads("42", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_EQ(0, celix_jansson_schema_validate_uri(v, inst, "#/definitions/AlwaysTrue", capture_error, nullptr, nullptr));
    json_decref(inst);

    /* false schema — rejects everything */
    inst = json_loads(R"("anything")", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_GT(celix_jansson_schema_validate_uri(v, inst, "#/definitions/AlwaysFalse", capture_error, nullptr, nullptr), 0);
    json_decref(inst);

    json_decref(sch);
    free_validator(v);
}

/* ── validate_uri with external URI (no fragment) ─────────────────────── */

static int ext_schema_loader(const char* /*uri*/, json_t** out, void* /*ud*/) {
    /* Return a schema for any requested URI */
    *out = json_loads(R"({
        "$id": "http://example.com/number.json",
        "type": "number",
        "minimum": 10
    })", 0, nullptr);
    return *out ? CELIX_JANSSON_SCHEMA_OK : CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
}

TEST(ValidateUriTest, ExternalUriNoFragment) {
    /* Root schema references an external schema, which gets loaded and
     * registered.  Then we validate directly against that external URI
     * with no fragment — exercising the empty-fragment lookup path. */
    static const char* schema = R"({
        "type": "object",
        "properties": {
            "score": { "$ref": "http://example.com/number.json" }
        }
    })";

    auto* v = celix_jansson_schema_validator_create(
        ext_schema_loader, nullptr,
        celix_jansson_schema_default_format_check, nullptr,
        nullptr, nullptr);
    ASSERT_NE(nullptr, v);

    json_t* sch = json_loads(schema, 0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    int rc = celix_jansson_schema_set_root_schema(v, sch, &errmsg);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, rc) << (errmsg ? errmsg : "");
    free(errmsg);

    /* Validate against the external schema's root (no fragment) */
    json_t* inst = json_loads("15", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_EQ(0, celix_jansson_schema_validate_uri(v, inst,
        "http://example.com/number.json", capture_error, nullptr, nullptr));
    json_decref(inst);

    /* Invalid: below minimum=10 */
    inst = json_loads("5", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_GT(celix_jansson_schema_validate_uri(v, inst,
        "http://example.com/number.json", capture_error, nullptr, nullptr), 0);
    json_decref(inst);

    /* Invalid: wrong type */
    inst = json_loads(R"("hello")", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_GT(celix_jansson_schema_validate_uri(v, inst,
        "http://example.com/number.json", capture_error, nullptr, nullptr), 0);
    json_decref(inst);

    json_decref(sch);
    free_validator(v);
}

/* ── validate_uri with external URI + fragment ─────────────────────────── */

TEST(ValidateUriTest, ExternalUriWithFragment) {
    /* Same external schema with a definitions entry */
    static const char* ext_schema_json = R"({
        "$id": "http://example.com/types.json",
        "definitions": {
            "Color": {
                "type": "string",
                "enum": ["red", "green", "blue"]
            }
        }
    })";

    /* Cache the schema for the loader */
    json_t* ext_schema = json_loads(ext_schema_json, 0, nullptr);
    ASSERT_NE(nullptr, ext_schema);

    auto loader = [](const char* /*uri*/, json_t** out, void* ud) -> int {
        json_t* cached = (json_t*)ud;
        *out = json_deep_copy(cached);
        return *out ? CELIX_JANSSON_SCHEMA_OK : CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
    };

    auto* v = celix_jansson_schema_validator_create(
        loader, ext_schema,
        celix_jansson_schema_default_format_check, nullptr,
        nullptr, nullptr);
    ASSERT_NE(nullptr, v);

    static const char* schema = R"({
        "type": "object",
        "properties": {
            "color": { "$ref": "http://example.com/types.json#/definitions/Color" }
        }
    })";

    json_t* sch = json_loads(schema, 0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    int rc = celix_jansson_schema_set_root_schema(v, sch, &errmsg);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, rc) << (errmsg ? errmsg : "");
    free(errmsg);

    /* Validate against the external schema's Color definition */
    json_t* inst = json_loads(R"("red")", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_EQ(0, celix_jansson_schema_validate_uri(v, inst,
        "http://example.com/types.json#/definitions/Color", capture_error, nullptr, nullptr));
    json_decref(inst);

    /* Invalid enum value */
    inst = json_loads(R"("yellow")", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_GT(celix_jansson_schema_validate_uri(v, inst,
        "http://example.com/types.json#/definitions/Color", capture_error, nullptr, nullptr), 0);
    json_decref(inst);

    json_decref(sch);
    json_decref(ext_schema);
    free_validator(v);
}
