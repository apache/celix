//  Licensed to the Apache Software Foundation (ASF) under one
//  or more contributor license agreements.  See the NOTICE file
//  distributed with this work for additional information
//  regarding copyright ownership.  The ASF licenses this file
//  to you under the Apache License, Version 2.0 (the
//  "License"); you may not use this file except in compliance
//  with the License.  You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing,
//  software distributed under the License is distributed on an
//  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
//  KIND, either express or implied.  See the License for the
//  specific language governing permissions and limitations
//  under the License.
//

#include <celix_jansson_pointer.h>
#include <celix_jansson_schema.h>
#include <celix_json_merge_patch.h>
#include <celix_json_patch.h>
#include <jansson.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Applies a merge patch and checks the result against the expected JSON. */
static int applyAndExpectEqual(const char* targetText, const char* patchText, const char* expectedText) {
    json_error_t jerr;
    json_t* target = json_loads(targetText, JSON_DECODE_ANY, &jerr);
    json_t* patch = json_loads(patchText, JSON_DECODE_ANY, &jerr);
    json_t* expected = json_loads(expectedText, JSON_DECODE_ANY, &jerr);
    if (!target || !patch || !expected) {
        fprintf(stderr, "Error parsing merge patch test input: %s\n", jerr.text);
        json_decref(target);
        json_decref(patch);
        json_decref(expected);
        return 1;
    }

    json_t* result = celix_json_merge_patch(target, patch);
    int rc = result && json_equal(result, expected) ? 0 : 1;
    if (rc != 0) {
        char* resultStr = json_dumps(result, JSON_ENCODE_ANY);
        fprintf(stderr,
                "Merge patch mismatch:\n  target   = %s\n  patch    = %s\n  expected = %s\n  got      = %s\n",
                targetText, patchText, expectedText, resultStr ? resultStr : "?");
        free(resultStr);
    }
    json_decref(result);
    json_decref(target);
    json_decref(patch);
    json_decref(expected);
    return rc;
}

