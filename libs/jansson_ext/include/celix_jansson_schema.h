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

#ifndef CELIX_CELIX_JANSSON_SCHEMA_H

#include "celix_jansson_ext_export.h"
#define CELIX_CELIX_JANSSON_SCHEMA_H

#include <jansson.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ────────────────────────────────────────────────────────────────────────
 * Error codes
 * ──────────────────────────────────────────────────────────────────────── */

enum celix_jansson_schema_error_e {
    CELIX_JANSSON_SCHEMA_OK = 0,

    /* Allocation failure */
    CELIX_JANSSON_SCHEMA_ERROR_NOMEM,

    /* Schema JSON is not a boolean or object */
    CELIX_JANSSON_SCHEMA_ERROR_INVALID_SCHEMA,

    /* JSON parse error (in CLI or schema loader) */
    CELIX_JANSSON_SCHEMA_ERROR_SCHEMA_PARSE,

    /* Malformed URI (e.g., path appended to a URN) */
    CELIX_JANSSON_SCHEMA_ERROR_URI,

    /* Dangling $ref after all external files have been loaded */
    CELIX_JANSSON_SCHEMA_ERROR_REF_UNRESOLVED,

    /* schema_loader callback required but not provided or failed */
    CELIX_JANSSON_SCHEMA_ERROR_LOADER,

    /* format keyword present but no format_checker installed */
    CELIX_JANSSON_SCHEMA_ERROR_FORMAT_CHECKER,

    /* contentEncoding/contentMediaType present but no content_checker */
    CELIX_JANSSON_SCHEMA_ERROR_CONTENT_CHECKER,

    /* Duplicate URI (same location + fragment registered twice) */
    CELIX_JANSSON_SCHEMA_ERROR_DUPLICATE_URI,

    /* regcomp failed for a pattern keyword at schema compile time */
    CELIX_JANSSON_SCHEMA_ERROR_INVALID_PATTERN,

    /* validate() called before set_root_schema() */
    CELIX_JANSSON_SCHEMA_ERROR_NO_ROOT_SCHEMA,

    /* Invalid function argument */
    CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT,
};

/** Returns a human-readable description for an error code. */
const char* celix_jansson_schema_strerror(int err);

/* ────────────────────────────────────────────────────────────────────────
 * Callback typedefs
 * ──────────────────────────────────────────────────────────────────────── */

/**
 * Schema loader callback.
 *
 * Called when the validator encounters a $ref to an external URI.
 * The receiver MUST fill *schema_out with the parsed JSON schema document
 * on success.  On failure, return a non-zero JSS_ERROR_* code.
 *
 * @param uri       The URI of the schema to load (location only, no fragment)
 * @param schema_out  Output: set to the loaded schema JSON (new reference)
 * @param user_data  Opaque pointer passed to celix_jansson_schema_validator_create()
 * @return CELIX_JANSSON_SCHEMA_OK on success, or an error code on failure
 */
typedef int (*celix_jansson_schema_loader_fn)(const char* uri, json_t** schema_out, void* user_data);

/**
 * Format checker callback.
 *
 * Called when a schema uses the "format" keyword.  The receiver should
 * validate that @p value conforms to the named @p format.
 *
 * @param format     Format name (e.g., "date-time", "email", "ipv4")
 * @param value      The string value to check
 * @param user_data  Opaque pointer
 * @return CELIX_JANSSON_SCHEMA_OK if valid; any non-zero error code if invalid
 */
typedef int (*celix_jansson_schema_format_checker_fn)(const char* format, const char* value, void* user_data);

/**
 * Content checker callback.
 *
 * Called when a schema uses "contentEncoding" / "contentMediaType" keywords.
 *
 * @param content_encoding   e.g., "base64", "binary"
 * @param content_media_type e.g., "application/json"
 * @param instance           The JSON instance to validate
 * @param user_data          Opaque pointer
 * @return CELIX_JANSSON_SCHEMA_OK if valid; any non-zero error code if invalid
 */
typedef int (*celix_jansson_schema_content_checker_fn)(const char* content_encoding,
                                                       const char* content_media_type,
                                                       json_t* instance,
                                                       void* user_data);

/**
 * Validation error callback.
 *
 * Called for each validation error found during validate().
 *
 * @param json_pointer  JSON Pointer to the failing location (e.g., "/name")
 * @param instance      The JSON value that failed validation (borrowed ref)
 * @param message       Human-readable error description
 * @param user_data     Opaque pointer
 */
typedef void (*celix_jansson_schema_error_fn)(const char* json_pointer,
                                              json_t* instance,
                                              const char* message,
                                              void* user_data);

/* ────────────────────────────────────────────────────────────────────────
 * Default format checker
 * ──────────────────────────────────────────────────────────────────────── */

/**
 * Built-in format checker supporting all draft-7 defined formats.
 *
 * Supported: date-time, date, time, email, idn-email, hostname, ipv4, ipv6,
 *            uri, uuid, regex.
 *
 * Unsupported draft-7 formats (uri-reference, iri, iri-reference,
 * idn-hostname, json-pointer, relative-json-pointer, uri-template) return
 * CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT.
 *
 * The @p user_data parameter is ignored (may be NULL).
 * This matches the celix_jansson_schema_format_checker_fn signature.
 */
