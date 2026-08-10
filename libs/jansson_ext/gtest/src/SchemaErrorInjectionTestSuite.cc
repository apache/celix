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

#include <string>
#include <vector>

#include "asprintf_ei.h"
#include "celix_cleanup.h"
#include "celix_jansson_pointer.h"
#include "celix_json_patch.h"
#include "celix_jansson_schema.h"
#include "celix_jansson_uri.h"
#include "celix_string_hash_map_ei.h"
#include "celix_util.h"
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
        celix_ei_expect_json_array(nullptr, 0, nullptr);
        celix_ei_expect_json_array_append_new(nullptr, 0, 0);
        celix_ei_expect_json_string(nullptr, 0, nullptr);
        celix_ei_expect_celix_stringHashMap_createWithOptions(nullptr, 0, nullptr);
        celix_ei_expect_celix_stringHashMap_put(nullptr, 0, CELIX_ENOMEM);
        celix_ei_expect_asprintf(nullptr, 0, 0);
        celix_ei_expect_vasprintf(nullptr, 0, 0);
    }

protected:
    static celix_jansson_schema_validator_t* makeValidator() {
        return celix_jansson_schema_validator_create(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    }

    static celix_jansson_schema_validator_t* makeValidatorWithLoader() {
        return celix_jansson_schema_validator_create(testLoader, nullptr, nullptr, nullptr, nullptr, nullptr);
    }

    static json_t* loadSchema(const char* text) {
        json_error_t err{};
        return json_loads(text, 0, &err);
    }

    /* Loader used by the external-ref tests. The returned document registers
     * both a definitions entry (resolved directly after loading) and a
     * properties entry (only reachable via the document-fragment walk). */
    static int testLoader(const char* location, json_t** out, void* ud) {
        (void)ud;
        if (strcmp(location, "http://example.com/doc") != 0)
            return CELIX_JANSSON_SCHEMA_ERROR_LOADER;
        *out = json_loads(
            "{\"definitions\":{\"x\":{\"type\":\"string\"}},\"properties\":{\"b\":{\"type\":\"string\"}}}",
            0,
            nullptr);
        return *out ? CELIX_JANSSON_SCHEMA_OK : CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
    }
};

static void countingErrorCb(const char*, json_t*, const char*, void* ud) {
    int* count = static_cast<int*>(ud);
    (*count)++;
}

