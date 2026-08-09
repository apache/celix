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
#ifndef CELIX_CELIX_JSON_PATCH_H
#define CELIX_CELIX_JSON_PATCH_H

#include "celix_jansson_ext_export.h"

#include <jansson.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Append an "add" operation (RFC 6902) to the patch array.
 *
 * The patch array is modified in-place.  The @p value is consumed —
 * ownership is transferred to the patch (steal semantics).  On failure
 * (invalid @p patch, or out-of-memory while building the operation) the
 * @p value is released and ownership is not transferred.
 * The stored value will be deep-copied later when the patch is applied
 * via celix_json_patch_apply().
 *
 * @param patch     The JSON Patch array (json_t* array, modified in-place)
 * @param path_str  JSON Pointer path for the operation (e.g., "/a/b")
 * @param value     The value to set at the path (ownership is taken on
 *                  success, released on failure)
 * @return 0 on success, -1 if @p patch is NULL or not an array, or on
 *         out-of-memory
 */
CELIX_JANSSON_EXT_EXPORT int celix_json_patch_add(json_t* patch, const char* path_str, json_t* value);

/**
 * Append a "replace" operation (RFC 6902) to the patch array.
 *
 * Like celix_json_patch_add(), @p value is consumed — ownership is
 * transferred to the patch.  On failure (invalid @p patch, or
 * out-of-memory while building the operation) the @p value is released
 * and ownership is not transferred.
 * At application time the value is deep-copied into the target document.
 *
 * @param patch     The JSON Patch array (json_t* array, modified in-place)
 * @param path_str  JSON Pointer path for the operation (e.g., "/a/b")
 * @param value     The new value (ownership is taken on success, released
 *                  on failure)
 * @return 0 on success, -1 if @p patch is NULL or not an array, or on
 *         out-of-memory
 */
CELIX_JANSSON_EXT_EXPORT int celix_json_patch_replace(json_t* patch, const char* path_str, json_t* value);

/**
 * Append a "remove" operation (RFC 6902) to the patch array.
 *
 * @param patch     The JSON Patch array (json_t* array, modified in-place)
 * @param path_str  JSON Pointer path of the value to remove
 * @return 0 on success, -1 if @p patch is NULL or not an array, or on
 *         out-of-memory
 */
CELIX_JANSSON_EXT_EXPORT int celix_json_patch_remove(json_t* patch, const char* path_str);

/**
 * Truncate the patch array back to @p old_size entries.
 *
 * Used internally for combinator rollback (e.g., anyOf/oneOf) to discard
 * patch entries from failed branches.  If the array has fewer than
 * @p old_size entries, this is a no-op.
 *
 * @param patch     The JSON Patch array (json_t* array, modified in-place)
 * @param old_size  Target number of entries to retain
 */
CELIX_JANSSON_EXT_EXPORT void celix_json_patch_truncate(json_t* patch, size_t old_size);

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
 *         On out-of-memory the partially built result is released and
 *         NULL is returned.  The caller must json_decref() the result.
 */
CELIX_JANSSON_EXT_EXPORT json_t* celix_json_patch_apply(json_t* original, json_t* patch);

#ifdef __cplusplus
}
#endif

#endif /* CELIX_CELIX_JSON_PATCH_H */
