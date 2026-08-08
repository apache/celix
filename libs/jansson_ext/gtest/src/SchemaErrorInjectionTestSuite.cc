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
#include "celix_jansson_schema.h"
#include "celix_string_hash_map_ei.h"
#include "jansson_ei.h"
#include "malloc_ei.h"
#include "string_ei.h"

/* celix_schema.h is an internal C header without C++ linkage guards */
extern "C" {
#include "celix_schema.h"
}

CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(celix_jansson_schema_validator_t, celix_jansson_schema_validator_destroy)

/**
 * Error-injection tests for the OOM (out-of-memory) handling paths of
 * celix_jansson_schema.
 *
 * The schema compilation/validation pipelines use several static helpers
 * (schema_make_internal_depth, make_type_schema, obj_node_map_create), so the
 * injected caller is matched with `level` = the number of frames between the
 * wrapped allocator and the exported function that starts the pipeline:
 *   - make_type_schema callocs:  level 2 (make_type_schema <- depth <- set_root_schema)
 *   - schema_make_internal_depth callocs: level 1 (depth <- set_root_schema)
 *   - obj_node_map_create createWithOptions: level 3 (obj_node_map_create <- make_type_schema <- depth <- set_root_schema)
 * Ordinals only distinguish consecutive allocations from the same caller.
 */
class JanssonExtSchemaErrorInjectionTestSuite : public ::testing::Test {
public:
    ~JanssonExtSchemaErrorInjectionTestSuite() noexcept override {
        celix_ei_expect_malloc(nullptr, 0, nullptr);
        celix_ei_expect_realloc(nullptr, 0, nullptr);
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_ei_expect_strdup(nullptr, 0, nullptr);
        celix_ei_expect_json_deep_copy(nullptr, 0, nullptr);
        celix_ei_expect_celix_stringHashMap_createWithOptions(nullptr, 0, nullptr);
    }

protected:
    static celix_jansson_schema_validator_t* makeValidator() {
        return celix_jansson_schema_validator_create(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    }

    static json_t* loadSchema(const char* text) {
        json_error_t err{};
        return json_loads(text, 0, &err);
    }
};

static void countingErrorCb(const char*, json_t*, const char*, void* ud) {
    int* count = static_cast<int*>(ud);
    (*count)++;
}

/* ── path_push ────────────────────────────────────────────────────────── */

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaPathPushReallocFail) {
    //Given realloc is injected to fail in path_push
    celix_jansson_path_t p;
    celix_jansson_path_init(&p);
    celix_ei_expect_realloc((void*)celix_jansson_path_push, 0, nullptr);
    //Then pushing a path token should fail
    EXPECT_EQ(-1, celix_jansson_path_push(&p, "abc"));
    celix_jansson_path_free(&p);
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaPathPushStrdupFail) {
    //Given strdup is injected to fail in path_push
    celix_jansson_path_t p;
    celix_jansson_path_init(&p);
    celix_ei_expect_strdup((void*)celix_jansson_path_push, 0, nullptr);
    //Then pushing a path token should fail
    EXPECT_EQ(-1, celix_jansson_path_push(&p, "abc"));
    celix_jansson_path_free(&p);
}

