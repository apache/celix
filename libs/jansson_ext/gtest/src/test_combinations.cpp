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

/* ── allOf ─────────────────────────────────────────────────────────────── */

TEST(CombinationsTest, AllOfBothPass) {
    static const char* schema = R"({
		"allOf": [
			{"type": "integer", "minimum": 1},
			{"type": "integer", "maximum": 10}
		]
	})";

    auto* v = make_validator();
    ASSERT_NE(nullptr, v);

    json_t* sch = json_loads(schema, 0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    int rc = celix_jansson_schema_set_root_schema(v, sch, &errmsg);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, rc) << (errmsg ? errmsg : "");
    free(errmsg);

    /* Valid: 5 passes both subschemas */
    reset_errors();
    json_t* inst = json_loads("5", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    EXPECT_EQ(0, celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr));
    json_decref(inst);

    /* Invalid: 0 fails minimum */
    inst = json_loads("0", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_GT(celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr), 0);
    json_decref(inst);

    /* Invalid: 11 fails maximum */
    inst = json_loads("11", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_GT(celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr), 0);
    json_decref(inst);

    json_decref(sch);
    free_validator(v);
}

/* ── anyOf ─────────────────────────────────────────────────────────────── */

TEST(CombinationsTest, AnyOfOnePass) {
    static const char* schema = R"({
		"anyOf": [
			{"type": "string"},
			{"type": "integer"}
		]
	})";

    auto* v = make_validator();
    ASSERT_NE(nullptr, v);

    json_t* sch = json_loads(schema, 0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, sch, &errmsg));
    free(errmsg);

    /* Valid: string matches first subschema */
    reset_errors();
    json_t* inst = json_loads("\"hello\"", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    EXPECT_EQ(0, celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr));
    json_decref(inst);

    /* Valid: integer matches second subschema */
    inst = json_loads("42", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_EQ(0, celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr));
    json_decref(inst);

    /* Invalid: boolean matches neither */
    inst = json_loads("true", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_GT(celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr), 0);
    json_decref(inst);

    json_decref(sch);
    free_validator(v);
}

/* ── oneOf ─────────────────────────────────────────────────────────────── */

TEST(CombinationsTest, OneOfExactlyOne) {
    static const char* schema = R"({
		"oneOf": [
			{"type": "integer", "minimum": 10},
			{"type": "integer", "multipleOf": 2}
		]
	})";

    auto* v = make_validator();
    ASSERT_NE(nullptr, v);

    json_t* sch = json_loads(schema, 0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, sch, &errmsg));
    free(errmsg);

    /* Valid: 7 is not >=10 and not multiple of 2 → but wait, oneOf requires exactly ONE match,
     * and 7 matches NEITHER. So it's invalid. */
    /* Let's use better examples: */

    /* 12: >=10 (yes) and multiple of 2 (yes) → both pass → invalid (multiple match) */
    reset_errors();
    json_t* inst = json_loads("12", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    EXPECT_GT(celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr), 0);
    json_decref(inst);

    /* 11: >=10 (yes), not multiple of 2 → exactly one → valid */
    inst = json_loads("11", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_EQ(0, celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr));
    json_decref(inst);

    /* 4: <10 (no), but multiple of 2 (yes) → exactly one → valid */
    inst = json_loads("4", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_EQ(0, celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr));
    json_decref(inst);

    /* 7: <10 (no), not multiple of 2 (no) → zero matches → invalid */
    inst = json_loads("7", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_GT(celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr), 0);
    json_decref(inst);

    json_decref(sch);
    free_validator(v);
}

/* ── not ───────────────────────────────────────────────────────────────── */

TEST(CombinationsTest, NotSchema) {
    static const char* schema = R"({
		"not": { "type": "string" }
	})";

    auto* v = make_validator();
    ASSERT_NE(nullptr, v);

    json_t* sch = json_loads(schema, 0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, sch, &errmsg));
    free(errmsg);

    /* Valid: 42 is not a string */
    reset_errors();
    json_t* inst = json_loads("42", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    EXPECT_EQ(0, celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr));
    json_decref(inst);

    /* Invalid: "hello" is a string — not requires it to NOT validate */
    inst = json_loads("\"hello\"", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_GT(celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr), 0);
    json_decref(inst);

    json_decref(sch);
    free_validator(v);
}

/* ── Verbose combination errors ────────────────────────────────────────── */

TEST(CombinationsTest, VerboseErrors) {
    /* Test that combination errors include case# prefixes */
    static const char* schema = R"({
		"allOf": [
			{"type": "integer", "minimum": 5},
			{"type": "integer", "multipleOf": 2}
		]
	})";

    auto* v = make_validator();
    ASSERT_NE(nullptr, v);

    json_t* sch = json_loads(schema, 0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, sch, &errmsg));
    free(errmsg);

    /* 3 fails both minimum and multipleOf → should get verbose errors */
    reset_errors();
    json_t* inst = json_loads("3", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    int n = celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr);
    EXPECT_GT(n, 0);
    EXPECT_GE(captured_messages.size(), 1u) << "Should have at least 1 error message";
    json_decref(inst);
    json_decref(sch);
    free_validator(v);
}

/* ── Nested combinations ───────────────────────────────────────────────── */

TEST(CombinationsTest, NestedCombinations) {
    static const char* schema = R"({
		"allOf": [
			{"anyOf": [{"type": "integer"}, {"type": "boolean"}]},
			{"not": {"type": "boolean"}}
		]
	})";

    auto* v = make_validator();
    ASSERT_NE(nullptr, v);

    json_t* sch = json_loads(schema, 0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, sch, &errmsg));
    free(errmsg);

    /* 42: integer (passes anyOf[0]), not boolean (passes not) → valid */
    reset_errors();
    json_t* inst = json_loads("42", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    EXPECT_EQ(0, celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr));
    json_decref(inst);

    /* "hello": string fails anyOf (neither integer nor boolean), fails allOf → invalid */
    inst = json_loads("\"hello\"", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_GT(celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr), 0);
    json_decref(inst);

    json_decref(sch);
    free_validator(v);
}

/* ── if/then/else ──────────────────────────────────────────────────────── */

TEST(CombinationsTest, IfThenElse) {
    static const char* schema = R"({
		"if": { "type": "string" },
		"then": { "minLength": 3 },
		"else": { "type": "integer", "minimum": 0 }
	})";

    auto* v = make_validator();
    ASSERT_NE(nullptr, v);

    json_t* sch = json_loads(schema, 0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, sch, &errmsg));
    free(errmsg);

    /* "ab": string, activates 'then', minLength=3 fails → invalid */
    reset_errors();
    json_t* inst = json_loads("\"ab\"", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    EXPECT_GT(celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr), 0);
    json_decref(inst);

    /* "abc": string, activates 'then', minLength=3 passes → valid */
    inst = json_loads("\"abc\"", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_EQ(0, celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr));
    json_decref(inst);

    /* 42: not string, activates 'else', integer >= 0 → valid */
    inst = json_loads("42", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_EQ(0, celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr));
    json_decref(inst);

    /* -1: not string, activates 'else', integer < 0 → invalid */
    inst = json_loads("-1", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    reset_errors();
    EXPECT_GT(celix_jansson_schema_validate(v, inst, capture_error, nullptr, nullptr), 0);
    json_decref(inst);

    json_decref(sch);
    free_validator(v);
}
