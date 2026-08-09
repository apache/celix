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
#include "celix_json_patch.h"
#include "test_common.h"

/* ── Object property default ──────────────────────────────────────────── */

TEST(DefaultsTest, ObjectDefault) {
    static const char* schema = R"({
		"type": "object",
		"properties": {
			"name": { "type": "string" },
			"age": { "type": "integer", "default": 18 }
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

    /* Validate with missing "age" — a patch should be generated */
    reset_errors();
    json_t* patch = nullptr;
    json_t* inst = json_loads(R"({"name":"Bob"})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    int n = celix_jansson_schema_validate(v, inst, capture_error, nullptr, &patch);
    EXPECT_EQ(0, n);

    /* Check patch */
    ASSERT_NE(nullptr, patch);
    EXPECT_TRUE(json_is_array(patch));
    EXPECT_GE(json_array_size(patch), 1u);

    json_decref(inst);
    json_decref(patch);
    json_decref(sch);
    free_validator(v);
}

/* ── Root default for null instance ───────────────────────────────────── */

TEST(DefaultsTest, RootDefaultForNull) {
    static const char* schema = R"({
		"type": "object",
		"properties": {
			"id":   { "type": "integer" },
			"name": { "type": "string", "default": "unknown" }
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

    /* Validate with empty object — defaults should be generated */
    reset_errors();
    json_t* patch = nullptr;
    json_t* inst = json_loads("{}", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    int n = celix_jansson_schema_validate(v, inst, capture_error, nullptr, &patch);
    EXPECT_EQ(0, n);

    ASSERT_NE(nullptr, patch);
    EXPECT_TRUE(json_is_array(patch));

    json_decref(inst);
    json_decref(patch);
    json_decref(sch);
    free_validator(v);
}

/* ── Boolean schema default ───────────────────────────────────────────── */

TEST(DefaultsTest, BooleanSchemaNoDefault) {
    /* Boolean schemas don't have defaults */
    auto* v = make_validator();
    ASSERT_NE(nullptr, v);

    json_t* sch = json_true();
    char* errmsg = nullptr;
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, sch, &errmsg));
    free(errmsg);

    json_t* patch = nullptr;
    json_t* inst = json_loads("42", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    int n = celix_jansson_schema_validate(v, inst, capture_error, nullptr, &patch);
    EXPECT_EQ(0, n);

    /* No defaults in boolean schema → empty or null patch */
    if (patch) {
        EXPECT_TRUE(json_is_array(patch));
    }

    json_decref(inst);
    json_decref(patch);
    json_decref(sch);
    free_validator(v);
}

/* ── Nested object defaults ───────────────────────────────────────────── */

TEST(DefaultsTest, NestedDefaults) {
    static const char* schema = R"({
		"type": "object",
		"properties": {
			"address": {
				"type": "object",
				"properties": {
					"street": { "type": "string", "default": "Main St" },
					"city":   { "type": "string", "default": "Springfield" }
				}
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

    /* Validate with empty object — nested defaults should be generated */
    reset_errors();
    json_t* patch = nullptr;
    json_t* inst = json_loads("{}", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    int n = celix_jansson_schema_validate(v, inst, capture_error, nullptr, &patch);
    EXPECT_EQ(0, n);

    /* Should get a patch with defaults */
    ASSERT_NE(nullptr, patch);

    json_decref(inst);
    json_decref(patch);
    json_decref(sch);
    free_validator(v);
}

/* ── Default value persistence (issue-25 pattern) ─────────────────────── */

TEST(DefaultsTest, DefaultPersistsOnValid) {
    static const char* schema = R"({
		"type": "object",
		"properties": {
			"name":    { "type": "string" },
			"address": {
				"type": "object",
				"properties": {
					"street": { "type": "string", "default": "Abbey Road" }
				}
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

    /* Instance with name and empty address — street default should be generated */
    reset_errors();
    json_t* patch = nullptr;
    json_t* inst = json_loads(R"({"name":"Hans","age":69,"address":{}})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    int n = celix_jansson_schema_validate(v, inst, capture_error, nullptr, &patch);
    EXPECT_EQ(0, n);

    ASSERT_NE(nullptr, patch);
    EXPECT_TRUE(json_is_array(patch));

    json_decref(inst);
    json_decref(patch);
    json_decref(sch);
    free_validator(v);
}

/* ── Patch application ────────────────────────────────────────────────── */

TEST(DefaultsTest, PatchApply) {
    json_t* original = json_loads("{\"a\":1}", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, original);

    json_t* patch = json_array();
    json_t* add_op = json_object();
    json_object_set_new(add_op, "op", json_string("add"));
    json_object_set_new(add_op, "path", json_string("/b"));
    json_object_set_new(add_op, "value", json_integer(2));
    json_array_append_new(patch, add_op);

    json_t* result = celix_json_patch_apply(original, patch);
    ASSERT_NE(nullptr, result);
    json_t* expected = json_loads("{\"a\":1,\"b\":2}", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, expected);
    EXPECT_TRUE(json_equal(result, expected));
    EXPECT_FALSE(json_equal(original, result));

    json_decref(original);
    json_decref(patch);
    json_decref(result);
    json_decref(expected);
}

TEST(DefaultsTest, PatchApplyNested) {
    json_t* original = json_loads("{}", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, original);

    json_t* patch = json_array();
    json_t* add_op = json_object();
    json_object_set_new(add_op, "op", json_string("add"));
    json_object_set_new(add_op, "path", json_string("/address/street"));
    json_object_set_new(add_op, "value", json_string("Main St"));
    json_array_append_new(patch, add_op);

    json_t* result = celix_json_patch_apply(original, patch);
    ASSERT_NE(nullptr, result);
    json_t* expected = json_loads(R"({"address":{"street":"Main St"}})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, expected);
    EXPECT_TRUE(json_equal(result, expected));

    json_decref(original);
    json_decref(patch);
    json_decref(result);
    json_decref(expected);
}

TEST(DefaultsTest, EndToEndDefaultsWithApply) {
    static const char* schema = R"({
		"type": "object",
		"properties": {
			"name": { "type": "string", "default": "anonymous" },
			"age":  { "type": "integer", "default": 0 }
		}
	})";

    auto* v = make_validator();
    ASSERT_NE(nullptr, v);
    json_t* sch = json_loads(schema, 0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, sch, &errmsg));
    free(errmsg);

    json_t* inst = json_loads("{}", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);

    reset_errors();
    json_t* patch = nullptr;
    int n = celix_jansson_schema_validate(v, inst, capture_error, nullptr, &patch);
    EXPECT_EQ(0, n);

    json_t* filled = celix_json_patch_apply(inst, patch);
    ASSERT_NE(nullptr, filled);

    json_t* expected = json_loads(R"({"name":"anonymous","age":0})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, expected);
    EXPECT_TRUE(json_equal(filled, expected));
    EXPECT_FALSE(json_equal(inst, filled));

    json_decref(inst);
    json_decref(patch);
    json_decref(filled);
    json_decref(expected);
    json_decref(sch);
    free_validator(v);
}

/* ── Default value with object type ───────────────────────────────────── */

TEST(DefaultsTest, ObjectTypeDefault) {
    static const char* schema = R"({
		"type": "object",
		"properties": {
			"config": {
				"type": "object",
				"default": {
					"timeout": 30,
					"retries": 3,
					"endpoint": {
						"host": "localhost",
						"port": 8080
					}
				},
				"properties": {
					"timeout": { "type": "integer" },
					"retries": { "type": "integer" },
					"endpoint": {
						"type": "object",
						"properties": {
							"host": { "type": "string" },
							"port": { "type": "integer" }
						}
					}
				}
			}
		}
	})";

    auto* v = make_validator();
    ASSERT_NE(nullptr, v);

    json_t* sch = json_loads(schema, 0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, sch, &errmsg));
    free(errmsg);

    /* Validate empty object — "config" should get the full default object */
    json_t* inst = json_loads("{}", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);

    reset_errors();
    json_t* patch = nullptr;
    int n = celix_jansson_schema_validate(v, inst, capture_error, nullptr, &patch);
    EXPECT_EQ(0, n);
    ASSERT_NE(nullptr, patch);

    /* Apply patch to get filled document */
    json_t* filled = celix_json_patch_apply(inst, patch);
    ASSERT_NE(nullptr, filled);

    /* filled should have the nested config object */
    json_t* cfg = json_object_get(filled, "config");
    ASSERT_NE(nullptr, cfg);
    EXPECT_TRUE(json_is_object(cfg));

    json_t* timeout = json_object_get(cfg, "timeout");
    ASSERT_NE(nullptr, timeout);
    EXPECT_EQ(30, json_integer_value(timeout));

    json_t* retries = json_object_get(cfg, "retries");
    ASSERT_NE(nullptr, retries);
    EXPECT_EQ(3, json_integer_value(retries));

    json_t* endpoint = json_object_get(cfg, "endpoint");
    ASSERT_NE(nullptr, endpoint);
    EXPECT_TRUE(json_is_object(endpoint));

    json_t* host = json_object_get(endpoint, "host");
    ASSERT_NE(nullptr, host);
    EXPECT_STREQ("localhost", json_string_value(host));

    json_t* port = json_object_get(endpoint, "port");
    ASSERT_NE(nullptr, port);
    EXPECT_EQ(8080, json_integer_value(port));

    /* Original unchanged */
    EXPECT_FALSE(json_equal(inst, filled));

    json_decref(inst);
    json_decref(patch);
    json_decref(filled);
    json_decref(sch);
    free_validator(v);
}

/* ── Object default only partially overridden by instance ──────────────── */
/* Per JSON Schema semantics, the object-level "default" applies when the
 * entire property is MISSING. When the property is present (even partially),
 * only per-property defaults are used. */

TEST(DefaultsTest, ObjectDefaultWhenMissing) {
    static const char* schema = R"({
		"type": "object",
		"properties": {
			"config": {
				"type": "object",
				"default": {
					"timeout": 30,
					"retries": 3
				},
				"properties": {
					"timeout": { "type": "integer" },
					"retries": { "type": "integer" }
				}
			}
		}
	})";

    auto* v = make_validator();
    ASSERT_NE(nullptr, v);

    json_t* sch = json_loads(schema, 0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, sch, &errmsg));
    free(errmsg);

    /* Case 1: config is MISSING — the full object default should be inserted */
    json_t* inst = json_loads(R"({})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);

    reset_errors();
    json_t* patch = nullptr;
    int n = celix_jansson_schema_validate(v, inst, capture_error, nullptr, &patch);
    EXPECT_EQ(0, n);
    ASSERT_NE(nullptr, patch);

    json_t* filled = celix_json_patch_apply(inst, patch);
    ASSERT_NE(nullptr, filled);

    json_t* cfg = json_object_get(filled, "config");
    ASSERT_NE(nullptr, cfg) << "config should be inserted from object default";
    EXPECT_EQ(30, json_integer_value(json_object_get(cfg, "timeout")));
    EXPECT_EQ(3, json_integer_value(json_object_get(cfg, "retries")));

    json_decref(inst);
    json_decref(patch);
    json_decref(filled);

    /* Case 2: config is present but with per-property defaults */
    /* For per-property filling, each property needs its own "default" */
    json_decref(sch);
    celix_jansson_schema_validator_destroy(v);
}

TEST(DefaultsTest, PerPropertyDefaultPartialOverride) {
    /* Each property has its own default — partial fill should work */
    static const char* schema = R"({
		"type": "object",
		"properties": {
			"config": {
				"type": "object",
				"properties": {
					"timeout": { "type": "integer", "default": 30 },
					"retries": { "type": "integer", "default": 3 }
				}
			}
		}
	})";

    auto* v = make_validator();
    ASSERT_NE(nullptr, v);

    json_t* sch = json_loads(schema, 0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, sch, &errmsg));
    free(errmsg);

    /* Instance provides config.timeout but not config.retries */
    json_t* inst = json_loads(R"({"config":{"timeout":60}})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);

    reset_errors();
    json_t* patch = nullptr;
    int n = celix_jansson_schema_validate(v, inst, capture_error, nullptr, &patch);
    EXPECT_EQ(0, n);
    ASSERT_NE(nullptr, patch);

    json_t* filled = celix_json_patch_apply(inst, patch);
    ASSERT_NE(nullptr, filled);

    json_t* cfg = json_object_get(filled, "config");
    ASSERT_NE(nullptr, cfg);

    /* timeout should be 60 (from instance), retries should be 3 (from per-property default) */
    EXPECT_EQ(60, json_integer_value(json_object_get(cfg, "timeout")));

    json_t* rv = json_object_get(cfg, "retries");
    ASSERT_NE(nullptr, rv) << "retries should be filled from per-property default";
    EXPECT_EQ(3, json_integer_value(rv));

    json_decref(inst);
    json_decref(patch);
    json_decref(filled);
    json_decref(sch);
    free_validator(v);
}

/* ── Array of objects with defaults ────────────────────────────────────── */

TEST(DefaultsTest, ArrayOfObjectsDefault) {
    static const char* schema = R"({
		"type": "object",
		"properties": {
			"items": {
				"type": "array",
				"default": [],
				"items": {
					"type": "object",
					"properties": {
						"id":   { "type": "integer" },
						"name": { "type": "string", "default": "untitled" }
					}
				}
			}
		}
	})";

    auto* v = make_validator();
    ASSERT_NE(nullptr, v);

    json_t* sch = json_loads(schema, 0, nullptr);
    ASSERT_NE(nullptr, sch);

    char* errmsg = nullptr;
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, sch, &errmsg));
    free(errmsg);

    /* Instance with nested object missing name */
    json_t* inst = json_loads(R"({"items":[{"id":1}]})", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);

    reset_errors();
    json_t* patch = nullptr;
    int n = celix_jansson_schema_validate(v, inst, capture_error, nullptr, &patch);
    EXPECT_EQ(0, n);
    ASSERT_NE(nullptr, patch);

    json_t* filled = celix_json_patch_apply(inst, patch);
    ASSERT_NE(nullptr, filled);

    json_t* arr = json_object_get(filled, "items");
    ASSERT_NE(nullptr, arr);
    EXPECT_EQ(1, json_array_size(arr));

    json_t* elem0 = json_array_get(arr, 0);
    ASSERT_NE(nullptr, elem0);
    EXPECT_EQ(1, json_integer_value(json_object_get(elem0, "id")));
    EXPECT_STREQ("untitled", json_string_value(json_object_get(elem0, "name")));

    json_decref(inst);
    json_decref(patch);
    json_decref(filled);
    json_decref(sch);
    free_validator(v);
}