/* ── Schema compilation (set_root_schema) ─────────────────────────────── */

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaBooleanNodeCallocFail) {
    //Given a validator and calloc is injected to fail for the boolean node
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    celix_ei_expect_calloc((void*)celix_jansson_schema_set_root_schema, 1, nullptr);
    //Then compiling a boolean schema should fail with NOMEM
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, json_true(), nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaNodeCallocFail) {
    //Given a validator and calloc is injected to fail for the type-schema node
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"type\":\"string\"}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_calloc((void*)celix_jansson_schema_set_root_schema, 2, nullptr);
    //Then compiling the schema should fail with NOMEM
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaTypedNodeCallocFail) {
    //Given calloc is injected to fail for the per-type (typed) node
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"type\":\"string\"}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_calloc((void*)celix_jansson_schema_set_root_schema, 2, nullptr, 2);
    //Then compiling the schema should fail with NOMEM
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaNoTypeSlotCallocFail) {
    //Given calloc is injected to fail for the first 7-slot node (schema without "type")
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_calloc((void*)celix_jansson_schema_set_root_schema, 2, nullptr, 2);
    //Then compiling the schema should fail with NOMEM
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaTypeArrayNonString) {
    //Given a "type" array containing a non-string entry (no injection needed)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"type\":[1,\"string\"]}");
    ASSERT_NE(nullptr, schema);
    //Then compiling should skip the non-string entry and succeed
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaPropertiesMapCreateFail) {
    //Given stringHashMap creation is injected to fail for the properties map
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"properties\":{\"a\":{\"type\":\"string\"}}}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_celix_stringHashMap_createWithOptions((void*)celix_jansson_schema_set_root_schema, 3, nullptr);
    //Then compiling the schema should fail with NOMEM
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaDependenciesMapCreateFail) {
    //Given stringHashMap creation is injected to fail for the dependencies map
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"dependencies\":{\"a\":{\"type\":\"string\"}}}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_celix_stringHashMap_createWithOptions((void*)celix_jansson_schema_set_root_schema, 3, nullptr);
    //Then compiling the schema should fail with NOMEM
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaRefUriDeriveReallocFail) {
    //Given realloc is injected to fail in strbuf_append (used by percent_decode during $ref URI
    //derivation). The single-char fragment "#x" makes percent_decode loop exactly once, so the
    //single injected realloc failure is not recovered by a later iteration.
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema(
        "{\"$id\":\"http://example.com/root.json\",\"properties\":{\"p\":{\"$ref\":\"#x\"}}}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_realloc((void*)celix_jansson_strbuf_append, 0, nullptr);
    //Then the $ref URI derivation fails with NOMEM, which is now propagated
    //through the properties subschema compilation (make_type_schema).
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaRefTargetPlaceholderCallocFail) {
    //Given calloc is injected to fail for the unresolved $ref target placeholder
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"$ref\":\"#/nonexistent\"}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_calloc((void*)celix_jansson_schema_set_root_schema, 1, nullptr);
    //Then compiling the schema should fail with NOMEM
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaRefNodeCallocFail) {
    //Given calloc is injected to fail for the $ref node (second allocation: placeholder succeeds)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"$ref\":\"#\"}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_calloc((void*)celix_jansson_schema_set_root_schema, 1, nullptr, 2);
    //Then compiling the schema should fail with NOMEM
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaGetOrCreateFileCallocFail) {
    //Given calloc is injected to fail in get_or_create_file (first call, from root_insert)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"type\":\"string\"}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_calloc((void*)celix_jansson_schema_root_get_or_create_file, 0, nullptr);
    //Then get_or_create_file returns NULL, root_insert reports NOMEM and
    //set_root_schema now propagates it (with an error message)
    char* errmsg = nullptr;
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, &errmsg));
    EXPECT_NE(nullptr, errmsg);
    free(errmsg);
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaGetOrCreateFileMapsFail) {
    //Given stringHashMap creation is injected to fail in get_or_create_file (via static obj_node_map_create)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"type\":\"string\"}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_celix_stringHashMap_createWithOptions((void*)celix_jansson_schema_root_get_or_create_file, 1, nullptr);
    //Then get_or_create_file returns NULL, root_insert reports NOMEM and
    //set_root_schema now propagates it.
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaSetRootSchemaDeepCopyFail) {
    //Given json_deep_copy is injected to fail in set_root_schema
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"type\":\"string\"}");
    ASSERT_NE(nullptr, schema);
    char* errmsg = nullptr;
    celix_ei_expect_json_deep_copy((void*)celix_jansson_schema_set_root_schema, 0, nullptr);
    //Then compiling the schema should fail with NOMEM and set an error message
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, &errmsg));
    EXPECT_NE(nullptr, errmsg);
    free(errmsg);
}

/* ── Validator creation ───────────────────────────────────────────────── */

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaValidatorCreateCallocFail) {
    //Given calloc is injected to fail for the validator struct
    celix_ei_expect_calloc((void*)celix_jansson_schema_validator_create, 0, nullptr);
    //Then creating a validator should fail
    EXPECT_EQ(nullptr, makeValidator());
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaValidatorCreateRootCallocFail) {
    //Given calloc is injected to fail for the root struct (second calloc)
    celix_ei_expect_calloc((void*)celix_jansson_schema_validator_create, 0, nullptr, 2);
    //Then creating a validator should fail
    EXPECT_EQ(nullptr, makeValidator());
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaValidatorCreateFilesMapFail) {
    //Given stringHashMap creation is injected to fail for root->files
    celix_ei_expect_celix_stringHashMap_createWithOptions((void*)celix_jansson_schema_validator_create, 0, nullptr);
    //Then creating a validator should fail
    EXPECT_EQ(nullptr, makeValidator());
}

