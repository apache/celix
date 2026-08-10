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
#include <celix_json_patch.h>
#include <jansson.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

    if (celix_json_pointer_set_new(doc, ptr, json_integer(42)) != 0) {
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

    return 0;
}
