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
#ifndef CELIX_CELIX_JANSSON_POINTER_H

#include "celix_jansson_ext_export.h"
#define CELIX_CELIX_JANSSON_POINTER_H

#include <jansson.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── JSON Pointer type ────────────────────────────────────────────────── */

/**
 * A JSON Pointer (RFC 6901) — a string-based path into a JSON document.
 *
 * Each token in the pointer corresponds to an object key (for JSON objects)
 * or an array index (for JSON arrays).  Tokens are stored unescaped.
 *
 * Example: the pointer "/store/book/0/title" contains four tokens:
 *          "store", "book", "0", "title"
 *
 * The struct is exposed like json_t so users can stack-allocate it.
 * Use celix_json_pointer_init() to initialize a stack-allocated instance
 * and celix_json_pointer_clear() to release its resources.
 */
typedef struct celix_json_pointer_t {
    char** tokens;
    size_t len;
    size_t cap;
} celix_json_pointer_t;

/* ── Lifecycle ─────────────────────────────────────────────────────────── */

/**
 * Create a new JSON Pointer from a string.
 *
 * The string must start with '/' (or be empty for the root pointer).
 * Returns NULL on parse failure or allocation error.
 *
 * @param ptr_str  RFC 6901 pointer string (e.g., "/foo/bar/0")
 * @return New pointer, or NULL on error.  Free with celix_json_pointer_destroy().
 */
CELIX_JANSSON_EXT_EXPORT celix_json_pointer_t* celix_json_pointer_create(const char* ptr_str);

/**
 * Initialize a stack-allocated pointer from a string.
 *
 * The pointer struct must be zeroed (e.g., with memset) before the first
 * call to this function.  Re-initialization is safe after calling
 * celix_json_pointer_clear() — the struct does not need to be re-zeroed.
 *
 * On error (-1), all partially allocated resources are automatically
 * cleaned up and the pointer is left in a cleared (empty) state.
 * The caller does NOT need to call celix_json_pointer_clear() after a
 * failed init.
 *
 * @param ptr      Pointer to stack-allocated celix_json_pointer_t (must be zeroed first)
 * @param ptr_str  RFC 6901 pointer string (must start with '/', or be "" for root)
 * @return 0 on success, -1 on parse failure or allocation error
 */
CELIX_JANSSON_EXT_EXPORT int celix_json_pointer_init(celix_json_pointer_t* ptr, const char* ptr_str);

/**
 * Create a deep copy of a pointer.
 *
 * @param src  The pointer to copy
 * @return New copy, or NULL on error.  Free with celix_json_pointer_destroy().
 */
CELIX_JANSSON_EXT_EXPORT celix_json_pointer_t* celix_json_pointer_copy(const celix_json_pointer_t* src);

/**
 * Free a heap-allocated pointer created by celix_json_pointer_create() or
 * celix_json_pointer_copy().  Does nothing if @p ptr is NULL.
 */
CELIX_JANSSON_EXT_EXPORT void celix_json_pointer_destroy(celix_json_pointer_t* ptr);

/**
 * Release all resources held by a stack-allocated pointer.
 * Does not free the struct itself.
 */
CELIX_JANSSON_EXT_EXPORT void celix_json_pointer_clear(celix_json_pointer_t* ptr);

/* ── Inspection ────────────────────────────────────────────────────────── */

/**
 * Return the number of tokens in this pointer.
 */
CELIX_JANSSON_EXT_EXPORT size_t celix_json_pointer_depth(const celix_json_pointer_t* ptr);

/**
 * Get the token at the given index (0-based).
 * Returns NULL if @p index is out of range.
 * The returned string is unescaped and owned by the pointer — do not free it.
 */
CELIX_JANSSON_EXT_EXPORT const char* celix_json_pointer_token(const celix_json_pointer_t* ptr, size_t index);

/* ── Mutation ──────────────────────────────────────────────────────────── */

/**
 * Append an unescaped token to the pointer.
 *
 * @param ptr    The pointer to modify
 * @param token  Unescaped token string (e.g., "foo bar")
 * @return 0 on success, -1 on error
 */
CELIX_JANSSON_EXT_EXPORT int celix_json_pointer_push(celix_json_pointer_t* ptr, const char* token);

/**
 * Remove the last token from the pointer.
 * Does nothing if the pointer is empty (has depth 0).
 */
CELIX_JANSSON_EXT_EXPORT void celix_json_pointer_pop(celix_json_pointer_t* ptr);

/* ── Serialization ─────────────────────────────────────────────────────── */

/**
 * Serialize the pointer to its RFC 6901 string representation.
 *
 * Tokens are escaped: '~' → "~0", '/' → "~1".
 * Returns a malloc'd string, or NULL on error.  Caller must free().
 */
CELIX_JANSSON_EXT_EXPORT char* celix_json_pointer_to_string(const celix_json_pointer_t* ptr);