/* ── Validation ───────────────────────────────────────────────────────── */

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaValidateSinkCallocFail) {
    //Given a validator with a compiled schema
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"type\":\"string\"}");
    ASSERT_NE(nullptr, schema);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
    //And calloc is injected to fail for the user sink
    celix_ei_expect_calloc((void*)celix_jansson_schema_validate, 0, nullptr);
    //Then validating should fail
    EXPECT_EQ(-1, celix_jansson_schema_validate(v, json_true(), nullptr, nullptr, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaValidateUriSinkCallocFail) {
    //Given a validator with a compiled schema
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"type\":\"string\"}");
    ASSERT_NE(nullptr, schema);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
    //And calloc is injected to fail for the user sink
    celix_ei_expect_calloc((void*)celix_jansson_schema_validate_uri, 0, nullptr);
    //Then validating by URI should fail
    EXPECT_EQ(-1, celix_jansson_schema_validate_uri(v, json_true(), "#", nullptr, nullptr, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaErrorListAddReallocFail) {
    //Given a validator with an anyOf schema
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"anyOf\":[{\"type\":\"string\"}]}");
    ASSERT_NE(nullptr, schema);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
    //And realloc is injected to fail in error_list_add (branch errors are silently dropped)
    int errorCount = 0;
    celix_ei_expect_realloc((void*)celix_jansson_error_list_add, 0, nullptr);
    //When validating an integer against the anyOf(string) schema
    json_auto_t* instance = json_integer(42);
    int errs = celix_jansson_schema_validate(v, instance, countingErrorCb, &errorCount, nullptr);
    //Then the branch errors are dropped and only the anyOf error is reported
    EXPECT_EQ(1, errs);
    EXPECT_EQ(1, errorCount);
}

/* ── OOM fixes: unchecked callocs in make_type_schema ─────────────────── */

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaTypeArrayTypedCallocFail) {
    //Given calloc is injected to fail for the per-type node of the "type" array form
    //(1st calloc = the type node, 2nd = the typed node)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"type\":[\"string\"]}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_calloc((void*)celix_jansson_schema_set_root_schema, 2, nullptr, 2);
    //Then compiling the schema should fail with NOMEM instead of crashing
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaRequiredArrayCallocFail) {
    //Given calloc is injected to fail for the required array
    //(1st = type node, 2nd = typed object node, 3rd = required array)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"type\":\"object\",\"required\":[\"a\"]}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_calloc((void*)celix_jansson_schema_set_root_schema, 2, nullptr, 3);
    //Then compiling the schema should fail with NOMEM instead of crashing
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaPatternPropertiesCallocFail) {
    //Given calloc is injected to fail for the patternProperties array
    //(1st = type node, 2nd = typed object node, 3rd = patternProperties array)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"type\":\"object\",\"patternProperties\":{\"^a\":{}}}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_calloc((void*)celix_jansson_schema_set_root_schema, 2, nullptr, 3);
    //Then compiling the schema should fail with NOMEM instead of crashing
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaDefinitionsGetOrCreateFileCallocFail) {
    //Given calloc is injected to fail in get_or_create_file, triggered by the
    //definitions block (the first get_or_create_file call during compilation)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"definitions\":{\"a\":{}}}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_calloc((void*)celix_jansson_schema_root_get_or_create_file, 0, nullptr);
    //Then compiling the schema should fail with NOMEM instead of crashing
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

/* ── OOM fixes: DUPLICATE_URI tolerance and error propagation ─────────── */

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaRootIdEmptyDuplicateIgnored) {
    //Given a schema with an empty top-level $id (registered during make with the
    //"" key) and no injection
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"$id\":\"\",\"type\":\"string\"}");
    ASSERT_NE(nullptr, schema);
    //Then the second registration in set_root_schema returns DUPLICATE_URI,
    //which must be tolerated
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaPropertiesInvalidPatternPropagates) {
    //Given a properties subschema whose pattern fails to compile (regcomp)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"properties\":{\"x\":{\"type\":\"string\",\"pattern\":\"***invalid\"}}}");
    ASSERT_NE(nullptr, schema);
    char* errmsg = nullptr;
    //Then the compile error is propagated from the subschema
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_INVALID_PATTERN,
              celix_jansson_schema_set_root_schema(v, schema, &errmsg));
    EXPECT_NE(nullptr, errmsg);
    free(errmsg);
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaOriginalSchemaDeepCopyFail) {
    //Given json_deep_copy is injected to fail for the second copy (the original
    //schema retention); the first copy succeeds (ordinal 1)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"type\":\"string\"}");
    ASSERT_NE(nullptr, schema);
    char* errmsg = nullptr;
    celix_ei_expect_json_deep_copy((void*)celix_jansson_schema_set_root_schema, 0, nullptr, 2);
    //Then compiling the schema should fail with NOMEM
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, &errmsg));
    EXPECT_NE(nullptr, errmsg);
    free(errmsg);
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaRootIdInsertFileCallocFail) {
    //Given calloc is injected to fail in get_or_create_file, reached through the
    //top-level $id registration in schema_make_internal_depth
    //(calloc <- get_or_create_file <- root_insert <- schema_make_internal_depth)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"$id\":\"http://example.com/x\",\"type\":\"string\"}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_calloc((void*)celix_jansson_schema_root_insert, 1, nullptr);
    //Then compiling the schema should fail with NOMEM (registration propagated)
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

/* ── OOM fixes: remaining unchecked callocs (dependencies/items/not/combos) ── */

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaDependenciesArrayCallocFail) {
    //Given calloc is injected to fail for the dependencies array-form node
    //(1st = type node, 2nd = typed object node, 3rd = the required-list node)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"type\":\"object\",\"dependencies\":{\"a\":[\"x\"]}}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_calloc((void*)celix_jansson_schema_set_root_schema, 2, nullptr, 3);
    //Then compiling the schema should fail with NOMEM instead of crashing
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaDependenciesArrayNamesCallocFail) {
    //Given calloc is injected to fail for the required-names array of a dependencies
    //array-form node (1st = type node, 2nd = typed object node, 3rd = required-list
    //node, 4th = names array)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"type\":\"object\",\"dependencies\":{\"a\":[\"x\"]}}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_calloc((void*)celix_jansson_schema_set_root_schema, 2, nullptr, 4);
    //Then compiling the schema should fail with NOMEM instead of crashing
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaItemsTupleCallocFail) {
    //Given calloc is injected to fail for the tuple-form items array
    //(1st = type node, 2nd = typed array node, 3rd = items array)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"type\":\"array\",\"items\":[{\"type\":\"string\"}]}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_calloc((void*)celix_jansson_schema_set_root_schema, 2, nullptr, 3);
    //Then compiling the schema should fail with NOMEM instead of crashing
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaNotNodeCallocFail) {
    //Given calloc is injected to fail for the "not" node
    //(1st = type node, 2nd = typed object node, 3rd = not node; the subschema's
    //own callocs do not match this caller)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"type\":\"object\",\"not\":{\"type\":\"string\"}}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_calloc((void*)celix_jansson_schema_set_root_schema, 2, nullptr, 3);
    //Then compiling the schema should fail with NOMEM instead of crashing
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaComboNodeCallocFail) {
    //Given calloc is injected to fail for the allOf node
    //(1st = type node, 2nd = typed object node, 3rd = combo node)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"type\":\"object\",\"allOf\":[{\"type\":\"string\"}]}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_calloc((void*)celix_jansson_schema_set_root_schema, 2, nullptr, 3);
    //Then compiling the schema should fail with NOMEM instead of crashing
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaComboItemsCallocFail) {
    //Given calloc is injected to fail for the combo items array
    //(1st = type node, 2nd = typed object node, 3rd = combo node, 4th = items array)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"type\":\"object\",\"allOf\":[{\"type\":\"string\"}]}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_calloc((void*)celix_jansson_schema_set_root_schema, 2, nullptr, 4);
    //Then compiling the schema should fail with NOMEM instead of crashing
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaComboSubschemaFailWithNot) {
    //Given a schema with a "not" node already pushed to the logic vec and an allOf
    //whose subschema fails to compile (no injection needed)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"type\":\"object\",\"not\":{},\"allOf\":[1]}");
    ASSERT_NE(nullptr, schema);
    //Then the allOf compile error is propagated and the not node is released
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_INVALID_SCHEMA,
              celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaComboNodeCallocFailWithNot) {
    //Given calloc is injected to fail for the combo node while the logic vec
    //already holds a "not" node (1st = type node, 2nd = typed object node,
    //3rd = not node, 4th = combo node)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"type\":\"object\",\"not\":{},\"allOf\":[{\"type\":\"string\"}]}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_calloc((void*)celix_jansson_schema_set_root_schema, 2, nullptr, 4);
    //Then compiling the schema should fail with NOMEM and release the not node
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaComboItemsCallocFailWithNot) {
    //Given calloc is injected to fail for the combo items array while the logic
    //vec already holds a "not" node (1st = type node, 2nd = typed object node,
    //3rd = not node, 4th = combo node, 5th = items array)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"type\":\"object\",\"not\":{},\"allOf\":[{\"type\":\"string\"}]}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_calloc((void*)celix_jansson_schema_set_root_schema, 2, nullptr, 5);
    //Then compiling the schema should fail with NOMEM and release the not node
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

/* ── Remaining defensive branches and internal API NULL handling ───────── */

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, VCombCollectingSinkAllocFail) {
    //Given a validator with an allOf schema
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"allOf\":[{\"type\":\"integer\"}]}");
    ASSERT_NE(nullptr, schema);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
    //And calloc is injected to fail for the master collecting sink
    //(calloc <- coll_new <- v_comb <- v_type <- root_validate, so level 3)
    json_auto_t* instance = json_integer(5);
    int errorCount = 0;
    celix_ei_expect_calloc((void*)celix_jansson_schema_root_validate, 3, nullptr, 1);
    //Then validating should report the combination as failed (fail-closed)
    EXPECT_EQ(1, celix_jansson_schema_validate(v, instance, countingErrorCb, &errorCount, nullptr));
    EXPECT_EQ(1, errorCount);

    //And calloc is injected to fail for the first branch collecting sink
    errorCount = 0;
    celix_ei_expect_calloc((void*)celix_jansson_schema_root_validate, 3, nullptr, 2);
    //Then validating should report the combination as failed as well
    EXPECT_EQ(1, celix_jansson_schema_validate(v, instance, countingErrorCb, &errorCount, nullptr));
    EXPECT_EQ(1, errorCount);
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, RootDestroyNullIsNoOp) {
    //Given a NULL root (no injection needed)
    //Then destroying it should be a no-op
    celix_jansson_schema_root_destroy(nullptr);
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, RootValidateNullArgs) {
    //Given a NULL root
    //Then validating should return -1
    EXPECT_EQ(-1, celix_jansson_schema_root_validate(nullptr, "#", nullptr, nullptr));
    //And with a zero-initialized root but NULL context it should also return -1
    //(the ctx check fires before any root member is touched)
    celix_jansson_schema_root_t r = {};
    EXPECT_EQ(-1, celix_jansson_schema_root_validate(&r, "#", nullptr, nullptr));
}

/* ── OOM fixes: $id registration and document resolution ───────────────── */

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaNestedDefinitionsGetOrCreateFileCallocFail) {
    //Given calloc is injected to fail in get_or_create_file, reached through the
    //definitions block of a nested schema with its own $id
    //(exercises the my_base cleanup when id_stored_in_out is false)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema =
        loadSchema("{\"properties\":{\"p\":{\"$id\":\"http://example.com/sub\",\"definitions\":{\"a\":{}}}}}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_calloc((void*)celix_jansson_schema_root_get_or_create_file, 0, nullptr);
    //Then compiling the schema should fail with NOMEM (propagated through properties)
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaNestedIdInsertFileCallocFail) {
    //Given calloc is injected to fail in get_or_create_file, reached through the
    //$id registration of a nested schema (id_stored_in_out is false → my_base
    //must be cleared on failure)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"properties\":{\"p\":{\"$id\":\"http://example.com/sub\",\"type\":\"string\"}}}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_calloc((void*)celix_jansson_schema_root_insert, 1, nullptr);
    //Then compiling the schema should fail with NOMEM (registration propagated)
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

