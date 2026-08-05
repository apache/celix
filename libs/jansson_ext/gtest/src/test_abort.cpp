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

/* Helper: create a validator with abort enabled, load schema, validate, return error count */
static int abort_validate(const char* schema_json, const char* instance_json) {
    auto* v = celix_jansson_schema_validator_create(
        nullptr, nullptr,
        celix_jansson_schema_default_format_check, nullptr,
        nullptr, nullptr);
    if (!v) return -1;
    celix_jansson_schema_validator_set_abort_on_error(v, true);

    json_error_t jerr;
    json_t* sch = json_loads(schema_json, 0, &jerr);
    if (!sch) { free_validator(v); return -1; }

    char* errmsg = nullptr;
    int rc = celix_jansson_schema_set_root_schema(v, sch, &errmsg);
    free(errmsg);
    if (rc != CELIX_JANSSON_SCHEMA_OK) { json_decref(sch); free_validator(v); return -1; }

    json_t* inst = json_loads(instance_json, JSON_DECODE_ANY, &jerr);
    if (!inst) { json_decref(sch); free_validator(v); return -1; }

    reset_errors();
    int n = celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr);
    json_decref(inst);
    json_decref(sch);
    free_validator(v);
    return n;
}

/* ── Test #1: v_type type slot abort ──────────────────────────────────── */

TEST(AbortTest, TypeSlotAbort) {
    /* Type mismatch → abort before checking minLength */
    int n = abort_validate(
        R"({"type":"string","minLength":3})",
        "42");
    EXPECT_EQ(1, n);
    EXPECT_EQ(1u, captured_errors.size());
    EXPECT_NE(std::string::npos, captured_messages[0].find("unexpected instance type"));
}

/* ── Test #2: v_type logic combinator abort ───────────────────────────── */

TEST(AbortTest, LogicCombinatorAbort) {
    /* allOf: first branch (type mismatch) fails → abort before second branch */
    int n = abort_validate(
        R"({"allOf":[{"type":"string"},{"minLength":10}]})",
        "42");
    EXPECT_EQ(1, n);
    /* The combination itself reports 1 error; abort prevents further branches */
}

/* ── Test #3: v_type then branch abort ─────────────────────────────────── */

TEST(AbortTest, ThenBranchAbort) {
    /* if passes (object), then requires name+age → only name failure captured */
    int n = abort_validate(
        R"({"if":{"type":"object"},"then":{"required":["name","age"]}})",
        R"({})");
    EXPECT_EQ(1, n);
}

/* ── Test #4: v_type else branch abort ─────────────────────────────────── */

TEST(AbortTest, ElseBranchAbort) {
    /* if fails (not string), else requires a+b → only "a" missing captured */
    int n = abort_validate(
        R"({"if":{"type":"string"},"else":{"required":["a","b"]}})",
        R"({})");
    EXPECT_EQ(1, n);
}

/* ── Test #5: v_object required loop abort ────────────────────────────── */

TEST(AbortTest, RequiredLoopAbort) {
    /* Missing a,b,c → only "a" reported */
    int n = abort_validate(
        R"({"required":["a","b","c"]})",
        R"({})");
    EXPECT_EQ(1, n);
    EXPECT_NE(std::string::npos, captured_messages[0].find("'a'"));
}

/* ── Test #6: v_object properties loop abort ──────────────────────────── */

TEST(AbortTest, PropertiesLoopAbort) {
    /* x has wrong type → abort; y and z never checked */
    int n = abort_validate(
        R"({"properties":{"x":{"type":"number"},"y":{"type":"string"},"z":{"type":"boolean"}}})",
        R"({"x":"wrong","y":42,"z":"wrong"})");
    EXPECT_EQ(1, n);
    EXPECT_NE(std::string::npos, captured_messages[0].find("unexpected instance type"));
}

/* ── Test #7: v_object propertyNames abort ────────────────────────────── */

TEST(AbortTest, PropertyNamesAbort) {
    /* "a" is too short (minLength=5) → abort; "bb" never checked */
    int n = abort_validate(
        R"({"propertyNames":{"minLength":5}})",
        R"({"a":1,"bb":2})");
    EXPECT_EQ(1, n);
}

/* ── Test #8: v_object patternProperties abort ────────────────────────── */

TEST(AbortTest, PatternPropertiesAbort) {
    /* ^x matches "x1" (wrong type) → abort; ^y pattern never checked */
    int n = abort_validate(
        R"({"patternProperties":{"^x":{"type":"number"},"^y":{"type":"string"}}})",
        R"({"x1":"wrong","y1":42})");
    EXPECT_EQ(1, n);
}

/* ── Test #9: v_object additionalProperties abort ─────────────────────── */

