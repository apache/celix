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