static void captureMessageCb(const char*, json_t*, const char* msg, void* ud) {
    std::string* out = static_cast<std::string*>(ud);
    *out = msg ? msg : "";
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
    //derivation; realloc <- strbuf_append <- appendc <- percent_decode, so level 0). The
    //single-char fragment "#x" makes percent_decode loop exactly once, so the single injected
    //realloc failure is not recovered by a later iteration.
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

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaRefLocationOomFail) {
    //Given strdup is injected to fail inside uri_location during $ref compilation.
    //The schema has no $id, so the first uri_location call is for the $ref URI;
    //today its NULL return reaches get_or_create_file and crashes the hashmap.
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"$ref\":\"urn:foo\"}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_strdup((void*)celix_jansson_uri_location, 0, nullptr);
    //Then compiling the schema should fail with NOMEM instead of crashing
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaRootInsertLocationOomFail) {
    //Given strdup is injected to fail inside uri_location during root_insert.
    //The first uri_location call in this flow is root_insert's; it also
    //exercises the root->root cleanup path of set_root_schema.
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"type\":\"string\"}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_strdup((void*)celix_jansson_uri_location, 0, nullptr);
    //Then compiling the schema should fail with NOMEM instead of crashing
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

/* ── Remaining OOM branches of schema_make_internal_depth and the
 *    document-fragment resolver (reached via error injection) ──────────── */

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaDefinitionsBaseLocOomFail) {
    //Given realloc is injected to fail in strbuf_append, hit by the definitions
    //block of a nested schema with its own $id (realloc <- strbuf_append <-
    //strbuf_appends <- uri_location, so level 0). The nested schema is compiled
    //with eff_base_out == NULL (schema_make_internal), so base_loc comes from
    //uri_location(my_base) and the failure must also clear my_base.
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema =
        loadSchema("{\"properties\":{\"p\":{\"$id\":\"http://x/nested\",\"definitions\":{\"x\":{\"type\":\"string\"}}}}}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_realloc((void*)celix_jansson_strbuf_append, 0, nullptr);
    //Then compiling the schema should fail with NOMEM
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaRefNoBaseUriInitOomFail) {
    //Given malloc is injected to fail inside uri_update while parsing the $ref
    //URI with no effective base (root schema without $id). The first
    //uri_update allocation is the location buffer of the ref URI.
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"$ref\":\"x\"}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_malloc((void*)celix_jansson_uri_update, 0, nullptr);
    //Then compiling the schema should fail with NOMEM instead of crashing
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaRefPlaceholderToStringOomFail) {
    //Given strdup is injected to fail inside uri_location, hit by the second
    //empty-location strdup: the placeholder node's uri_to_string during $ref
    //compilation (1st = rloc, 2nd = uri_to_string's location).
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"$ref\":\"#/definitions/missing\"}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_strdup((void*)celix_jansson_uri_location, 0, nullptr, 2);
    //Then compiling the schema should fail with NOMEM instead of crashing
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaRefNodeToStringOomFail) {
    //Given strdup is injected to fail inside uri_location, hit by the second
    //empty-location strdup: the $ref node's uri_to_string when the target is
    //already registered (1st = rloc, 2nd = uri_to_string's location).
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema =
        loadSchema("{\"definitions\":{\"x\":{\"type\":\"string\"}},\"$ref\":\"#/definitions/x\"}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_strdup((void*)celix_jansson_uri_location, 0, nullptr, 2);
    //Then compiling the schema should fail with NOMEM instead of crashing
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaDocFragmentCurBaseUriOomFail) {
    //Given strdup is injected to fail in uri_update, hit by the document-fragment
    //walk's cur_base init in resolve_document_fragment (1st = root $ref path,
    //2nd = retrieval URI path, 3rd = cur_base path). The walk returns NULL and
    //the ref is resolved on the next resolve_external_refs iteration.
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidatorWithLoader();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"$ref\":\"http://example.com/doc#/properties/b\"}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_strdup((void*)celix_jansson_uri_update, 0, nullptr, 3);
    //Then the schema still compiles (the failed walk self-heals on retry)
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaDocFragmentNoBaseUriOomFail) {
    //Given strdup is injected to fail inside compile_external_document, keeping
    //the external file's base_uri NULL. The 2nd frame-2 strdup matches (1st =
    //root_insert's fragment strdup, 2nd = strdup(location) in
    //compile_external_document). And malloc is injected to fail in uri_update,
    //hit by the walk's cur_base init taken from the location instead of
    //base_uri (10th uri_update malloc: 3 for the root $id derive, 3 for the
    //$ref URI, 3 for the retrieval URI, then cur_base).
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidatorWithLoader();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema(
        "{\"$id\":\"http://x/root\",\"properties\":{\"p\":{\"$ref\":\"http://example.com/doc#/properties/b\"}}}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_strdup((void*)celix_jansson_schema_set_root_schema, 2, nullptr, 2);
    celix_ei_expect_malloc((void*)celix_jansson_uri_update, 0, nullptr, 10);
    //Then the schema still compiles (the failed walk self-heals on retry)
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaDocFragmentFullUriOomFail) {
    //Given strdup is injected to fail in uri_update, hit by the walk's full_uri
    //init in resolve_document_fragment (1st = root $ref path, 2nd = retrieval
    //URI path, 3rd = cur_base path, 4th = full_uri path).
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidatorWithLoader();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"$ref\":\"http://example.com/doc#/properties/b\"}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_strdup((void*)celix_jansson_uri_update, 0, nullptr, 4);
    //Then the schema still compiles (the failed walk self-heals on retry)
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaDocFragmentPointerPushOomFail) {
    //Given strdup is injected to fail in celix_json_pointer_push, hit by the
    //walk's fragment re-attach loop. The push is also used internally by
    //pointer_init (1st-2nd = $ref URI pointer_init, 3rd-4th = walk pointer_init,
    //5th = re-attach push). The walk returns NULL and the ref is resolved on
    //the next resolve_external_refs iteration.
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema =
        loadSchema("{\"properties\":{\"b\":{\"type\":\"string\"}},\"$ref\":\"#/properties/b\"}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_strdup((void*)celix_json_pointer_push, 0, nullptr, 5);
    //Then the schema still compiles (the failed walk self-heals on retry)
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaCompileExternalUriInitOomFail) {
    //Given strdup is injected to fail in uri_update, hit by the retrieval-URI
    //init in compile_external_document (1st = root $ref path, 2nd = retrieval
    //URI path). The loader-based document compile fails and propagates NOMEM.
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidatorWithLoader();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"$ref\":\"http://example.com/doc#/definitions/x\"}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_strdup((void*)celix_jansson_uri_update, 0, nullptr, 2);
    //Then compiling the schema should fail with NOMEM instead of crashing
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaResolveExternalPhaseAvecOomFail) {
    //Given realloc is injected to fail in celix_jansson_vec_push, hit by the
    //Phase A location-key snapshot of resolve_external_refs (1st vec_push).
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidatorWithLoader();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"$ref\":\"http://example.com/doc#/definitions/x\"}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_realloc((void*)celix_jansson_vec_push, 0, nullptr);
    //Then compiling the schema should fail with NOMEM instead of crashing
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaResolveExternalPhaseBPairAllocOomFail) {
    //Given malloc is injected to fail for the Phase B (location, fragment) pair
    //allocation of resolve_external_refs (1st frame-1 malloc of set_root_schema).
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidatorWithLoader();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"$ref\":\"http://example.com/doc#/definitions/x\"}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_malloc((void*)celix_jansson_schema_set_root_schema, 1, nullptr);
    //Then the schema still compiles (the failed snapshot self-heals on retry)
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaResolveExternalPhaseBPairVecPushOomFail) {
    //Given realloc is injected to fail in celix_jansson_vec_push, hit by the
    //Phase B pairs-vec push (1st realloc = Phase A locs-vec push, 2nd = pairs
    //push, which triggers the pair cleanup).
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidatorWithLoader();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"$ref\":\"http://example.com/doc#/definitions/x\"}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_realloc((void*)celix_jansson_vec_push, 0, nullptr, 2);
    //Then the schema still compiles (the failed snapshot self-heals on retry)
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaResolveExternalRegisterRootOomFail) {
    //Given realloc is injected to fail inside root_insert's uri_location
    //while registering the external document at its retrieval URI (realloc <-
    //strbuf_append <- strbuf_appends <- uri_location <- root_insert, so level 3)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidatorWithLoader();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"$ref\":\"http://example.com/doc\"}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_realloc((void*)celix_jansson_schema_root_insert, 3, nullptr);
    //Then compiling fails with NOMEM and the document/node refs are released
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaResolveExternalFragmentRegisterOomFail) {
    //Given realloc is injected to fail inside root_insert's uri_location
    //while the document-fragment walk registers the resolved node (ordinal 2:
    //the external document itself is registered first)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidatorWithLoader();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"$ref\":\"http://example.com/doc#/definitions/x\"}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_realloc((void*)celix_jansson_schema_root_insert, 3, nullptr, 2);
    //Then the fragment-walk registration fails and the schema still compiles
    //(the placeholder resolution retries on the next iteration)
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}


TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaResolveExternalPhaseAKeyStrdupOomFail) {
    //Given strdup is injected to fail for the Phase A location-key snapshot of
    //resolve_external_refs. The 3rd frame-1 strdup matches (1st = the
    //set_root_schema rloc-block uri_location strdup, which is tolerated, 2nd =
    //first file key, 3rd = second file key — failing it exercises the snapshot
    //cleanup loop over the already-collected keys).
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidatorWithLoader();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"$ref\":\"http://example.com/doc#/definitions/x\"}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_strdup((void*)celix_jansson_schema_set_root_schema, 1, nullptr, 3);
    //Then compiling the schema should fail with NOMEM instead of crashing
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

/* ── Runtime validator OOM: first-sink callocs (P0) ───────────────────── */

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaValidateAdditionalPropertiesSinkOomFail) {
    //Given a validator with an object schema that rejects additional properties
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"type\":\"object\",\"additionalProperties\":{\"type\":\"string\"}}");
    ASSERT_NE(nullptr, schema);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
    //And calloc is injected to fail for the additionalProperties first-sink
    //(calloc <- first_sink_new <- v_object <- v_type <- root_validate, so level 3)
    json_auto_t* instance = loadSchema("{\"a\":\"x\"}");
    int errorCount = 0;
    celix_ei_expect_calloc((void*)celix_jansson_schema_root_validate, 3, nullptr);
    //Then validating fails closed with an out-of-memory error instead of crashing
    EXPECT_EQ(1, celix_jansson_schema_validate(v, instance, countingErrorCb, &errorCount, nullptr));
    EXPECT_EQ(1, errorCount);
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaValidateContainsSinkOomFail) {
    //Given a validator with a contains schema
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"type\":\"array\",\"contains\":{\"type\":\"string\"}}");
    ASSERT_NE(nullptr, schema);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
    //And calloc is injected to fail for the contains first-sink
    //(calloc <- first_sink_new <- v_array <- v_type <- root_validate, so level 3)
    json_auto_t* instance = json_pack("[s]", "x");
    int errorCount = 0;
    celix_ei_expect_calloc((void*)celix_jansson_schema_root_validate, 3, nullptr);
    //Then validating fails closed with an out-of-memory error instead of crashing
    EXPECT_EQ(1, celix_jansson_schema_validate(v, instance, countingErrorCb, &errorCount, nullptr));
    EXPECT_EQ(1, errorCount);
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaValidateNotSinkOomFail) {
    //Given a validator with a not schema
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"not\":{\"type\":\"string\"}}");
    ASSERT_NE(nullptr, schema);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
    //And calloc is injected to fail for the not first-sink
    //(calloc <- first_sink_new <- v_not <- v_type <- root_validate, so level 3)
    json_auto_t* instance = json_integer(42);
    int errorCount = 0;
    celix_ei_expect_calloc((void*)celix_jansson_schema_root_validate, 3, nullptr);
    //Then validating fails closed with an out-of-memory error instead of crashing
    EXPECT_EQ(1, celix_jansson_schema_validate(v, instance, countingErrorCb, &errorCount, nullptr));
    EXPECT_EQ(1, errorCount);
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaValidateIfSinkOomFail) {
    //Given a validator with an if/then schema
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"if\":{\"type\":\"string\"},\"then\":{\"type\":\"integer\"}}");
    ASSERT_NE(nullptr, schema);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
    //And calloc is injected to fail for the if-condition first-sink
    //(calloc <- first_sink_new <- v_type <- root_validate, so level 2)
    json_auto_t* instance = json_integer(42);
    int errorCount = 0;
    celix_ei_expect_calloc((void*)celix_jansson_schema_root_validate, 2, nullptr);
    //Then validating fails closed with an out-of-memory error instead of crashing
    EXPECT_EQ(1, celix_jansson_schema_validate(v, instance, countingErrorCb, &errorCount, nullptr));
    EXPECT_EQ(1, errorCount);
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaValidatePropertyNamesJsonStringOomFail) {
    //Given a validator with a propertyNames schema
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"type\":\"object\",\"propertyNames\":{\"type\":\"string\"}}");
    ASSERT_NE(nullptr, schema);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
    //And json_string is injected to fail for the property-name wrapper
    //(json_string <- v_object <- v_type <- root_validate, so level 2)
    json_auto_t* instance = loadSchema("{\"a\":1}");
    int errorCount = 0;
    celix_ei_expect_json_string((void*)celix_jansson_schema_root_validate, 2, nullptr);
    //Then validating fails closed with an out-of-memory error instead of
    //crashing on json_typeof(NULL)
    EXPECT_EQ(1, celix_jansson_schema_validate(v, instance, countingErrorCb, &errorCount, nullptr));
    EXPECT_EQ(1, errorCount);
}

