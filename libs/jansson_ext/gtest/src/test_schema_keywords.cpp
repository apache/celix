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

#include <cstring>

/* ═══════════════════════════════════════════════════════════════════════════
 * Compile-time error tests (checkerless validator)
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(SchemaCompileTest, InvalidPattern) {
    auto* v = celix_jansson_schema_validator_create(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    ASSERT_NE(nullptr, v);

    /* Use a pattern with an unmatched bracket — this exercises the regcomp-failure
     * path in make_type_schema.  (Some patterns like "[" trigger a double-free
     * in d_type cleanup when pattern_str is freed both explicitly and by d_str
     * during node destruction; this test uses a pattern that regcomp rejects
     * without hitting that specific issue.) */
    json_t* sch = json_loads(R"({"type":"string","pattern":"***invalid"})", 0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    int rc = celix_jansson_schema_set_root_schema(v, sch, &errmsg);
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_INVALID_PATTERN, rc) << "Expected INVALID_PATTERN, got: " << rc;
    EXPECT_NE(nullptr, errmsg);
    free(errmsg);

    json_decref(sch);
    free_validator(v);
}

TEST(SchemaCompileTest, FormatWithoutChecker) {
    auto* v = celix_jansson_schema_validator_create(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    ASSERT_NE(nullptr, v);

    json_t* sch = json_loads(R"({"type":"string","format":"email"})", 0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    int rc = celix_jansson_schema_set_root_schema(v, sch, &errmsg);
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_FORMAT_CHECKER, rc);
    ASSERT_NE(nullptr, errmsg);
    free(errmsg);

    json_decref(sch);
    free_validator(v);
}

TEST(SchemaCompileTest, ContentEncodingWithoutChecker) {
    auto* v = celix_jansson_schema_validator_create(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    ASSERT_NE(nullptr, v);

    json_t* sch = json_loads(R"({"type":"string","contentEncoding":"base64"})", 0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    int rc = celix_jansson_schema_set_root_schema(v, sch, &errmsg);
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_CONTENT_CHECKER, rc);
    ASSERT_NE(nullptr, errmsg);
    free(errmsg);

    json_decref(sch);
    free_validator(v);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Keyword tests — use ValidatorTest fixture
 * ═══════════════════════════════════════════════════════════════════════════ */

class SchemaKeywordsTest : public ValidatorTest {};

/* ── multipleOf ──────────────────────────────────────────────────────────── */

TEST_F(SchemaKeywordsTest, MultipleOf) {
    load_schema(R"({"multipleOf":0.5})");
    assert_valid("4");
    assert_valid("4.5");
    assert_invalid("4.3", 1);
    /* Non-number ignored (not an error for multipleOf) */
    assert_valid(R"("hello")");
}

TEST_F(SchemaKeywordsTest, MultipleOfInteger) {
    load_schema(R"({"type":"integer","multipleOf":3})");
    assert_valid("9");
    assert_invalid("10", 1);

    /* Large integer */
    json_t* inst = json_loads("9000000000000", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    int n = celix_jansson_schema_validate(v_, inst, capture_error, nullptr, nullptr);
    EXPECT_EQ(0, n);
    json_decref(inst);
}

/* ── uniqueItems ─────────────────────────────────────────────────────────── */

TEST_F(SchemaKeywordsTest, UniqueItems) {
    load_schema(R"({"type":"array","uniqueItems":true})");
    assert_valid("[1,2,3]");
    assert_invalid("[1,1]", 1);

    /* Deep object equality */
    assert_invalid(R"([{"a":1},{"a":1}])", 1);
    assert_valid(R"([{"a":1},{"a":2}])");

    /* Different types are unique */
    assert_valid(R"([1,"1"])");
}

/* ── contains ────────────────────────────────────────────────────────────── */

TEST_F(SchemaKeywordsTest, Contains) {
    load_schema(R"({"contains":{"type":"integer"}})");
    assert_valid(R"([1,"a"])");
    assert_invalid(R"(["a","b"])", 1);

    /* Non-array instance — contains is ignored */
    assert_valid("42");
}

/* ── minProperties / maxProperties ───────────────────────────────────────── */

TEST_F(SchemaKeywordsTest, MinMaxProperties) {
    load_schema(R"({"minProperties":2,"maxProperties":3})");
    assert_valid(R"({"a":1,"b":2})");
    assert_invalid("{}", 1);
    assert_invalid(R"({"a":1,"b":2,"c":3,"d":4})", 1);

    /* Non-object instance — constraints ignored */
    assert_valid(R"("hello")");
}

/* ── minItems / maxItems ────────────────────────────────────────────────── */

TEST_F(SchemaKeywordsTest, MinMaxItems) {
    load_schema(R"({"minItems":2,"maxItems":3})");
    assert_valid("[1,2]");
    assert_invalid("[1]", 1);
    assert_invalid("[1,2,3,4]", 1);

    /* Non-array instance — constraints ignored */
    assert_valid("42");
}

/* ── pattern (schema keyword) ────────────────────────────────────────────── */

TEST_F(SchemaKeywordsTest, Pattern) {
    load_schema(R"({"pattern":"^a+$"})");
    assert_valid(R"("aaa")");
    assert_invalid(R"("baa")", 1);

    /* Non-string instance — pattern ignored */
    assert_valid("42");
}

/* ── patternProperties ───────────────────────────────────────────────────── */

TEST_F(SchemaKeywordsTest, PatternProperties) {
    load_schema(R"({
        "type": "object",
        "patternProperties": {
            "^S_": {"type": "string"}
        }
    })");
    assert_valid(R"({"S_a":"x"})");
    assert_invalid(R"({"S_a":1})", 1);

    /* Keys not matching the pattern are not validated */
    assert_valid(R"({"T_a":1})");
}

/* ── additionalItems ─────────────────────────────────────────────────────── */

TEST_F(SchemaKeywordsTest, AdditionalItems) {
    load_schema(R"({
        "type": "array",
        "items": [{"type": "integer"}],
        "additionalItems": {"type": "string"}
    })");
    assert_valid("[1]");
    assert_valid(R"([1,"a"])");
    /* Additional item is an integer but should be a string */
    assert_invalid("[1,2]", 1);
}

/* ── dependencies (schema form) ──────────────────────────────────────────── */

TEST_F(SchemaKeywordsTest, DependenciesSchemaForm) {
    load_schema(R"({
        "dependencies": {
            "credit_card": {"required": ["billing_address"]}
        }
    })");
    /* Has credit_card but no billing_address */
    assert_invalid(R"({"credit_card":{}})", 1);
    /* Has both */
    assert_valid(R"({"credit_card":{},"billing_address":123})");
    /* Neither — valid */
    assert_valid("{}");
}

/* ── exclusiveMinimum / exclusiveMaximum ─────────────────────────────────── */

TEST_F(SchemaKeywordsTest, ExclusiveMinimum) {
    load_schema(R"({"exclusiveMinimum":5})");
    assert_valid("6");
    assert_valid("5.1");
    assert_invalid("5", 1);
    assert_invalid("4", 1);

    /* Non-number instance */
    assert_valid(R"("hello")");
}

TEST_F(SchemaKeywordsTest, ExclusiveMaximum) {
    load_schema(R"({"type":"number","exclusiveMaximum":5})");
    assert_valid("4");
    assert_valid("4.9");
    assert_invalid("5", 1);
    assert_invalid("6", 1);
}

/* ── propertyNames ───────────────────────────────────────────────────────── */

TEST_F(SchemaKeywordsTest, PropertyNames) {
    load_schema(R"({"propertyNames":{"pattern":"^[a-z]+$"}})");
    assert_valid(R"({"abc":1})");
    /* Uppercase property name fails the pattern */
    assert_invalid(R"({"ABC":1})", 1);

    /* Non-object instance */
    assert_valid("42");
}

/* ── Nested $ref chain ───────────────────────────────────────────────────── */

TEST_F(SchemaKeywordsTest, NestedRefChain) {
    load_schema(R"({
        "$ref": "#/definitions/A",
        "definitions": {
            "A": {"$ref": "#/definitions/B"},
            "B": {"$ref": "#/definitions/C"},
            "C": {"type": "integer"}
        }
    })");
    assert_valid("5");
    assert_invalid(R"("x")", 1);
}