TEST(AbortTest, AdditionalPropertiesAbort) {
    /* "a" is not a number → abort; "b" never checked */
    int n = abort_validate(
        R"({"additionalProperties":{"type":"number"}})",
        R"({"a":"wrong","b":"wrong"})");
    EXPECT_EQ(1, n);
    EXPECT_NE(std::string::npos, captured_messages[0].find("'a'"));
}

/* ── Test #10: v_object dependencies loop abort ───────────────────────── */

TEST(AbortTest, DependenciesLoopAbort) {
    /* a present → x missing → abort; b/y never checked */
    int n = abort_validate(
        R"({"dependencies":{"a":["x"],"b":["y"]}})",
        R"({"a":1,"b":2})");
    EXPECT_EQ(1, n);
}

/* ── Test #11: v_array items_schema (uniform) abort ───────────────────── */

TEST(AbortTest, ItemsUniformAbort) {
    /* First element "wrong" is not number → abort */
    int n = abort_validate(
        R"({"items":{"type":"number"}})",
        R"(["wrong",42,"also wrong"])");
    EXPECT_EQ(1, n);
}

/* ── Test #12: v_array tuple items abort ──────────────────────────────── */

TEST(AbortTest, TupleItemsAbort) {
    /* First position fails type check → abort */
    int n = abort_validate(
        R"({"items":[{"type":"number"},{"type":"string"}]})",
        R"(["wrong","ok"])");
    EXPECT_EQ(1, n);
}

/* ── Test #13: v_array additionalItems abort ──────────────────────────── */

TEST(AbortTest, AdditionalItemsAbort) {
    /* First extra element 100 is not string → abort */
    int n = abort_validate(
        R"({"items":[{"type":"number"}],"additionalItems":{"type":"string"}})",
        "[42,100,200]");
    EXPECT_EQ(1, n);
}

/* ── Test #14: nested object abort propagation ────────────────────────── */

TEST(AbortTest, NestedObjectPropagation) {
    /* Inner object missing "a" → abort propagates up */
    int n = abort_validate(
        R"({"properties":{"inner":{"type":"object","properties":{"a":{"type":"number","minimum":1}},"required":["a"]}}})",
        R"({"inner":{}})");
    EXPECT_EQ(1, n);
}

/* ── Test #15: internal probe (not) does NOT trigger abort ────────────── */

TEST(AbortTest, InternalProbeNotDoesNotAbort) {
    /* not with first_sink_t should not trigger abort; the real abort comes
     * from x property failing type check */
    int n = abort_validate(
        R"({"not":{"type":"string"},"properties":{"x":{"type":"number"}}})",
        R"({"x":"wrong"})");
    EXPECT_EQ(1, n);
    EXPECT_NE(std::string::npos, captured_messages[0].find("unexpected instance type"));
}

/* ── Test #16: default behavior (abort disabled) ──────────────────────── */

TEST(AbortTest, DefaultBehaviorCollectsAll) {
    /* Without abort, both property errors are reported */
    auto* v = make_validator();
    ASSERT_NE(nullptr, v);
    /* Explicitly NOT calling set_abort_on_error — default is false */

    json_t* sch = json_loads(R"({"properties":{"a":{"type":"number"},"b":{"type":"number"}}})", 0, nullptr);
    ASSERT_NE(nullptr, sch);
    char* errmsg = nullptr;
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, sch, &errmsg));
    free(errmsg);

    json_t* inst = json_loads(R"({"a":"x","b":"y"})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    int n = celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr);
    /* Both errors should be reported */
    EXPECT_GT(n, 1);
    json_decref(inst);
    json_decref(sch);
    free_validator(v);
}

/* ── Test #17: validate_uri also supports abort ───────────────────────── */

TEST(AbortTest, ValidateUriAbort) {
    auto* v = celix_jansson_schema_validator_create(
        nullptr, nullptr,
        celix_jansson_schema_default_format_check, nullptr,
        nullptr, nullptr);
    ASSERT_NE(nullptr, v);
    celix_jansson_schema_validator_set_abort_on_error(v, true);

    json_t* sch = json_loads(
        R"({"definitions":{"T":{"type":"integer","minimum":10}}})", 0, nullptr);
    ASSERT_NE(nullptr, sch);
    char* errmsg = nullptr;
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, sch, &errmsg));
    free(errmsg);

    json_t* inst = json_loads(R"("wrong")", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    int n = celix_jansson_schema_validate_uri(v, inst, "#/definitions/T", capture_error, nullptr, nullptr);
    EXPECT_EQ(1, n);
    json_decref(inst);
    json_decref(sch);
    free_validator(v);
}