int celix_jansson_schema_default_format_check(const char* format, const char* value, void* user_data);

/* ────────────────────────────────────────────────────────────────────────
 * Validator handle (opaque)
 * ──────────────────────────────────────────────────────────────────────── */

typedef struct celix_jansson_schema_validator_t celix_jansson_schema_validator_t;

/* ────────────────────────────────────────────────────────────────────────
 * Lifecycle
 * ──────────────────────────────────────────────────────────────────────── */

/**
 * Create a new validator.
 *
 * @param loader   Schema loader for external $ref URIs (may be NULL if no
 *                 external refs are used)
 * @param loader_ud  User data for loader callback
 * @param format   Format checker (may be NULL; an error is raised at schema
 *                 compile time if a schema uses the "format" keyword but
 *                 no checker is installed)
 * @param format_ud  User data for format callback
 * @param content  Content checker (may be NULL; same rules as format)
 * @param content_ud User data for content callback
 * @return New validator, or NULL on allocation failure
 */
celix_jansson_schema_validator_t* celix_jansson_schema_validator_create(celix_jansson_schema_loader_fn loader,
                                                                        void* loader_ud,
                                                                        celix_jansson_schema_format_checker_fn format,
                                                                        void* format_ud,
                                                                        celix_jansson_schema_content_checker_fn content,
                                                                        void* content_ud);

/** Free a validator and all compiled schemas. */
void celix_jansson_schema_validator_destroy(celix_jansson_schema_validator_t* v);

/* ────────────────────────────────────────────────────────────────────────
 * Schema compilation
 * ──────────────────────────────────────────────────────────────────────── */

/**
 * Compile a JSON Schema for later validation.
 *
 * The input schema is deep-copied; the caller retains ownership.
 * This is a potentially expensive operation — call once, validate many times.
 *
 * @param v       Validator
 * @param schema  JSON Schema document (boolean or object per draft-7)
 * @return CELIX_JANSSON_SCHEMA_OK on success, or an error code; *errmsg is set on failure
 *         (caller must free *errmsg) and may be NULL if the caller does not
 *         need a detailed message
 */
int celix_jansson_schema_set_root_schema(celix_jansson_schema_validator_t* v, json_t* schema, char** errmsg);

/* ────────────────────────────────────────────────────────────────────────
 * Validation
 * ──────────────────────────────────────────────────────────────────────── */

/**
 * Validate a JSON instance against the compiled root schema.
 *
 * Thread-safe (const operation).  Must be called after a successful
 * set_root_schema().
 *
 * @param v          Validator
 * @param instance   JSON instance to validate (borrowed, not modified)
 * @param on_error   Error callback (may be NULL; errors are still counted)
 * @param error_ud   User data for error callback
 * @param patch_out  Output: JSON Patch (RFC 6902) array of default-value
 *                   insertions as json_t* (new reference).  May be NULL
 *                   if the caller does not need defaults.
 * @return The number of validation errors (0 = valid)
 */
int celix_jansson_schema_validate(celix_jansson_schema_validator_t* v,
                                  json_t* instance,
                                  celix_jansson_schema_error_fn on_error,
                                  void* error_ud,
                                  json_t** patch_out);

/**
 * Validate a JSON instance against a specific subschema identified by URI.
 *
 * The URI typically contains a JSON Pointer fragment, e.g.,
 * "#/definitions/MyType".
 *
 * @param v            Validator
 * @param instance     JSON instance to validate
 * @param initial_uri  URI of the subschema to validate against
 * @param on_error     Error callback (may be NULL)
 * @param error_ud     User data for error callback
 * @param patch_out    Output: JSON Patch array (may be NULL)
 * @return Number of validation errors (0 = valid)
 */
int celix_jansson_schema_validate_uri(celix_jansson_schema_validator_t* v,
                                      json_t* instance,
                                      const char* initial_uri,
                                      celix_jansson_schema_error_fn on_error,
                                      void* error_ud,
                                      json_t** patch_out);

/* ────────────────────────────────────────────────────────────────────────
 * JSON Patch application (RFC 6902)
 * ──────────────────────────────────────────────────────────────────────── */

/**
 * Apply a JSON Patch (RFC 6902) to a JSON document.
 *
 * The patch is an array of operations as returned by celix_jansson_schema_validate().
 * The original document is NOT modified — a patched copy is returned.
 *
 * Supported operations: "add", "remove", "replace".
 *
 * @param original  The original JSON document (not modified)
 * @param patch     JSON Patch array (e.g., from celix_jansson_schema_validate)
 * @return A new json_t* with the patch applied, or NULL on error.
 *         The caller must json_decref() the result.
 */
json_t* celix_jansson_schema_patch_apply(json_t* original, json_t* patch);

/* ────────────────────────────────────────────────────────────────────────
 * Built-in draft-7 meta-schema
 * ──────────────────────────────────────────────────────────────────────── */

/**
 * Returns a new reference to the embedded JSON Schema draft-7 meta-schema.
 *
 * Useful as the return value for a schema_loader when the requested URI is
 * "http://json-schema.org/draft-07/schema".
 */
json_t* celix_jansson_schema_draft7_meta_schema(void);

#ifdef __cplusplus
}
#endif

#endif /* CELIX_CELIX_JANSSON_SCHEMA_H */