int main() {
    /* ── JSON Schema draft-7 validation ─────────────────────────────────── */
    json_error_t jerr;
    json_t* schema = json_loads(
        "{\"type\":\"object\",\"properties\":{\"name\":{\"type\":\"string\"}},\"required\":[\"name\"]}",
        0, &jerr);
    if (!schema) {
        fprintf(stderr, "Error parsing schema: %s\n", jerr.text);
        return 1;
    }

    celix_jansson_schema_validator_t* validator = celix_jansson_schema_validator_create(
        NULL, NULL, celix_jansson_schema_default_format_check, NULL, NULL, NULL);
    if (!validator) {
        fprintf(stderr, "Failed to create validator\n");
        json_decref(schema);
        return 1;
    }

    char* errmsg = NULL;
    int rc = celix_jansson_schema_set_root_schema(validator, schema, &errmsg);
    if (rc != CELIX_JANSSON_SCHEMA_OK) {
        fprintf(stderr, "Schema compilation error: %s\n", errmsg ? errmsg : celix_jansson_schema_strerror(rc));
        free(errmsg);
        json_decref(schema);
        celix_jansson_schema_validator_destroy(validator);
        return 1;
    }
    free(errmsg);
    json_decref(schema);

    json_t* instance = json_loads("{\"name\":\"celix\"}", 0, &jerr);
    if (!instance) {
        fprintf(stderr, "Error parsing instance: %s\n", jerr.text);
        celix_jansson_schema_validator_destroy(validator);
        return 1;
    }
    int errors = celix_jansson_schema_validate(validator, instance, NULL, NULL, NULL);
    printf("valid instance errors = %d\n", errors);
    json_decref(instance);
    if (errors != 0) {
        celix_jansson_schema_validator_destroy(validator);
        return 1;
    }

    instance = json_loads("{}", 0, &jerr);
    if (!instance) {
        fprintf(stderr, "Error parsing instance: %s\n", jerr.text);
        celix_jansson_schema_validator_destroy(validator);
        return 1;
    }
    errors = celix_jansson_schema_validate(validator, instance, NULL, NULL, NULL);
    printf("invalid instance errors = %d\n", errors);
    json_decref(instance);
    celix_jansson_schema_validator_destroy(validator);
    if (errors == 0) {
        fprintf(stderr, "Expected invalid instance to be rejected\n");
        return 1;
    }

    /* ── JSON Pointer (RFC 6901) ───────────────────────────────────────── */
    json_t* doc = json_loads("{\"foo\":{\"bar\":1}}", 0, &jerr);
    if (!doc) {
        fprintf(stderr, "Error parsing document: %s\n", jerr.text);
        return 1;
    }
    celix_json_pointer_t* ptr = celix_json_pointer_create("/foo/bar");
    json_t* value = ptr ? celix_json_pointer_get(doc, ptr) : NULL;
    char* ptr_str = ptr ? celix_json_pointer_to_string(ptr) : NULL;
    if (!ptr || !value || !ptr_str || strcmp(ptr_str, "/foo/bar") != 0) {
        fprintf(stderr, "JSON pointer resolution failed\n");
        free(ptr_str);
        celix_json_pointer_destroy(ptr);
        json_decref(doc);
        return 1;
    }
    printf("pointer = %s, value = %lld\n", ptr_str, json_integer_value(value));
    free(ptr_str);

    if (celix_json_pointer_set(doc, ptr, json_integer(42)) != 0) {
        fprintf(stderr, "JSON pointer set failed\n");
        celix_json_pointer_destroy(ptr);
        json_decref(doc);
        return 1;
    }
    printf("value after set = %lld\n", json_integer_value(celix_json_pointer_get(doc, ptr)));
    celix_json_pointer_destroy(ptr);

    /* ── JSON Patch (RFC 6902) ─────────────────────────────────────────── */
    json_t* patch = json_array();
    if (!patch || celix_json_patch_add(patch, "/foo/bar", json_integer(7)) != 0) {
        fprintf(stderr, "JSON patch add failed\n");
        json_decref(patch);
        json_decref(doc);
        return 1;
    }
    json_t* patched = celix_json_patch_apply(doc, patch);
    if (!patched || json_integer_value(json_object_get(json_object_get(patched, "foo"), "bar")) != 7) {
        fprintf(stderr, "JSON patch apply failed\n");
        json_decref(patched);
        json_decref(patch);
        json_decref(doc);
        return 1;
    }
    char* patched_str = json_dumps(patched, 0);
    printf("patched = %s\n", patched_str ? patched_str : "?");
    free(patched_str);
    json_decref(patched);
    json_decref(patch);
    json_decref(doc);

    /* ── JSON Merge Patch (RFC 7396) ────────────────────────────────────── */
    /* Section 1 example and Appendix A examples, as in the gtest suite */
    int mergePatchFailures = 0;
    mergePatchFailures += applyAndExpectEqual("{\"a\":\"b\",\"c\":{\"d\":\"e\",\"f\":\"g\"}}",
                                              "{\"a\":\"z\",\"c\":{\"f\":null}}",
                                              "{\"a\":\"z\",\"c\":{\"d\":\"e\"}}");
    mergePatchFailures += applyAndExpectEqual("{\"a\":\"b\"}", "{\"b\":\"c\"}", "{\"a\":\"b\",\"b\":\"c\"}");      /* add member */
    mergePatchFailures += applyAndExpectEqual("{\"a\":\"b\"}", "{\"a\":null}", "{}");                             /* remove member */
    mergePatchFailures += applyAndExpectEqual("[\"a\",\"b\"]", "[\"c\",\"d\"]", "[\"c\",\"d\"]");                 /* non-object patch */
    mergePatchFailures += applyAndExpectEqual("{\"a\":\"foo\"}", "null", "null");                                 /* null patch */
    mergePatchFailures += applyAndExpectEqual("{\"a\":\"foo\"}", "\"bar\"", "\"bar\"");                           /* scalar patch */
    mergePatchFailures += applyAndExpectEqual("[1,2]", "{\"a\":\"b\",\"c\":null}", "{\"a\":\"b\"}");              /* object patch on array */
    mergePatchFailures += applyAndExpectEqual("{\"a\":1}", "{}", "{\"a\":1}");                                    /* empty patch on object */
    mergePatchFailures += applyAndExpectEqual("[1,2]", "{}", "{}");                                               /* empty patch on non-object */
    mergePatchFailures += applyAndExpectEqual("{}", "{\"a\":{\"bb\":{\"ccc\":null}}}", "{\"a\":{\"bb\":{}}}");    /* nested absent member */
    if (mergePatchFailures != 0) {
        fprintf(stderr, "JSON merge patch tests failed\n");
        return 1;
    }
    printf("merge patch RFC 7396 examples ok\n");

    /* NULL arguments are rejected */
    doc = json_loads("{\"a\":1}", 0, &jerr);
    if (!doc) {
        fprintf(stderr, "Error parsing merge patch document: %s\n", jerr.text);
        return 1;
    }
    if (celix_json_merge_patch(NULL, doc) != NULL || celix_json_merge_patch(doc, NULL) != NULL ||
        celix_json_merge_patch(NULL, NULL) != NULL) {
        fprintf(stderr, "Merge patch with NULL argument(s) should return NULL\n");
        json_decref(doc);
        return 1;
    }
    json_decref(doc);

    /* inputs are never modified and a new document is returned */
    doc = json_loads("{\"a\":{\"b\":1},\"c\":[1,2]}", 0, &jerr);
    patch = json_loads("{\"a\":{\"b\":2}}", 0, &jerr);
    if (!doc || !patch) {
        fprintf(stderr, "Error parsing merge patch inputs: %s\n", jerr.text);
        json_decref(doc);
        json_decref(patch);
        return 1;
    }
    json_t* docCopy = json_deep_copy(doc);
    json_t* patchCopy = json_deep_copy(patch);
    json_t* merged = celix_json_merge_patch(doc, patch);
    if (!docCopy || !patchCopy || !merged || merged == doc || merged == patch ||
        !json_equal(doc, docCopy) || !json_equal(patch, patchCopy) ||
        json_integer_value(json_object_get(json_object_get(merged, "a"), "b")) != 2 ||
        !json_object_get(merged, "c")) {
        fprintf(stderr, "Merge patch should return a new document without modifying its inputs\n");
        json_decref(merged);
        json_decref(patchCopy);
        json_decref(docCopy);
        json_decref(patch);
        json_decref(doc);
        return 1;
    }
    char* merged_str = json_dumps(merged, 0);
    printf("merged = %s\n", merged_str ? merged_str : "?");
    free(merged_str);
    json_decref(merged);
    json_decref(patchCopy);
    json_decref(docCopy);
    json_decref(patch);
    json_decref(doc);

    return 0;
}
