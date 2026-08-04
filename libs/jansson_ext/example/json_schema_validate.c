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
#include "celix_jansson_schema.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Schema loader callback: reads external schema files from the current
 * working directory based on the URI path.
 */
static int file_loader(const char* uri, json_t** schema_out, void* user_data) {
    (void)user_data;

    /* Serve the built-in draft-7 meta-schema */
    if (strcmp(uri, "http://json-schema.org/draft-07/schema") == 0 ||
        strcmp(uri, "http://json-schema.org/draft-07/schema#") == 0) {
        *schema_out = celix_jansson_schema_draft7_meta_schema();
        if (!*schema_out)
            return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
        return CELIX_JANSSON_SCHEMA_OK;
    }

    /* Try to load from disk: ./<path> */
    char path[4096];
    const char* uri_path = uri;

    /* Strip scheme://authority prefix if present */
    const char* scheme_end = strstr(uri, "://");
    if (scheme_end) {
        const char* slash = strchr(scheme_end + 3, '/');
        if (slash)
            uri_path = slash;
        else
            uri_path = "/";
    }

    snprintf(path, sizeof(path), ".%s", uri_path);

    json_error_t jerr;
    *schema_out = json_load_file(path, 0, &jerr);
    if (!*schema_out) {
        fprintf(stderr, "Error loading schema '%s': %s\n", path, jerr.text);
        return CELIX_JANSSON_SCHEMA_ERROR_LOADER;
    }

    return CELIX_JANSSON_SCHEMA_OK;
}

static void print_error(const char* ptr, json_t* instance, const char* msg, void* user_data) {
    (void)user_data;
    char* inst_str = json_dumps(instance, JSON_ENCODE_ANY);
    fprintf(stderr, "ERROR: '%s' - '%s': %s\n", ptr, inst_str ? inst_str : "?", msg);
    free(inst_str);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <schema-file> < document.json\n", argv[0]);
        return 2;
    }

    /* Load schema from file */
    json_error_t jerr;
    json_t* schema = json_load_file(argv[1], 0, &jerr);
    if (!schema) {
        fprintf(stderr, "Error loading schema from '%s': %s\n", argv[1], jerr.text);
        return 1;
    }

    /* Create validator with file loader and built-in format checker */
    celix_jansson_schema_validator_t* validator = celix_jansson_schema_validator_create(
        file_loader, NULL, celix_jansson_schema_default_format_check, NULL, NULL, NULL);

    if (!validator) {
        fprintf(stderr, "Failed to create validator\n");
        json_decref(schema);
        return 1;
    }

    /* Compile the schema */
    char* errmsg = NULL;
    int rc = celix_jansson_schema_set_root_schema(validator, schema, &errmsg);
    json_decref(schema);

    if (rc != CELIX_JANSSON_SCHEMA_OK) {
        fprintf(stderr, "Schema compilation error: %s\n", errmsg ? errmsg : celix_jansson_schema_strerror(rc));
        free(errmsg);
        celix_jansson_schema_validator_destroy(validator);
        return 1;
    }

    /* Read document from stdin */
    json_t* instance = json_loadf(stdin, 0, &jerr);
    if (!instance) {
        fprintf(stderr, "Error parsing input: %s\n", jerr.text);
        celix_jansson_schema_validator_destroy(validator);
        return 1;
    }

    /* Validate */
    json_t* patch = NULL;
    int errors = celix_jansson_schema_validate(validator, instance, print_error, NULL, &patch);

    if (errors == 0) {
        fprintf(stderr, "Document is valid.\n");
    } else {
        fprintf(stderr, "Document is invalid (%d errors).\n", errors);
    }

    /* Print defaults patch if any */
    if (patch && json_array_size(patch) > 0) {
        char* patch_str = json_dumps(patch, JSON_INDENT(2));
        fprintf(stderr, "Default values patch:\n%s\n", patch_str ? patch_str : "");
        free(patch_str);

        /* Apply the patch to get a document with defaults filled in */
        json_t* filled = celix_jansson_schema_patch_apply(instance, patch);
        if (filled) {
            char* filled_str = json_dumps(filled, JSON_INDENT(2));
            fprintf(stderr, "Document with defaults:\n%s\n", filled_str ? filled_str : "");
            free(filled_str);
            json_decref(filled);
        }
    }
    json_decref(patch);

    json_decref(instance);
    celix_jansson_schema_validator_destroy(validator);

    return errors > 0 ? 1 : 0;
}