/* ── Runtime validator OOM: patch array / error list / path building ──── */

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaValidatePatchArrayOomFail) {
    //Given a validator with a compiled schema
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"type\":\"string\"}");
    ASSERT_NE(nullptr, schema);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
    //And json_array is injected to fail for the patch
    json_auto_t* instance = json_string("x");
    celix_ei_expect_json_array((void*)celix_jansson_schema_validate, 0, nullptr);
    //Then validating should fail instead of running with a broken patch
    EXPECT_EQ(-1, celix_jansson_schema_validate(v, instance, nullptr, nullptr, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaValidateUriPatchArrayOomFail) {
    //Given a validator with a compiled schema
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"type\":\"string\"}");
    ASSERT_NE(nullptr, schema);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
    //And json_array is injected to fail for the patch
    json_auto_t* instance = json_string("x");
    celix_ei_expect_json_array((void*)celix_jansson_schema_validate_uri, 0, nullptr);
    //Then validating by URI should fail instead of running with a broken patch
    EXPECT_EQ(-1, celix_jansson_schema_validate_uri(v, instance, "#", nullptr, nullptr, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaValidateErrorListStrdupOomFail) {
    //Given a validator with an anyOf schema
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"anyOf\":[{\"type\":\"string\"}]}");
    ASSERT_NE(nullptr, schema);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
    //And strdup is injected to fail for the error entry's path (message survives)
    int errorCount = 0;
    celix_ei_expect_strdup((void*)celix_jansson_error_list_add, 0, nullptr);
    //When validating an integer against the anyOf(string) schema
    json_auto_t* instance = json_integer(42);
    int errs = celix_jansson_schema_validate(v, instance, countingErrorCb, &errorCount, nullptr);
    //Then the branch error is still propagated with an empty path (no crash on
    //appends(NULL)/emit(NULL)) and the anyOf error is reported as well
    EXPECT_EQ(1, errs);
    EXPECT_EQ(2, errorCount);
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaValidatePathChildOomFail) {
    //Given a validator with an object schema
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"type\":\"object\",\"properties\":{\"a\":{\"type\":\"string\"}}}");
    ASSERT_NE(nullptr, schema);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
    //And realloc is injected to fail inside path_child_checked
    //(realloc <- path_push <- path_child_checked <- v_object <- v_type <- root_validate, so level 4)
    json_auto_t* instance = loadSchema("{\"a\":\"x\"}");
    int errorCount = 0;
    celix_ei_expect_realloc((void*)celix_jansson_schema_root_validate, 4, nullptr);
    //Then validating fails closed with an out-of-memory error instead of
    //silently descending with a truncated path
    EXPECT_EQ(1, celix_jansson_schema_validate(v, instance, countingErrorCb, &errorCount, nullptr));
    EXPECT_EQ(1, errorCount);
}