/* ── Document access ───────────────────────────────────────────────────── */

/**
 * Check whether this pointer exists in the document.
 *
 * @param doc  The JSON document
 * @param ptr  The pointer to check
 * @return 1 if the path exists, 0 otherwise.  Never throws/errors.
 */
CELIX_JANSSON_EXT_EXPORT int celix_json_pointer_contains(json_t* doc, const celix_json_pointer_t* ptr);

/* ── Document access ───────────────────────────────────────────────────── */

/**
 * Resolve this pointer against a JSON document.
 *
 * Walks the document following object keys and array indices.
 * Returns a borrowed reference — do NOT json_decref().
 * Returns NULL if the path does not exist in the document.
 *
 * @param doc   The JSON document (object, array, or any value)
 * @param ptr   The pointer to resolve
 * @return The resolved json_t*, borrowed, or NULL
 */
CELIX_JANSSON_EXT_EXPORT json_t* celix_json_pointer_get(json_t* doc, const celix_json_pointer_t* ptr);

/**
 * Like celix_json_pointer_get(), but if the path does not exist, creates
 * intermediate objects/arrays as needed and returns the created node.
 *
 * If the last token is "-" (RFC 6901 array-end marker), creates a null
 * element at the end of the target array and returns the parent array.
 *
 * @param doc   The JSON document (must be an object or array)
 * @param ptr   The pointer to resolve/create
 * @return The resolved/created json_t*, new reference — caller must json_decref(),
 *         or NULL on error
 */
CELIX_JANSSON_EXT_EXPORT json_t* celix_json_pointer_get_or_create(json_t* doc, const celix_json_pointer_t* ptr);

/**
 * Set a value at the given pointer in a document.
 *
 * Creates intermediate objects/arrays as needed.
 * If the path already exists, the old value is replaced.
 *
 * @param doc    The JSON document (modified in-place)
 * @param ptr    The pointer path
 * @param value  The value to set (ownership is taken — "steal" semantics)
 * @return 0 on success, -1 on error
 */
CELIX_JANSSON_EXT_EXPORT int celix_json_pointer_set(json_t* doc, const celix_json_pointer_t* ptr, json_t* value);

/**
 * Set a value at the given pointer, incrementing the reference.
 * Like celix_json_pointer_set() but json_incref()s @p value instead of stealing it.
 */
CELIX_JANSSON_EXT_EXPORT int celix_json_pointer_set_new(json_t* doc, const celix_json_pointer_t* ptr, json_t* value);

/**
 * Remove the value at this pointer from the document.
 *
 * @param doc  The JSON document (modified in-place)
 * @param ptr  The pointer to remove
 * @return 0 on success, -1 if the path does not exist
 */
CELIX_JANSSON_EXT_EXPORT int celix_json_pointer_remove(json_t* doc, const celix_json_pointer_t* ptr);

/* ── Token escaping utilities ──────────────────────────────────────────── */

/**
 * Escape a single token for use in a JSON Pointer string.
 *
 * Replaces '~' with "~0" and '/' with "~1".
 * Returns a malloc'd string.  Caller must free().
 */
CELIX_JANSSON_EXT_EXPORT char* celix_json_pointer_escape(const char* token);

/**
 * Unescape a single token from a JSON Pointer string fragment.
 *
 * Replaces "~1" with '/' and "~0" with '~'.
 * Returns a malloc'd string.  Caller must free().
 */
CELIX_JANSSON_EXT_EXPORT char* celix_json_pointer_unescape(const char* token);

/* ── Navigation ────────────────────────────────────────────────────────── */

/**
 * Get the parent pointer by removing the last token.
 *
 * @param ptr   The source pointer
 * @param out   The parent pointer (must be zeroed first, or NULL to get a new one)
 * @return The parent pointer (out if provided, or a new allocation).
 *         Returns NULL if the pointer is already at the root.
 *         If allocated, free with celix_json_pointer_destroy().
 */
CELIX_JANSSON_EXT_EXPORT celix_json_pointer_t* celix_json_pointer_parent(const celix_json_pointer_t* ptr, celix_json_pointer_t* out);

/**
 * Append all tokens from @p suffix to @p ptr.
 *
 * @param ptr     The pointer to extend (modified in-place)
 * @param suffix  Tokens to append
 * @return 0 on success, -1 on error
 */
CELIX_JANSSON_EXT_EXPORT int celix_json_pointer_concat(celix_json_pointer_t* ptr, const celix_json_pointer_t* suffix);

/* ── Comparison ────────────────────────────────────────────────────────── */

/**
 * Compare two pointers for equality.
 *
 * @return 0 if equal, non-zero otherwise
 */
CELIX_JANSSON_EXT_EXPORT int celix_json_pointer_equals(const celix_json_pointer_t* a, const celix_json_pointer_t* b);

#ifdef __cplusplus
}
#endif

#endif /* CELIX_CELIX_JANSSON_POINTER_H */