/* ── RFC 6901 escaping in default patch paths ─────────────────────────── */

TEST(DefaultsTest, DefaultPatchPathEscaping) {
    /* Keys containing '/' and '~' must be escaped (~1/~0) in the JSON Patch
     * path per RFC 6901. The patch path is built via the path API, so the
     * paths below are the escaped forms. */
    static const char* schema = R"({
		"type": "object",
		"properties": {
			"a/b": { "type": "string", "default": "x" },
			"a~b": { "type": "string", "default": "y" }
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

    /* Validate with empty object — both defaults generate a patch op */
    reset_errors();
    json_t* patch = nullptr;
    json_t* inst = json_loads("{}", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, inst);
    int n = celix_jansson_schema_validate(v, inst, capture_error, nullptr, &patch);
    EXPECT_EQ(0, n);

    ASSERT_NE(nullptr, patch);
    ASSERT_TRUE(json_is_array(patch));
    ASSERT_EQ(2u, json_array_size(patch));

    bool saw_slash_escaped = false;
    bool saw_tilde_escaped = false;
    json_t* op;
    size_t i;
    json_array_foreach(patch, i, op) {
        const char* path = json_string_value(json_object_get(op, "path"));
        ASSERT_NE(nullptr, path);
        if (std::string(path) == "/a~1b")
            saw_slash_escaped = true;
        if (std::string(path) == "/a~0b")
            saw_tilde_escaped = true;
    }
    EXPECT_TRUE(saw_slash_escaped);
    EXPECT_TRUE(saw_tilde_escaped);

    json_decref(inst);
    json_decref(patch);
    json_decref(sch);
    free_validator(v);
}