/* ── Compile-time OOM: strdup / deep_copy propagation (P1) ────────────── */
/* The root $id keeps root_insert's uri_location/fragment as strbuf reallocs
 * (no strdup), so the level-2 strdup ordinals below count only
 * make_type_schema's own strdups plus root_insert's fragment strdup(""). */

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaPatternStrdupOomFail) {
    //Given strdup is injected to fail for the pattern string
    //(1st level-2 strdup of make_type_schema, before root_insert's fragment strdup)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"$id\":\"http://x/root\",\"type\":\"string\",\"pattern\":\"a\"}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_strdup((void*)celix_jansson_schema_set_root_schema, 2, nullptr);
    //Then compiling the schema should fail with NOMEM instead of compiling a
    //schema whose pattern_str is NULL
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaFormatStrdupOomFail) {
    //Given strdup is injected to fail for the format string
    //(1st level-2 strdup; the missing format checker returns early, so this
    //is the only strdup in the flow)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"$id\":\"http://x/root\",\"type\":\"string\",\"format\":\"email\"}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_strdup((void*)celix_jansson_schema_set_root_schema, 2, nullptr);
    //Then compiling the schema should fail with NOMEM instead of passing NULL
    //to a format checker at validation time
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaContentEncodingStrdupOomFail) {
    //Given strdup is injected to fail for the contentEncoding string
    //(1st level-2 strdup; the missing content checker returns early, so this
    //is the only strdup in the flow)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"$id\":\"http://x/root\",\"type\":\"string\",\"contentEncoding\":\"base64\"}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_strdup((void*)celix_jansson_schema_set_root_schema, 2, nullptr);
    //Then compiling the schema should fail with NOMEM instead of passing NULL
    //to a content checker at validation time
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaRequiredStrdupOomFail) {
    //Given strdup is injected to fail for the second required property name
    //(2nd level-2 strdup: 1st = "a", later ones = root_insert's fragment strdup(""))
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"$id\":\"http://x/root\",\"type\":\"object\",\"required\":[\"a\",\"b\"]}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_strdup((void*)celix_jansson_schema_set_root_schema, 2, nullptr, 2);
    //Then compiling the schema should fail with NOMEM instead of storing a
    //NULL property name
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaDependenciesRequiredStrdupOomFail) {
    //Given strdup is injected to fail for the second dependency name
    //(2nd level-2 strdup: 1st = "x", later ones = root_insert's fragment strdup(""))
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"$id\":\"http://x/root\",\"type\":\"object\",\"dependencies\":{\"a\":[\"x\",\"y\"]}}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_strdup((void*)celix_jansson_schema_set_root_schema, 2, nullptr, 2);
    //Then compiling the schema should fail with NOMEM instead of storing a
    //NULL dependency name
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaEnumDeepCopyOomFail) {
    //Given json_deep_copy is injected to fail for the enum values
    //(1st level-2 deep_copy: set_root_schema's two copies are level 1)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"$id\":\"http://x/root\",\"type\":\"string\",\"enum\":[\"a\"]}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_json_deep_copy((void*)celix_jansson_schema_set_root_schema, 2, nullptr);
    //Then compiling the schema should fail with NOMEM instead of validating
    //every instance against a NULL enum
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaConstDeepCopyOomFail) {
    //Given json_deep_copy is injected to fail for the const value
    //(1st level-2 deep_copy: set_root_schema's two copies are level 1)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"$id\":\"http://x/root\",\"type\":\"string\",\"const\":\"a\"}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_json_deep_copy((void*)celix_jansson_schema_set_root_schema, 2, nullptr);
    //Then compiling the schema should fail with NOMEM instead of validating
    //every instance against a NULL const
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

/* ── Registry OOM: stringHashMap_put failures (P2) ────────────────────── */

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaPropertiesPutOomFail) {
    //Given stringHashMap_put is injected to fail for a properties entry
    //(put <- make_type_schema <- depth <- set_root_schema, so level 2)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"$id\":\"http://x/root\",\"type\":\"object\",\"properties\":{\"a\":{\"type\":\"string\"}}}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_celix_stringHashMap_put((void*)celix_jansson_schema_set_root_schema, 2, CELIX_ENOMEM);
    //Then compiling the schema should fail with NOMEM instead of leaking the
    //compiled subschema
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaDependenciesPutOomFail) {
    //Given stringHashMap_put is injected to fail for a dependencies entry
    //(put <- make_type_schema <- depth <- set_root_schema, so level 2)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"$id\":\"http://x/root\",\"type\":\"object\",\"dependencies\":{\"a\":{\"type\":\"string\"}}}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_celix_stringHashMap_put((void*)celix_jansson_schema_set_root_schema, 2, CELIX_ENOMEM);
    //Then compiling the schema should fail with NOMEM instead of leaking the
    //compiled dependency
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaDefinitionsPutOomFail) {
    //Given stringHashMap_put is injected to fail for a definitions entry
    //(put <- depth <- set_root_schema, so level 1)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"definitions\":{\"x\":{\"type\":\"string\"}},\"type\":\"string\"}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_celix_stringHashMap_put((void*)celix_jansson_schema_set_root_schema, 1, CELIX_ENOMEM);
    //Then compiling the schema should fail with NOMEM instead of leaking the
    //compiled definition
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaDefinitionsFragAsprintfFail) {
    //Given asprintf is injected to fail for the definitions fragment key
    //(asprintf <- depth <- set_root_schema, so level 1)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"definitions\":{\"a\":{\"type\":\"string\"}}}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_asprintf((void*)celix_jansson_schema_set_root_schema, 1, -1);
    //Then compiling fails with NOMEM and the definition node is released
    //(no leak: the map never took the ref)
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaUnresolvedPutOomFail) {
    //Given stringHashMap_put is injected to fail for the unresolved-ref
    //placeholder entry (put <- depth <- set_root_schema, so level 1)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"$ref\":\"#/definitions/x\"}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_celix_stringHashMap_put((void*)celix_jansson_schema_set_root_schema, 1, CELIX_ENOMEM);
    //Then compiling the schema should fail with NOMEM instead of leaving the
    //placeholder's owning ref stranded
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaGetOrCreateFilePutOomFail) {
    //Given stringHashMap_put is injected to fail inside get_or_create_file
    //(put <- get_or_create_file <- root_insert <- set_root_schema, so level 2)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"type\":\"string\"}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_celix_stringHashMap_put((void*)celix_jansson_schema_set_root_schema, 2, CELIX_ENOMEM);
    //Then get_or_create_file returns NULL, root_insert reports NOMEM and
    //set_root_schema propagates it instead of registering a file the map does
    //not contain
    char* errmsg = nullptr;
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, &errmsg));
    EXPECT_NE(nullptr, errmsg);
    free(errmsg);
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaRootInsertPutOomFail) {
    //Given stringHashMap_put is injected to fail for the root node registration
    //in root_insert (put <- root_insert <- depth <- set_root_schema, so level 2;
    //the get_or_create_file put inside root_insert is level 3)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"$id\":\"http://x/root\",\"type\":\"string\"}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_celix_stringHashMap_put((void*)celix_jansson_schema_set_root_schema, 2, CELIX_ENOMEM);
    //Then compiling the schema should fail with NOMEM instead of reporting OK
    //for a node the registry never stored (which would later surface as a
    //confusing REF_UNRESOLVED)
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaRootInsertRetainedVecPushOomFail) {
    //Given realloc is injected to fail in celix_jansson_vec_push, hit by the
    //retained-placeholder push inside root_insert (1st = Phase A locs-vec push,
    //2nd = Phase B pairs push, 3rd = retained push during the document-fragment
    //walk's root_insert). The failed walk is retried on the next
    //resolve_external_refs iteration.
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidatorWithLoader();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"$ref\":\"http://example.com/doc#/properties/b\"}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_realloc((void*)celix_jansson_vec_push, 0, nullptr, 3);
    //Then the schema still compiles (the failed resolution self-heals on retry)
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaResolvePlaceholderVecPushOomFail) {
    //Given realloc is injected to fail in celix_jansson_vec_push, hit by the
    //retained-placeholder push inside resolve_placeholder (1st = Phase A
    //locs-vec push, 2nd = Phase B pairs push, 3rd = retained push for the
    //definitions entry registered by the document compile). The failed
    //resolution is retried on the next resolve_external_refs iteration.
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidatorWithLoader();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"$ref\":\"http://example.com/doc#/definitions/x\"}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_realloc((void*)celix_jansson_vec_push, 0, nullptr, 3);
    //Then the schema still compiles (the failed resolution self-heals on retry)
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

/* ── Consistency: patternProperties regcomp (P3) ───────────────────────── */

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaPatternPropertiesInvalidRegcomp) {
    //Given a patternProperties key that is an invalid regex
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema =
        loadSchema("{\"type\":\"object\",\"patternProperties\":{\"***invalid\":{\"type\":\"string\"}}}");
    ASSERT_NE(nullptr, schema);
    //Then compiling the schema should fail with INVALID_PATTERN like the
    //string "pattern" keyword does, instead of silently storing the pattern
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_INVALID_PATTERN, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

/* ── Runtime path building OOM: path_child_checked call sites (P3) ────── */
/* All inject realloc inside celix_jansson_path_push, reached through
 * path_child_checked (realloc <- path_push <- path_child_checked <- validator
 * <- v_type <- root_validate, so level 4). */

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaValidatePathChildCopyOomFail) {
    //Given a nested object schema and realloc is injected to fail for the
    //parent-token copy inside path_child_checked. The nested property adds two
    //extra frames (v_object <- v_type), so the copy push is reached at level 6
    //(realloc <- path_push <- path_child_checked <- v_object(b) <- v_type(b)
    //<- v_object(root) <- v_type(root) <- root_validate)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema(
        "{\"type\":\"object\",\"properties\":{\"b\":{\"type\":\"object\",\"properties\":{\"a\":{\"type\":\"string\"}}}}}");
    ASSERT_NE(nullptr, schema);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
    json_auto_t* instance = loadSchema("{\"b\":{\"a\":\"x\"}}");
    int errorCount = 0;
    celix_ei_expect_realloc((void*)celix_jansson_schema_root_validate, 6, nullptr);
    //Then validating fails closed with an out-of-memory error
    EXPECT_EQ(1, celix_jansson_schema_validate(v, instance, countingErrorCb, &errorCount, nullptr));
    EXPECT_EQ(1, errorCount);
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaValidateRequiredPathOomFail) {
    //Given a dependencies-array schema and realloc is injected to fail for the
    //required-error path build in v_required. The v_required node is dispatched
    //directly from obj_validate_deps, so the push is reached at level 6
    //(realloc <- path_push <- path_child_checked <- v_required <-
    //obj_validate_deps <- v_object <- v_type <- root_validate)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"type\":\"object\",\"dependencies\":{\"a\":[\"b\"]}}");
    ASSERT_NE(nullptr, schema);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
    json_auto_t* instance = loadSchema("{\"a\":1}");
    int errorCount = 0;
    celix_ei_expect_realloc((void*)celix_jansson_schema_root_validate, 6, nullptr);
    //Then validating fails closed with an out-of-memory error
    EXPECT_EQ(1, celix_jansson_schema_validate(v, instance, countingErrorCb, &errorCount, nullptr));
    EXPECT_EQ(1, errorCount);
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaValidatePropertyNamesPathOomFail) {
    //Given a propertyNames schema and realloc is injected to fail for the
    //property-name path build
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"type\":\"object\",\"propertyNames\":{\"type\":\"string\"}}");
    ASSERT_NE(nullptr, schema);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
    json_auto_t* instance = loadSchema("{\"a\":1}");
    int errorCount = 0;
    celix_ei_expect_realloc((void*)celix_jansson_schema_root_validate, 4, nullptr);
    //Then validating fails closed with an out-of-memory error
    EXPECT_EQ(1, celix_jansson_schema_validate(v, instance, countingErrorCb, &errorCount, nullptr));
    EXPECT_EQ(1, errorCount);
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaValidateDefaultPathOomFail) {
    //Given a property with a default and realloc is injected to fail for the
    //default-fill path build
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"type\":\"object\",\"properties\":{\"a\":{\"type\":\"string\",\"default\":\"x\"}}}");
    ASSERT_NE(nullptr, schema);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
    json_auto_t* instance = loadSchema("{}");
    int errorCount = 0;
    celix_ei_expect_realloc((void*)celix_jansson_schema_root_validate, 4, nullptr);
    //Then validating fails closed with an out-of-memory error
    EXPECT_EQ(1, celix_jansson_schema_validate(v, instance, countingErrorCb, &errorCount, nullptr));
    EXPECT_EQ(1, errorCount);
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaValidateDepsPathOomFail) {
    //Given a dependencies schema and realloc is injected to fail for the
    //dependency path build
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"type\":\"object\",\"dependencies\":{\"a\":{\"type\":\"string\"}}}");
    ASSERT_NE(nullptr, schema);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
    json_auto_t* instance = loadSchema("{\"a\":\"x\"}");
    int errorCount = 0;
    //obj_validate_deps adds a frame: realloc <- path_push <- path_child_checked
    //<- obj_validate_deps <- v_object <- v_type <- root_validate, so level 5
    celix_ei_expect_realloc((void*)celix_jansson_schema_root_validate, 5, nullptr);
    //Then validating fails closed with an out-of-memory error
    EXPECT_EQ(1, celix_jansson_schema_validate(v, instance, countingErrorCb, &errorCount, nullptr));
    EXPECT_EQ(1, errorCount);
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaValidateArrayItemsPathOomFail) {
    //Given an items schema and realloc is injected to fail for the item path build
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"type\":\"array\",\"items\":{\"type\":\"string\"}}");
    ASSERT_NE(nullptr, schema);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
    json_auto_t* instance = json_pack("[s]", "x");
    int errorCount = 0;
    celix_ei_expect_realloc((void*)celix_jansson_schema_root_validate, 4, nullptr);
    //Then validating fails closed with an out-of-memory error
    EXPECT_EQ(1, celix_jansson_schema_validate(v, instance, countingErrorCb, &errorCount, nullptr));
    EXPECT_EQ(1, errorCount);
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaValidateTupleItemsPathOomFail) {
    //Given a tuple items schema and realloc is injected to fail for the item path build
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"type\":\"array\",\"items\":[{\"type\":\"string\"}]}");
    ASSERT_NE(nullptr, schema);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
    json_auto_t* instance = json_pack("[s]", "x");
    int errorCount = 0;
    celix_ei_expect_realloc((void*)celix_jansson_schema_root_validate, 4, nullptr);
    //Then validating fails closed with an out-of-memory error
    EXPECT_EQ(1, celix_jansson_schema_validate(v, instance, countingErrorCb, &errorCount, nullptr));
    EXPECT_EQ(1, errorCount);
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaValidateAdditionalItemsPathOomFail) {
    //Given a tuple+additionalItems schema and realloc is injected to fail for
    //the additional-item path build (2nd realloc: 1st = the tuple item push)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema =
        loadSchema("{\"type\":\"array\",\"items\":[{\"type\":\"string\"}],\"additionalItems\":{\"type\":\"string\"}}");
    ASSERT_NE(nullptr, schema);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
    json_auto_t* instance = json_pack("[s,s]", "x", "y");
    int errorCount = 0;
    celix_ei_expect_realloc((void*)celix_jansson_schema_root_validate, 4, nullptr, 2);
    //Then validating fails closed with an out-of-memory error
    EXPECT_EQ(1, celix_jansson_schema_validate(v, instance, countingErrorCb, &errorCount, nullptr));
    EXPECT_EQ(1, errorCount);
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaValidateContainsPathOomFail) {
    //Given a contains schema and realloc is injected to fail for the contains
    //element path build (after the first-sink allocation succeeds)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"type\":\"array\",\"contains\":{\"type\":\"string\"}}");
    ASSERT_NE(nullptr, schema);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
    json_auto_t* instance = json_pack("[s]", "x");
    int errorCount = 0;
    celix_ei_expect_realloc((void*)celix_jansson_schema_root_validate, 4, nullptr);
    //Then validating fails closed with an out-of-memory error
    EXPECT_EQ(1, celix_jansson_schema_validate(v, instance, countingErrorCb, &errorCount, nullptr));
    EXPECT_EQ(1, errorCount);
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaPatternPropertiesInvalidRegcompSecond) {
    //Given patternProperties where the second pattern fails to compile, the
    //first (compiled) entry must be released (regfree + unref) before the
    //INVALID_PATTERN error propagates
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema =
        loadSchema("{\"type\":\"object\",\"patternProperties\":{\"a\":{\"type\":\"string\"},\"***invalid\":{\"type\":\"string\"}}}");
    ASSERT_NE(nullptr, schema);
    //Then compiling the schema should fail with INVALID_PATTERN
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_INVALID_PATTERN, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaDocFragmentDeriveOomFail) {
    //Given strdup is injected to fail inside uri_update, hit by the
    //document-fragment walk's $id base update for properties.x. Only the
    //walk's derive has a scheme ("http://y/z" → path = strdup("/z")); the
    //$ref URI derive ("#/properties/x") and the cur_base init ("") do not
    //strdup, so ordinal 1 is the walk's derive. The failed walk returns NULL
    //and the ref is resolved on the next resolve_external_refs iteration.
    //properties.x is used (not definitions) so the walk is actually required.
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema =
        loadSchema("{\"properties\":{\"x\":{\"$id\":\"http://y/z\",\"type\":\"string\"}},\"$ref\":\"#/properties/x\"}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_strdup((void*)celix_jansson_uri_update, 0, nullptr);
    //Then the schema still compiles (the failed walk self-heals on retry)
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaValidatePathStrFallbackOomFail) {
    //Given strdup is injected to fail for path_str's empty-string fallback
    //(the only strdup inside path_str), making path_str return NULL — the
    //only way emit_error_v's defensive empty-path fallback runs
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"type\":\"string\"}");
    ASSERT_NE(nullptr, schema);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
    json_auto_t* instance = json_integer(42);
    int errorCount = 0;
    celix_ei_expect_strdup((void*)celix_jansson_path_str, 0, nullptr);
    //Then the type error is still reported with an empty path instead of NULL
    EXPECT_EQ(1, celix_jansson_schema_validate(v, instance, countingErrorCb, &errorCount, nullptr));
    EXPECT_EQ(1, errorCount);
}