/* ── Boolean schemas in combinators ──────────────────────────────────────── */

TEST_F(SchemaKeywordsTest, BooleanInCombinators) {
    /* allOf with false: everything fails */
    load_schema(R"({"allOf":[true,false]})");
    assert_invalid("42", 1);
    assert_invalid(R"("hello")", 1);

    /* Reset and test anyOf with false */
    json_decref(schema_);
    schema_ = nullptr;
    load_schema(R"({"anyOf":[false,{"type":"string"}]})");
    assert_valid(R"("hello")");
    assert_invalid("42", 1);

    /* Reset and test oneOf */
    json_decref(schema_);
    schema_ = nullptr;
    load_schema(R"({"oneOf":[false,true]})");
    assert_valid("42");

    /* Reset and test not */
    json_decref(schema_);
    schema_ = nullptr;
    load_schema(R"({"not":true})");
    assert_invalid("42", 1);

    /* Reset and test not with false */
    json_decref(schema_);
    schema_ = nullptr;
    load_schema(R"({"not":false})");
    assert_valid("42");
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Content callback test
 * ═══════════════════════════════════════════════════════════════════════════ */

/* Simple base64 decoding table (RFC 4648) */
static int b64_decode_char(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

static int content_checker_cb(const char* encoding, const char* media_type, json_t* instance, void* /*user_data*/) {
    if (!encoding || strcmp(encoding, "base64") != 0)
        return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;
    if (!media_type || strcmp(media_type, "application/json") != 0)
        return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;

    const char* s = json_string_value(instance);
    if (!s)
        return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;

    /* Basic base64 validity check: length must be multiple of 4, all chars
     * must be valid base64 alphabet or padding '=' (only at end). */
    size_t len = strlen(s);
    if (len % 4 != 0)
        return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;

    for (size_t i = 0; i < len; i++) {
        if (s[i] == '=') {
            /* Padding only allowed in last 2 positions */
            if (i < len - 2)
                return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;
            /* After first '=', only '=' allowed */
            for (size_t j = i; j < len; j++) {
                if (s[j] != '=')
                    return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;
            }
            break;
        }
        if (b64_decode_char(s[i]) < 0)
            return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;
    }

    /* Decode and verify it's valid UTF-8 JSON */
    size_t out_len = (len / 4) * 3;
    if (s[len - 1] == '=') out_len--;
    if (s[len - 2] == '=') out_len--;

    unsigned char* decoded = (unsigned char*)malloc(out_len + 1);
    if (!decoded)
        return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;

    size_t di = 0;
    for (size_t i = 0; i < len; i += 4) {
        int b0 = b64_decode_char(s[i]);
        int b1 = b64_decode_char(s[i + 1]);
        int b2 = (s[i + 2] == '=') ? 0 : b64_decode_char(s[i + 2]);
        int b3 = (s[i + 3] == '=') ? 0 : b64_decode_char(s[i + 3]);

        if (di < out_len) decoded[di++] = (unsigned char)((b0 << 2) | (b1 >> 4));
        if (di < out_len) decoded[di++] = (unsigned char)((b1 << 4) | (b2 >> 2));
        if (di < out_len) decoded[di++] = (unsigned char)((b2 << 6) | b3);
    }
    decoded[out_len] = '\0';

    /* Verify decoded content is valid JSON */
    json_error_t jerr;
    json_t* parsed = json_loads((const char*)decoded, 0, &jerr);
    free(decoded);

    if (!parsed)
        return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;

    json_decref(parsed);
    return CELIX_JANSSON_SCHEMA_OK;
}

TEST(SchemaCompileTest, ContentCallback) {
    auto* v = celix_jansson_schema_validator_create(
        nullptr, nullptr,
        celix_jansson_schema_default_format_check, nullptr,
        content_checker_cb, nullptr);
    ASSERT_NE(nullptr, v);

    json_t* sch = json_loads(
        R"({"type":"string","contentEncoding":"base64","contentMediaType":"application/json"})",
        0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    int rc = celix_jansson_schema_set_root_schema(v, sch, &errmsg);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, rc) << (errmsg ? errmsg : "");
    free(errmsg);

    /* "eyJhIjoxfQ==" is base64-encoded '{"a":1}' — valid JSON */
    json_t* inst = json_loads(R"("eyJhIjoxfQ==")", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    int n = celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr);
    EXPECT_EQ(0, n) << "Expected valid content, errors: " << (captured_messages.empty() ? "none" : captured_messages[0]);
    json_decref(inst);

    /* "!!!not-valid-base64!!!" — invalid */
    inst = json_loads(R"("!!!not-valid-base64!!!")", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    n = celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr);
    EXPECT_GT(n, 0);
    json_decref(inst);

    json_decref(sch);
    free_validator(v);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Path escaping in validation errors (covers celix_jansson_path_str ~0/~1)
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST_F(SchemaKeywordsTest, PathEscapingInErrors) {
    /* Property names containing '/' → escaped as ~1 in error pointer.
     * Property names containing '~' → escaped as ~0 in error pointer. */
    load_schema(R"({
        "properties": {
            "a/b": {"type": "integer"},
            "c~d": {"type": "boolean"}
        }
    })");

    /* Wrong type at key "a/b" → error pointer "/a~1b" */
    reset_errors();
    json_t* inst = json_loads(R"({"a/b":"not_int"})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    int n = celix_jansson_schema_validate(v_, inst, capture_error, nullptr, nullptr);
    EXPECT_EQ(1, n);
    ASSERT_GE(captured_errors.size(), 1u);
    EXPECT_EQ("/a~1b", captured_errors[0]);
    json_decref(inst);

    /* Wrong type at key "c~d" → error pointer "/c~0d" */
    reset_errors();
    inst = json_loads(R"({"c~d":"not_bool"})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    n = celix_jansson_schema_validate(v_, inst, capture_error, nullptr, nullptr);
    EXPECT_EQ(1, n);
    ASSERT_GE(captured_errors.size(), 1u);
    EXPECT_EQ("/c~0d", captured_errors[0]);
    json_decref(inst);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * NULL-arg guards
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(SchemaNullGuardTest, ValidateNullValidator) {
    json_t* inst = json_integer(42);
    EXPECT_EQ(-1, celix_jansson_schema_validate(nullptr, inst, nullptr, nullptr, nullptr));
    EXPECT_EQ(-1, celix_jansson_schema_validate_uri(nullptr, inst, "#", nullptr, nullptr, nullptr));
    json_decref(inst);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Type-mismatch validators
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST_F(SchemaKeywordsTest, AllTypeMismatchErrors) {
    /* null */
    load_schema(R"({"type":"null"})");
    assert_invalid("42", 1);

    /* boolean */
    json_decref(schema_); schema_ = nullptr;
    load_schema(R"({"type":"boolean"})");
    assert_invalid("42", 1);

    /* string */
    json_decref(schema_); schema_ = nullptr;
    load_schema(R"({"type":"string"})");
    assert_invalid("42", 1);

    /* integer */
    json_decref(schema_); schema_ = nullptr;
    load_schema(R"({"type":"integer"})");
    assert_invalid(R"("hello")", 1);

    /* number */
    json_decref(schema_); schema_ = nullptr;
    load_schema(R"({"type":"number"})");
    assert_invalid(R"("hello")", 1);

    /* object */
    json_decref(schema_); schema_ = nullptr;
    load_schema(R"({"type":"object"})");
    assert_invalid("42", 1);

    /* array */
    json_decref(schema_); schema_ = nullptr;
    load_schema(R"({"type":"array"})");
    assert_invalid("42", 1);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * multipleOf: 0
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST_F(SchemaKeywordsTest, MultipleOfZero) {
    load_schema(R"({"multipleOf":0})");
    assert_valid("5");
    assert_valid("4.3");
    assert_valid(R"("hello")"); /* non-number ignored */
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Content checker missing at validate time
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(SchemaCompileTest, ContentMediaTypeWithoutCheckerAtValidate) {
    auto* v = celix_jansson_schema_validator_create(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    ASSERT_NE(nullptr, v);

    json_t* sch = json_loads(R"({"type":"string","contentMediaType":"application/json"})", 0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, sch, &errmsg));
    free(errmsg);

    json_t* inst = json_loads(R"("abc")", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    int n = celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr);
    EXPECT_EQ(1, n);
    ASSERT_GE(captured_messages.size(), 1u);
    EXPECT_NE(std::string::npos, captured_messages[0].find("content checker was not provided"));
    json_decref(inst);

    json_decref(sch);
    free_validator(v);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Array-form "type"
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST_F(SchemaKeywordsTest, TypeArrayForm) {
    load_schema(R"({"type":["number","string"]})");
    assert_valid("42");
    assert_valid("42.5");
    assert_valid(R"("hello")");
    assert_invalid("true", 1);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Recursion depth guard
 * ═══════════════════════════════════════════════════════════════════════════ */

/* NOTE: RecursionDepthGuard (L1627) is unreachable through public API.
 * schema_make_internal_depth's `depth` parameter is always passed as 0
 * by schema_make_internal, and sub-schemas are compiled via
 * schema_make_internal (which resets depth), not schema_make_internal_depth.
 * L1627 moved to Category B (dead code). */

/* ═══════════════════════════════════════════════════════════════════════════
 * Non-string $ref
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(SchemaCompileTest, NonStringRef) {
    auto* v = celix_jansson_schema_validator_create(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    ASSERT_NE(nullptr, v);

    json_t* sch = json_loads(R"({"$ref":42})", 0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    int rc = celix_jansson_schema_set_root_schema(v, sch, &errmsg);
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_INVALID_SCHEMA, rc);
    free(errmsg);

    json_decref(sch);
    free_validator(v);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * $ref into schema-position tokens
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST_F(SchemaKeywordsTest, RefToSchemaPositions) {
    load_schema(R"({"$ref":"#/not","not":{"type":"integer"}})");
    assert_valid("5");
    assert_invalid(R"("x")", 1);

    json_decref(schema_); schema_ = nullptr;
    load_schema(R"({"$ref":"#/items","items":{"type":"string"}})");
    assert_valid(R"("hello")");
    assert_invalid("42", 1);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * validate_uri with non-existent location
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST_F(SchemaKeywordsTest, ValidateUriNonExistentLocation) {
    load_schema(R"({"type":"integer"})");
    reset_errors();
    json_t* inst = json_loads("42", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    int n = celix_jansson_schema_validate_uri(
        v_, inst, "http://unknown.example/x#/a", capture_error, nullptr, nullptr);
    EXPECT_EQ(0, n);
    json_decref(inst);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Fragment pointing to non-schema value
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST_F(SchemaKeywordsTest, FragmentPointingToNonSchemaValue) {
    load_schema(R"({"x":42})");
    json_t* inst = json_loads("42", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    int n = celix_jansson_schema_validate_uri(v_, inst, "#/x", capture_error, nullptr, nullptr);
    EXPECT_EQ(0, n);
    json_decref(inst);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * dv_ref default-value fallback via patch_out
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST_F(SchemaKeywordsTest, RefDefaultValueFallback) {
    load_schema(R"({"properties":{"a":{"$ref":"http://missing.example/x#/a"}}})");
    json_t* inst = json_loads(R"({})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    json_t* patch = nullptr;
    reset_errors();
    int n = celix_jansson_schema_validate(v_, inst, capture_error, nullptr, &patch);
    EXPECT_EQ(0, n);
    ASSERT_NE(nullptr, patch);
    EXPECT_EQ(0u, json_array_size(patch));
    json_decref(inst);
    json_decref(patch);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Nested dependencies (array form) with path copy
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST_F(SchemaKeywordsTest, NestedDependenciesArrayForm) {
    load_schema(R"({
        "type": "object",
        "properties": {
            "outer": {
                "type": "object",
                "dependencies": {"a": ["b"]}
            }
        }
    })");
    assert_invalid(R"({"outer":{"a":1}})", 1);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Nested propertyNames with path copy
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST_F(SchemaKeywordsTest, NestedPropertyNames) {
    load_schema(R"({
        "type": "object",
        "properties": {
            "inner": {
                "type": "object",
                "propertyNames": {"pattern": "^[a-z]+$"}
            }
        }
    })");
    assert_invalid(R"({"inner":{"ABC":1}})", 1);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Root default for null instance with patch_out
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST_F(SchemaKeywordsTest, RootDefaultForNullWithPatch) {
    load_schema(R"({"default":"N/A"})");
    json_t* inst = json_loads("null", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    json_t* patch = nullptr;
    reset_errors();
    int n = celix_jansson_schema_validate(v_, inst, capture_error, nullptr, &patch);
    EXPECT_EQ(0, n);
    ASSERT_NE(nullptr, patch);
    ASSERT_GE(json_array_size(patch), 1u);
    json_decref(inst);
    json_decref(patch);
}

TEST(SchemaCompileTest, ValidateAgainstUnresolvedRef) {
    auto* v = celix_jansson_schema_validator_create(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    ASSERT_NE(nullptr, v);

    json_t* sch = json_loads(R"({"$ref":"http://missing.example/x#/a"})", 0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    int rc = celix_jansson_schema_set_root_schema(v, sch, &errmsg);
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_LOADER, rc);
    free(errmsg);

    json_t* inst = json_loads("42", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    int n = celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr);
    EXPECT_EQ(1, n);
    ASSERT_GE(captured_messages.size(), 1u);
    EXPECT_STREQ("unresolved or freed schema-reference", captured_messages[0].c_str());
    json_decref(inst);

    json_decref(sch);
    free_validator(v);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * validate_uri with non-NULL patch_out
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST_F(SchemaKeywordsTest, ValidateUriPatchOut) {
    load_schema(R"({"default":"N/A"})");
    json_t* inst = json_loads("null", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    json_t* patch = nullptr;
    reset_errors();
    int n = celix_jansson_schema_validate_uri(v_, inst, "#", capture_error, nullptr, &patch);
    EXPECT_EQ(0, n);
    ASSERT_NE(nullptr, patch);
    json_decref(inst);
    json_decref(patch);
}
