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
#include "celix_json_merge_patch.h"

/**
 * Recursively merge @p patch into @p target.
 *
 * Consumes @p target (an owned reference from the caller) and returns a
 * new owned reference, or NULL on out-of-memory, in which case @p target
 * has been released.  @p patch is borrowed and never modified.  The
 * returned document is always distinct from @p patch.
 */
static json_t* merge_patch_recursive(json_t* target, const json_t* patch) {
    if (!json_is_object(patch)) {
        /* Whole-document replacement: the patch value becomes the result */
        json_t* replacement = json_deep_copy(patch);
        json_decref(target);
        return replacement;
    }

    if (!json_is_object(target)) {
        /* An object patch replaces a non-object target with a fresh object */
        json_decref(target);
        target = json_object();
        if (!target)
            return NULL;
    }

    const char* key;
    json_t* value;
    json_object_foreach((json_t*)patch, key, value) {
        if (json_is_null(value)) {
            /* A null value removes the member; an absent member is a no-op
             * (json_object_del returns -1 without side effects) */
            json_object_del(target, key);
        } else {
            /* An absent member is treated as starting from null (RFC 7396).
             * json_incref never fails; json_null only fails via error injection */
            json_t* sub = json_object_get(target, key);
            json_t* arg = sub ? json_incref(sub) : json_null();
            if (!arg) {
                json_decref(target);
                return NULL;
            }
            json_t* merged = merge_patch_recursive(arg, value);
            if (!merged) {
                json_decref(target);
                return NULL;
            }
            /* merged is consumed by a failed set_new as well (jansson
             * decrefs the value on all failure paths), so it must not be
             * released here */
            if (json_object_set_new(target, key, merged) != 0) {
                json_decref(target);
                return NULL;
            }
        }
    }

    return target;
}

json_t* celix_json_merge_patch(const json_t* target, const json_t* patch) {
    if (!target || !patch)
        return NULL;

    if (!json_is_object(patch))
        return json_deep_copy(patch); /* avoids copying an irrelevant target */

    json_t* copy = json_deep_copy(target);
    if (!copy)
        return NULL;

    return merge_patch_recursive(copy, patch);
}