/* ── Round 2: logic vec_push, invalid schema entries, default handling ── */

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaLogicNotVecPushOomFail) {
    //Given realloc is injected to fail in celix_jansson_vec_push, hit by the
    //not-node push into the logic vec (the first vec_push of the compile)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"not\":{\"type\":\"string\"}}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_realloc((void*)celix_jansson_vec_push, 0, nullptr);
    //Then compiling fails with NOMEM instead of leaking the not node and
    //silently dropping the keyword
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaLogicComboVecPushOomFail) {
    //Given realloc is injected to fail in celix_jansson_vec_push, hit by the
    //combo-node push into the logic vec (the first vec_push of the compile)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"allOf\":[{\"type\":\"string\"}]}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_realloc((void*)celix_jansson_vec_push, 0, nullptr);
    //Then compiling fails with NOMEM instead of leaking the combo node and
    //silently dropping the keyword
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaRequiredNonStringElement) {
    //Given a required array containing a non-string entry
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"type\":\"object\",\"required\":[1]}");
    ASSERT_NE(nullptr, schema);
    //Then compiling rejects the schema instead of crashing on strdup(NULL)
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_INVALID_SCHEMA, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaDependenciesNonStringElement) {
    //Given a dependencies-array containing a non-string entry
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"type\":\"object\",\"dependencies\":{\"a\":[1]}}");
    ASSERT_NE(nullptr, schema);
    //Then compiling rejects the schema instead of crashing on strdup(NULL)
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_INVALID_SCHEMA, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaValidateDefaultPatchPathBuildOomFail) {
    //Given realloc is injected to fail for the second default-fill path
    //build (the JSON Patch path; ordinal 2 — the first matching realloc is
    //the default-value callback's path build)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema =
        loadSchema("{\"type\":\"object\",\"properties\":{\"a\":{\"type\":\"string\",\"default\":\"x\"}}}");
    ASSERT_NE(nullptr, schema);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
    json_auto_t* instance = loadSchema("{}");
    int errorCount = 0;
    celix_ei_expect_realloc((void*)celix_jansson_schema_root_validate, 4, nullptr, 2);
    //Then validating fails closed with an out-of-memory error
    EXPECT_EQ(1, celix_jansson_schema_validate(v, instance, countingErrorCb, &errorCount, nullptr));
    EXPECT_EQ(1, errorCount);
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaValidateDefaultPatchPathOomFail) {
    //Given realloc is injected to fail in strbuf_append while building the
    //default patch path (realloc <- strbuf_append <- appendc <- path_str, so
    //level 0: strbuf_append is the realloc's direct caller)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema =
        loadSchema("{\"type\":\"object\",\"properties\":{\"a\":{\"type\":\"string\",\"default\":\"x\"}}}");
    ASSERT_NE(nullptr, schema);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
    json_auto_t* instance = loadSchema("{}");
    int errorCount = 0;
    celix_ei_expect_realloc((void*)celix_jansson_strbuf_append, 0, nullptr);
    //Then the default patch is not applied and the validation fails closed
    EXPECT_EQ(1, celix_jansson_schema_validate(v, instance, countingErrorCb, &errorCount, nullptr));
    EXPECT_EQ(1, errorCount);
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaEmitErrorVasprintfFail) {
    //Given vasprintf is injected to fail while formatting a validation
    //error message (vasprintf <- emit_error_v <- emit_error <- v_type <-
    //root_validate, so level 3)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"type\":\"string\"}");
    ASSERT_NE(nullptr, schema);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
    json_auto_t* instance = json_integer(42);
    std::string message;
    celix_ei_expect_vasprintf((void*)celix_jansson_schema_root_validate, 3, -1);
    //Then the error is still reported with the OOM hint constant
    EXPECT_EQ(1, celix_jansson_schema_validate(v, instance, captureMessageCb, &message, nullptr));
    EXPECT_EQ("out of memory: error message unavailable", message);
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaDefaultDeepCopyOomFail) {
    //Given json_deep_copy is injected to fail for the type-schema default
    //(1st level-2 deep_copy: set_root_schema's two copies are level 1)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"$id\":\"http://x/root\",\"type\":\"string\",\"default\":\"x\"}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_json_deep_copy((void*)celix_jansson_schema_set_root_schema, 2, nullptr);
    //Then compiling fails with NOMEM instead of silently dropping the default
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaRefDefaultDeepCopyOomFail) {
    //Given json_deep_copy is injected to fail for the $ref default
    //(1st level-1 deep_copy: deep_copy <- depth <- set_root_schema)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"$ref\":\"#\",\"default\":\"x\"}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_json_deep_copy((void*)celix_jansson_schema_set_root_schema, 1, nullptr);
    //Then compiling fails with NOMEM instead of silently dropping the default
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_NOMEM, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaValidateDefaultPatchOomFail) {
    //Given a property default and json_array_append_new is injected to fail
    //inside celix_json_patch_add (its only append point)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema =
        loadSchema("{\"type\":\"object\",\"properties\":{\"a\":{\"type\":\"string\",\"default\":\"x\"}}}");
    ASSERT_NE(nullptr, schema);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
    json_auto_t* instance = loadSchema("{}");
    int errorCount = 0;
    celix_ei_expect_json_array_append_new((void*)celix_json_patch_add, 0, -1);
    //Then validating fails closed with an out-of-memory error instead of
    //silently dropping the default patch
    EXPECT_EQ(1, celix_jansson_schema_validate(v, instance, countingErrorCb, &errorCount, nullptr));
    EXPECT_EQ(1, errorCount);
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaValidateRootDefaultPatchOomFail) {
    //Given a root default (no type, so a null instance passes the type check)
    //and json_array_append_new is injected to fail inside celix_json_patch_add
    //(its only append point)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"default\":\"x\"}");
    ASSERT_NE(nullptr, schema);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
    json_auto_t* instance = json_null();
    int errorCount = 0;
    celix_ei_expect_json_array_append_new((void*)celix_json_patch_add, 0, -1);
    //Then validating fails closed with an out-of-memory error instead of
    //silently dropping the root default patch
    EXPECT_EQ(1, celix_jansson_schema_validate(v, instance, countingErrorCb, &errorCount, nullptr));
    EXPECT_EQ(1, errorCount);
}

