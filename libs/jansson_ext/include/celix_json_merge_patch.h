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
#ifndef CELIX_CELIX_JSON_MERGE_PATCH_H
#define CELIX_CELIX_JSON_MERGE_PATCH_H

#include "celix_jansson_ext_export.h"

#include <jansson.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Apply a JSON Merge Patch (RFC 7396) to a JSON document.
 *
 * Produces a new document; neither @p target nor @p patch is modified.
 * Semantics follow RFC 7396 section 2 exactly:
 *   - If @p patch is an object: if @p target is not an object, it is
 *     treated as an empty object; then for each member of @p patch, a
 *     null value removes the member from the target (a no-op if the
 *     member is absent), and any other value recursively merges into
 *     the target member (an absent member starts from null).
 *   - If @p patch is not an object (array, string, number, boolean or
 *     null): the result is a deep copy of @p patch, i.e. the patch
 *     replaces the whole document.
 *
 * Cyclic JSON structures are not supported and yield NULL (jansson's
 * deep copy rejects them).
 *
 * @param target    The target document (borrowed, never modified)
 * @param patch     The merge patch (borrowed, never modified)
 * @return A new json_t* with the patch applied, or NULL on invalid
 *         argument or out-of-memory.  On out-of-memory the partially
 *         built result is released.  The caller must json_decref() the
 *         result.
 */
CELIX_JANSSON_EXT_EXPORT json_t* celix_json_merge_patch(const json_t* target, const json_t* patch);

#ifdef __cplusplus
}
#endif

#endif /* CELIX_CELIX_JSON_MERGE_PATCH_H */