/* ── strbuf append OOM: path_str, coll_propagate, fragment walk ───────── */

/* Message-capturing error callback: the coll_propagate OOM test needs the
 * message CONTENT (prefixed vs raw) to tell the fallback from the normal
 * path, which an error count alone cannot distinguish. */
static std::vector<std::string> g_collMsgs;
static void captureErrorCb(const char*, json_t*, const char* msg, void* ud) {
    g_collMsgs.emplace_back(msg ? msg : "");
    int* count = static_cast<int*>(ud);
    if (count)
        (*count)++;
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaValidatePathStrOomFail) {
    //Given realloc is injected to fail in strbuf_append, hit by the first
    //append of path_str's path build (the '/' separator, ordinal 1 — no
    //strbuf realloc precedes it in this flow) while reporting the type error
    //at the nested "/a" path
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"type\":\"object\",\"properties\":{\"a\":{\"type\":\"string\"}}}");
    ASSERT_NE(nullptr, schema);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
    json_auto_t* instance = loadSchema("{\"a\":42}");
    int errorCount = 0;
    celix_ei_expect_realloc((void*)celix_jansson_strbuf_append, 0, nullptr, 1);
    //Then the type error is still reported with the empty-path fallback
    //instead of a silently truncated path, no crash
    EXPECT_EQ(1, celix_jansson_schema_validate(v, instance, countingErrorCb, &errorCount, nullptr));
    EXPECT_EQ(1, errorCount);

    //And with a token long enough to realloc mid-build (the 64th char, ordinal
    //2 — the first append grew the cap to 64), the failure is hit by a
    //token-char append instead of the '/' separator (inner oom branch)
    std::string key(64, 'a');
    std::string longSchema =
        "{\"type\":\"object\",\"properties\":{\"" + key + "\":{\"type\":\"string\"}}}";
    std::string longInstance = "{\"" + key + "\":42}";
    celix_autoptr(celix_jansson_schema_validator_t) v2 = makeValidator();
    ASSERT_NE(nullptr, v2);
    json_auto_t* schema2 = loadSchema(longSchema.c_str());
    ASSERT_NE(nullptr, schema2);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v2, schema2, nullptr));
    json_auto_t* instance2 = loadSchema(longInstance.c_str());
    errorCount = 0;
    celix_ei_expect_realloc((void*)celix_jansson_strbuf_append, 0, nullptr, 2);
    //Then the same empty-path fallback applies, no crash
    EXPECT_EQ(1, celix_jansson_schema_validate(v2, instance2, countingErrorCb, &errorCount, nullptr));
    EXPECT_EQ(1, errorCount);
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaValidateRootDefaultPathStrOomFail) {
    //Given realloc is injected to fail in strbuf_append, hit by path_str
    //while applying a property-level default to a null instance (the "/a"
    //path build, ordinal 1) — a root default would have an empty path and
    //never append, so the property default is required here
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"properties\":{\"a\":{\"default\":5}}}");
    ASSERT_NE(nullptr, schema);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
    json_auto_t* instance = loadSchema("{\"a\":null}");
    int errorCount = 0;
    celix_ei_expect_realloc((void*)celix_jansson_strbuf_append, 0, nullptr, 1);
    //Then validating fails closed with an out-of-memory error instead of
    //crashing on a NULL patch path (json_string(NULL)), no leak of the value
    EXPECT_EQ(1, celix_jansson_schema_validate(v, instance, countingErrorCb, &errorCount, nullptr));
    EXPECT_EQ(1, errorCount);
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaValidateCollPropagateOomFail) {
    //Given realloc is injected to fail in strbuf_append, hit by the first
    //append of coll_propagate's prefixed-message build (ordinal 1 — the
    //failing branch emits at the root path, whose empty path_str never
    //appends, so no strbuf realloc precedes it)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    json_auto_t* schema = loadSchema("{\"anyOf\":[{\"type\":\"string\"}]}");
    ASSERT_NE(nullptr, schema);
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
    json_auto_t* instance = json_integer(42);
    int errorCount = 0;
    g_collMsgs.clear();
    celix_ei_expect_realloc((void*)celix_jansson_strbuf_append, 0, nullptr, 1);
    //Then both the anyOf failure and the branch error are still reported, the
    //latter with the raw message instead of a truncated one (only the
    //"[combination:" prefix is lost — two emissions either way, the message
    //content is what distinguishes the fallback)
    EXPECT_EQ(1, celix_jansson_schema_validate(v, instance, captureErrorCb, &errorCount, nullptr));
    EXPECT_EQ(2, errorCount);
    ASSERT_EQ(2u, g_collMsgs.size());
    EXPECT_NE(std::string::npos, g_collMsgs[0].find("no subschema has succeeded"));
    EXPECT_NE(std::string::npos, g_collMsgs[1].find("unexpected instance type"));
    EXPECT_EQ(std::string::npos, g_collMsgs[1].find("[combination:"));
}

TEST_F(JanssonExtSchemaErrorInjectionTestSuite, SchemaDocFragmentTokenDecodeOomFail) {
    //Given realloc is injected to fail in strbuf_append, hit by the first
    //append of the document-fragment walk's token decode (ordinal 2: 1 =
    //percent_decode of the fragment inside uri_update, 2 = the walk's first
    //token "definitions" — uri_location/uri_fragment allocate via strdup and
    //malloc, not strbuf)
    celix_autoptr(celix_jansson_schema_validator_t) v = makeValidator();
    ASSERT_NE(nullptr, v);
    //"a" compiles before "b" registers, so its $ref forces the fragment walk
    //(which aborts on OOM); the placeholder then resolves from the registry
    //once "b" is registered — self-healing to OK
    json_auto_t* schema =
        loadSchema("{\"definitions\":{\"a\":{\"$ref\":\"#/definitions/b\"},\"b\":{\"type\":\"string\"}}}");
    ASSERT_NE(nullptr, schema);
    celix_ei_expect_realloc((void*)celix_jansson_strbuf_append, 0, nullptr, 2);
    //Then the walk aborts (a truncated token is never used for lookups) and
    //the ref resolves from the registry on the next pass
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, nullptr));
}
